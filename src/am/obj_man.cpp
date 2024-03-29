/*******************************************************************************
 * Copyright (c) 2024. Liu Xiangzhi
 * This file is part of CAMI.
 *
 * CAMI is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software Foundation,
 * either version 2 of the License, or any later version.
 *
 * CAMI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with CAMI.
 * If not, see <https://www.gnu.org/licenses/>.
 ******************************************************************************/

#include <functional>
#include <set>
#include <obj_man.h>
#include <am.h>
#include <exception.h>
#include <foundation/type/helper.h>
#include <foundation/logger.h>

using namespace cami;
using namespace ts;
using am::ObjectManager;
using am::Object;

ObjectManager::~ObjectManager()
{
    const auto clearPage = [](Page& page) {
        for (size_t i = 0; i < page.usage; ++i) {
            page[i].~Object();
        }
    };
    clearPage(this->eden);
    clearPage(this->survivor[0]);
    clearPage(this->survivor[1]);
    clearPage(this->old_generation);
    clearPage(this->permanent);
}

Object* ObjectManager::new_(std::string name, const ts::Type& type, uint64_t address)
{
    this->state.gc.reset();
    auto alloc_num = countCorrespondingObjectFamily(type);
    this->state.alloc = alloc_num < LARGE_OBJ_THRESHOLD ? State::eden : State::old_generation;
    auto obj = [&]() {
        if (alloc_num < LARGE_OBJ_THRESHOLD) {
            this->state.alloc = State::eden;
            return this->newSmall(std::move(name), type, address);
        }
        this->state.alloc = State::old_generation;
        return this->newLarge(std::move(name), type, address);
    }();
    this->am.state.entities.emplace(obj->address, obj);
    return obj;
}

Object* ObjectManager::newPermanent(std::string name, const ts::Type& type, uint64_t address)
{
    ASSERT(countCorrespondingObjectFamily(type) + this->permanent.usage <= this->permanent.max_size,
           "too small permanent size");
    this->state.alloc = State::permanent;
    auto* obj = this->createObject(type, address);
    obj->name = std::move(name);
    this->am.state.entities.emplace(obj->address, obj);
    applyRecursively(*obj, [](Object& o) {
        o.status = Object::Status::well;
    });
    return obj;
}

Object* ObjectManager::newSmall(std::string name, const ts::Type& type, uint64_t address)
{
    auto alloc_num = countCorrespondingObjectFamily(type);
    if (this->eden.usage + alloc_num > this->eden.max_size) [[unlikely]] {
        if (!this->minorGC()) {
            return this->newLarge(std::move(name), type, address);
        }
        ASSERT(alloc_num <= this->eden.max_size && this->eden.usage == 0, "post-condition violation");
    }
    auto* obj = this->createObject(type, address);
    obj->name = std::move(name);
    return obj;
}

Object* ObjectManager::newLarge(std::string name, const ts::Type& type, uint64_t address)
{
    auto alloc_num = countCorrespondingObjectFamily(type);
    if (alloc_num + this->old_generation.usage > this->old_generation.max_size) [[unlikely]] {
        this->majorGC();
    }
    if (alloc_num + this->old_generation.usage > this->old_generation.max_size) [[unlikely]] {
        throw ObjectStorageOutOfMemoryException{name};
    }
    auto* obj = this->createObject(type, address);
    obj->name = std::move(name);
    return obj;
}

void ObjectManager::cleanup(Object* object)
{
    ASSERT(!object->super_object, "cannot cleanup non-top object");
    applyRecursively(*object, [](Object& obj) { obj.status = Object::Status::destroyed; });
    for (Object* item: object->referenced_by) {
        applyRecursively(*item, [](Object& obj) { obj.status = Object::Status::indeterminate; });
    }
    if (auto ref = this->getReferencedObject(object); ref) {
        [[maybe_unused]] auto cnt = (*ref)->referenced_by.erase(object);
        ASSERT(cnt == 1, "referenced object do not contains referencing object's reference");
    }
    [[maybe_unused]] auto cnt = this->am.state.entities.erase(object->address);
    ASSERT(cnt == 1, "global entities do not contains object being cleanup");
}

Object* ObjectManager::createObject(const Type& type, uint64_t address) // NOLINT
{
    auto obj = this->allocOneObject();
    if (isScalar(removeQualify(type).kind())) {
        new(obj) Object{"", type, address, {}, {}};
        return obj;
    }
    new(obj) Object{"", type, address, {}, this->createSubObject(type, address)};
    for (Object* item: obj->sub_objects) {
        item->super_object = obj;
    }
    return obj;
}

lib::Array<Object*> ObjectManager::createSubObject(const Type& _type, uint64_t address) // NOLINT
{
    const auto [qualifier, type] = peelQualify(_type);
    if (type.kind() == Kind::array) {
        auto& t = down_cast<const Array&>(type);
        lib::Array<Object*> sub_obj(t.len);
        auto sub_obj_addr = address;
        auto& sub_type = addQualify(t.element, qualifier);
        for (size_t i = 0; i < t.len; ++i) {
            sub_obj[i] = this->createObject(sub_type, sub_obj_addr);
            sub_obj_addr += sub_type.size();
        }
        return sub_obj;
    } else if (type.kind() == Kind::struct_) {
        auto& t = down_cast<const Struct&>(type);
        lib::Array<Object*> sub_obj(t.members.length());
        auto sub_obj_addr = address;
        for (size_t i = 0; i < sub_obj.length(); ++i) {
            sub_obj_addr = lib::roundUp(sub_obj_addr, t.members[i]->align());
            sub_obj[i] = this->createObject(addQualify(*t.members[i], qualifier), sub_obj_addr);
            sub_obj_addr += t.members[i]->size();
        }
        return sub_obj;
    } else {
        ASSERT(type.kind() == Kind::union_, "invalid object type");
        auto& t = down_cast<const Union&>(type);
        lib::Array<Object*> sub_obj(t.members.length());
        for (size_t i = 0; i < sub_obj.length(); ++i) {
            sub_obj[i] = this->createObject(addQualify(*t.members[i], qualifier), address);
        }
        return sub_obj;
    }
}

Object* ObjectManager::allocOneObject()
{
    auto& page = [&]() -> auto& {
        switch (this->state.alloc) {
        case State::eden:
            return this->eden;
        case State::old_generation:
            return this->old_generation;
        default:
            return this->permanent;
        }
    }();
    ASSERT(page.usage < page.max_size, "precondition violation");
    return &page[page.usage++];
}

bool ObjectManager::minorGC()
{
    this->minorGC_mark();
    auto [total_survivor_cnt, promote_cnt] = this->minorGC_statistic();
    return this->minorGC_arrange(total_survivor_cnt, promote_cnt);
}

void ObjectManager::minorGC_mark()
{
    this->markRootReachable();
    std::deque<Object*> working_queue{};
    // do not use `this->gc.root_reachable` which is used in major gc
    const auto getRootReachable = [&](Page& page) {
        for (size_t i = 0; i < page.usage; ++i) {
            if (page.testBitmap(i)) {
                working_queue.push_back(&page[i]);
            }
        }
    };
    getRootReachable(this->eden);
    getRootReachable(this->currentSurvivor());
    this->topdownSearchMark(working_queue);
    this->crossGenerationSearchMark(this->eden);
    this->crossGenerationSearchMark(this->currentSurvivor());
}

std::pair<uint64_t, uint64_t> ObjectManager::minorGC_statistic()
{
    uint64_t promote_cnt = 0;
    uint64_t total_survivor_cnt = 0;
    const auto saturateIncrease = [](uint8_t& val) {
        val = val == UINT8_MAX ? val : val + 1;
        return val;
    };
    const auto statistic = [&](Page& page) {
        for (size_t i = 0; i < page.usage; ++i) {
            if (page.testBitmap(i)) {
                total_survivor_cnt++;
                if (saturateIncrease(page[i].age) > PROMOTE_THRESHOLD) {
                    promote_cnt++;
                }
            }
        }
    };
    statistic(this->eden);
    statistic(this->currentSurvivor());
    return {total_survivor_cnt, promote_cnt};
}

bool ObjectManager::minorGC_arrange(uint64_t total_survivor_cnt, uint64_t promote_cnt)
{
    const auto isOldGenSpaceEnough = [&](uint64_t size) {
        return size <= this->old_generation.max_size - this->old_generation.usage;
    };
    const uint64_t survivor_cnt = total_survivor_cnt - promote_cnt;
    std::map<Object*, Object*> arrange_map;
    if (survivor_cnt > this->survivor[0].max_size) {
        if (!isOldGenSpaceEnough(total_survivor_cnt)) {
            this->majorGC();
        }
        if (!isOldGenSpaceEnough(total_survivor_cnt)) {
            return false;
        }
        this->arrangeYGToOldGeneration(this->eden, arrange_map);
        this->arrangeYGToOldGeneration(this->currentSurvivor(), arrange_map);
        this->eden.usage = 0;
        this->currentSurvivor().usage = 0;
    } else {
        if (!isOldGenSpaceEnough(promote_cnt)) [[unlikely]] {
            this->majorGC();
        }
        this->arrangeYGToSurvivor(this->eden, arrange_map, isOldGenSpaceEnough(promote_cnt));
        this->arrangeYGToSurvivor(this->currentSurvivor(), arrange_map, isOldGenSpaceEnough(promote_cnt));
        this->eden.usage = 0;
        this->currentSurvivor().usage = 0;
        this->state.survivor_idx = !this->state.survivor_idx;
    }
    this->amRefRelocate(arrange_map);
    return true;
}

void ObjectManager::majorGC()
{
    if (this->state.gc.majored) {
        return;
    }
    this->state.gc.majored = true;
    this->markRootReachable();
    this->topdownSearchMark(this->state.gc.root_reachable);
    this->amRefRelocate(this->shrinkOldGeneration());
}

void ObjectManager::markRootReachable()
{
    if (!this->state.gc.root_reachable.empty()) {
        return;
    }
    this->eden.resetBitmap();
    this->currentSurvivor().resetBitmap();
    this->old_generation.resetBitmap();
    for (const auto& item: this->am.operand_stack.getStack()) {
        auto& type = item.vb->getType();
        if (type.kind() == Kind::pointer
            && down_cast<const Pointer&>(type).referenced.kind() != Kind::function) {
            if (auto ptr = item.vb.get<PointerValue>().getReferenced();ptr) {
                this->markReachable(down_cast<Object*>(*ptr));
                this->state.gc.root_reachable.push_back(down_cast<Object*>(*ptr));
            }
        }
    }
    if (this->am.dsg_reg.entity != nullptr &&
        this->am.dsg_reg.entity->effective_type.kind() != Kind::function) {
        this->markReachable(down_cast<Object*>(this->am.dsg_reg.entity));
        this->state.gc.root_reachable.push_back(down_cast<Object*>(this->am.dsg_reg.entity));
    }
    for (const auto& item: this->am.state.call_stack) {
        for (Object* obj: item.automatic_objects) {
            if (obj != nullptr) {
                this->markReachable(obj);
                this->state.gc.root_reachable.push_back(obj);
            }
        }
    }
    for (size_t i = 0; i < this->permanent.usage; ++i) {
        if (auto ref = this->getReferencedObject(&this->permanent[i]); ref) {
            this->markReachable(*ref);
            this->state.gc.root_reachable.push_back(*ref);
        }
    }
}

void ObjectManager::markReachable(Object* object)
{
    if (this->belongToEden(object)) {
        this->eden.setBitmap(object);
        return;
    }
    if (this->belongToSurvivor(object)) {
        this->currentSurvivor().setBitmap(object);
        return;
    }
    if (this->belongToOldGeneration(object)) {
        this->old_generation.setBitmap(object);
        return;
    }
    ASSERT(this->belongToPermanent(object), "invalid object address");
    // do nothing for permanent object
}

bool ObjectManager::isMarked(Object* object)
{
    if (this->belongToEden(object)) {
        return this->eden.testBitmap(object);
    }
    if (this->belongToSurvivor(object)) {
        return this->currentSurvivor().testBitmap(object);
    }
    if (this->belongToOldGeneration(object)) {
        return this->old_generation.testBitmap(object);
    }
    ASSERT(this->belongToPermanent(object), "invalid object address");
    return true;
}

bool ObjectManager::belongToEden(Object* obj)
{
    return ObjectManager::belongTo(obj, this->eden);
}

bool ObjectManager::belongToSurvivor(Object* obj)
{
    return ObjectManager::belongTo(obj, this->currentSurvivor());
}

bool ObjectManager::belongToOldGeneration(Object* obj)
{
    return ObjectManager::belongTo(obj, this->old_generation);
}

bool ObjectManager::belongToPermanent(Object* obj)
{
    return ObjectManager::belongTo(obj, this->permanent);
}

bool ObjectManager::belongTo(Object* obj, Page& page)
{
    return obj >= page.data() && obj < page.data() + page.max_size;
}

lib::Optional<Object*> ObjectManager::getReferencedObject(const Object* obj) const
{
    auto& type = removeQualify(obj->effective_type);
    if (type.kind() != Kind::pointer){
        return {};
    }
    if (obj->isIndeterminateRepresentation()) {
        return {};
    }
    const auto isObject = [&](Entity* ent) {
        return ent->effective_type.kind() != Kind::function;
    };
    if (auto ptr = reinterpret_cast<Entity*>(this->am.memory.read64(obj->address)); ptr && isObject(ptr)) {
        return down_cast<Object*>(ptr);
    }
    return {};
}

void ObjectManager::topdownSearchMark(std::deque<Object*>& queue)
{
    while (!queue.empty()) {
        auto obj = queue.front();
        queue.pop_front();
        this->markReachable(obj);
        if (obj->super_object && !this->isMarked(*obj->super_object)) {
            queue.push_back(*obj->super_object);
        }
        if (auto ref = this->getReferencedObject(obj); ref && !this->isMarked(*ref)) {
            queue.push_back(*ref);
        }
        for (const auto& sub_obj: obj->sub_objects) {
            if (!this->isMarked(sub_obj)) {
                queue.push_back(sub_obj);
            }
        }
    }
}

void ObjectManager::crossGenerationSearchMark(Page& page)
{
    const auto hasCrossRef = [&](Object& obj) {
        return std::any_of(obj.referenced_by.begin(), obj.referenced_by.end(), [&](Object* o) {
            return this->belongToOldGeneration(o);
        });
    };
    std::deque<Object*> cross_refed_top_obj{};
    for (size_t i = 0; i < page.usage; ++i) {
        if (!this->isMarked(&page[i]) && hasCrossRef(page[i])
            && this->backwardsReachableTest(&page[i])) {
            cross_refed_top_obj.push_back(&page[i].top());
        }
    }
    this->topdownSearchMark(cross_refed_top_obj);
}

bool ObjectManager::backwardsReachableTest(Object* object)
{
    std::deque<Object*> queue{object};
    std::set<Object*> s{object};
    const auto searched = [&](Object* obj) { return s.find(obj) != s.end(); };
    const auto root_contains = [&](Object* obj) {
        return this->isMarked(object);
    };
    while (!queue.empty()) {
        auto obj = queue.front();
        queue.pop_front();
        if (obj->super_object) {
            if (root_contains(*obj->super_object)) {
                return true;
            }
            if (!searched(*obj->super_object)) {
                queue.push_back(*obj->super_object);
                s.insert(*obj->super_object);
            }
        }
        for (Object* item: obj->referenced_by) {
            if (root_contains(item)) {
                return true;
            }
            if (!searched(item)) {
                queue.push_back(item);
                s.insert(item);
            }
        }
    }
    return false;
}

void ObjectManager::arrangeYGToSurvivor(Page& page, std::map<Object*, Object*>& mapper, bool promote)
{
    auto& the_other_survivor = this->survivor[!this->state.survivor_idx];
    for (size_t i = 0; i < page.usage; ++i) {
        if (!page.testBitmap(i)) {
            ObjectManager::checkMemoryLeak(&page[i]);
            page[i].~Object();
            continue;
        }
        auto dest =
                promote && page[i].age > PROMOTE_THRESHOLD ? &this->old_generation[this->old_generation.usage++]
                                                           : &the_other_survivor[the_other_survivor.usage++];
        this->moveObject(&page[i], dest, mapper);
    }
}

void ObjectManager::arrangeYGToOldGeneration(Page& page, std::map<Object*, Object*>& mapper)
{
    for (size_t i = 0; i < page.usage; ++i) {
        if (!page.testBitmap(i)) {
            ObjectManager::checkMemoryLeak(&page[i]);
            page[i].~Object();
            continue;
        }
        this->moveObject(&page[i], &this->old_generation[this->old_generation.usage++], mapper);
    }
}

std::map<Object*, Object*> ObjectManager::shrinkOldGeneration()
{
    std::map<Object*, Object*> arrange_map{};
    uint64_t cnt = 0;
    for (size_t i = 0; i < this->old_generation.usage; ++i) {
        if (!this->old_generation.testBitmap(i)) {
            ObjectManager::checkMemoryLeak(&this->old_generation[i]);
            this->old_generation[i].~Object();
            continue;
        }
        ASSERT(i >= cnt, "");
        if (i > cnt) {
            this->moveObject(&this->old_generation[i], &this->old_generation[cnt], arrange_map);
        }
        cnt++;
    }
    this->old_generation.usage = cnt;
    return arrange_map;
}

void ObjectManager::moveObject(Object* src, Object* dest, std::map<Object*, Object*>& mapper)
{
    new(dest) Object{std::move(*src)};
    this->innerObjectRefRelocate(dest, src);
    mapper.emplace(src, dest);
}

void ObjectManager::innerObjectRefRelocate(Object* obj, Object* origin)
{
    if (obj->super_object) {
        for (auto& item: (*obj->super_object)->sub_objects) {
            if (item == origin) {
                item = obj;
            }
        }
        ASSERT(false, "super object do not contains sub-object's reference");
    }
    for (Object* item: obj->sub_objects) {
        item->super_object = obj;
    }
    for (Object* item: obj->referenced_by) {
        this->am.memory.write64(item->address, reinterpret_cast<uint64_t>(obj));
    }
    if (auto ref = this->getReferencedObject(obj);ref) {
        [[maybe_unused]] auto cnt = (*ref)->referenced_by.erase(origin);
        ASSERT(cnt == 1, "referenced object do not contains referencing object's reference");
        (*ref)->referenced_by.insert(obj);
    }
}

void ObjectManager::amRefRelocate(const std::map<Object*, Object*>& mapper)
{
    for (const auto& item: this->am.operand_stack.getStack()) {
        auto& type = item.vb->getType();
        if (type.kind() != Kind::pointer
            || down_cast<const Pointer&>(type).referenced.kind() == Kind::function) {
            continue;
        }
        if (auto ptr = item.vb.get<PointerValue>().getReferenced();ptr) {
            if (auto itr = mapper.find(down_cast<Object*>(*ptr)); itr != mapper.end()) {
                item.vb.get<PointerValue>().set(itr->second);
            }
        }
    }
    if (this->am.dsg_reg.entity != nullptr &&
        this->am.dsg_reg.entity->effective_type.kind() != Kind::function) {
        if (auto itr = mapper.find(down_cast<Object*>(this->am.dsg_reg.entity)); itr != mapper.end()) {
            this->am.dsg_reg.entity = itr->second;
        }
    }
    for (auto& item: this->am.state.call_stack) {
        for (auto& obj: item.automatic_objects) {
            if (auto itr = mapper.find(obj); itr != mapper.end()) {
                obj = itr->second;
            }
        }
    }
    for (auto& item: this->am.state.entities) {
        if (item.second->effective_type.kind() != Kind::function) {
            if (auto itr = mapper.find(down_cast<Object*>(item.second)); itr != mapper.end()) {
                item.second = itr->second;
            }
        }
    }
}

void ObjectManager::checkMemoryLeak(Object* object)
{
    // no need to use `upper_bound`, the whole object family must all leak or all not
    //  i.e. if the top object leak, then all its sub-objects(and sub-sub-objects,
    //  recursively) must leak, and vice versa.
    if (object->status != Object::Status::destroyed){
        log::unbuffered.wprintln("Memory leak! leaked object(auto cleaned by GC):\n${}", *object);
    }
}
