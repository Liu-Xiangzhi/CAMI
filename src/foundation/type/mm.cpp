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

#include <type/mm.h>
#ifdef CAMI_TARGET_INFO_UNIX_LIKE
#include <unistd.h>
#else
#endif

using namespace cami;
using namespace cami::ts;
using ts::detail::TrivialDeconstructedPool;
using ts::detail::TypePoolBase;
using ts::detail::TypePool;
using ts::detail::page_size;

const size_t ts::detail::page_size = []() -> size_t {
#ifdef CAMI_TARGET_INFO_UNIX_LIKE
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1) {
        perror("Failed to get page size, set 4096 by default");
        return 4096;
    }
    if (page_size < 4096) {
        std::fputs("Too small page size, set 4096 by default", stderr);
        return 4096;
    }
    return page_size;
#else
#endif
}();
MemoryManager ts::type_manager;

TrivialDeconstructedPool::~TrivialDeconstructedPool()
{
    for (uint8_t* item: this->pool) {
        delete[] item;
    }
}

uint8_t* TrivialDeconstructedPool::alloc(size_t size, size_t align)
{
    this->prev_usage = this->usage;
    this->usage = lib::roundUp(this->usage, align);
    if (size + this->usage > page_size) {
        this->new_page();
        this->prev_usage = 0;
    }
    auto pos = this->getCurrentPage() + this->usage;
    this->usage += size;
    return pos;
}

void TrivialDeconstructedPool::dealloc()
{
    this->usage = this->prev_usage;
}

void TrivialDeconstructedPool::new_page()
{
    this->usage = 0;
    this->pool.push_back(new unsigned char[page_size]);
}

uint8_t* TrivialDeconstructedPool::getCurrentPage() const noexcept
{
    return this->pool.back();
}

uint8_t* TypePoolBase::alloc(size_t size)
{
    if (size + this->usage > page_size) {
        this->new_page();
    }
    auto pos = this->getCurrentPage() + this->usage;
    this->usage += size;
    return pos;
}

void TypePoolBase::new_page()
{
    if (!this->pool.empty()) {
        this->pool.back().usage = this->usage;
    }
    this->pool.push_back({new unsigned char[page_size], 0});
    this->usage = 0;
}

uint8_t* TypePoolBase::getCurrentPage() const noexcept
{
    return this->pool.back().addr;
}

template<typename T>
TypePool<T>::~TypePool()
{
    if (this->pool.empty()) [[unlikely]] {
        return;
    }
    this->pool.back().usage = this->usage;
    for (auto& item: this->pool) {
        auto* addr = reinterpret_cast<T*>(item.addr);
        for (size_t i = 0; i < item.usage / sizeof(T); ++i) {
            MemoryManager::delete_(addr);
            addr++;
        }
        delete item.addr;
    }
}

template<typename T>
template<typename ...ARGS>
T* TypePool<T>::alloc(ARGS&& ...args)
{
    auto p = reinterpret_cast<T*>(TypePoolBase::alloc(sizeof(T)));
    MemoryManager::new_(p, std::forward<ARGS>(args)...);
    return p;
}

template<typename T>
void TypePool<T>::dealloc()
{
    ASSERT(this->usage >= sizeof(T), "incorrect function call");
    this->usage -= sizeof(T);
    MemoryManager::delete_(reinterpret_cast<T*>(this->getCurrentPage() + this->usage));
}

MemoryManager::MemoryManager()
{
    auto _basic_area = this->trivial_deconstructed_area.alloc(
            sizeof(Basic) * (static_cast<kind_t>(Kind::_basic_max) + 2), 1);
    auto ba = reinterpret_cast<Basic*>(_basic_area);
    this->basic_area = ba;
    for (size_t i = 0; i <= static_cast<kind_t>(Kind::i64); ++i) {
        new(ba++) Integer{static_cast<Kind>(i)};
    }
    ba += static_cast<kind_t>(Kind::u8) - static_cast<kind_t>(Kind::i64) - 1;
    for (int i = static_cast<kind_t>(Kind::u8); i <= static_cast<kind_t>(Kind::u64); ++i) {
        new(ba++) Integer{static_cast<Kind>(i)};
    }
    new(ba++) Float{Kind::f32};
    new(ba++) Float{Kind::f64};
    new(ba++) Basic{Kind::void_};
    new(ba++) Basic{Kind::null};
    new(ba++) Basic{Kind::dissociative_pointer};
    new(ba) Basic{Kind::ivd};
}

const Basic& MemoryManager::getInvalid()
{
    return this->basic_area[static_cast<kind_t>(Kind::_basic_max) + 1];
}

const Basic& MemoryManager::getDissociativePointer()
{
    return this->basic_area[static_cast<kind_t>(Kind::_basic_max)];
}

const Basic& MemoryManager::getBasicType(Kind kind)
{
    ASSERT(kind < Kind::_basic_max, "param `kind` out of boundary");
    ASSERT(kind <= Kind::i64 || kind >= Kind::u8, "invalid param");
    return this->basic_area[static_cast<kind_t>(kind)];
}

const Pointer& MemoryManager::getPointer(const Type& referenced)
{
    auto _area = this->trivial_deconstructed_area.alloc(sizeof(Pointer), alignof(Pointer));
    auto area = reinterpret_cast<Pointer*>(_area);
    new(area) Pointer{referenced};
    if (auto itr = this->index.find(area); itr != this->index.end()) {
        this->trivial_deconstructed_area.dealloc();
        return down_cast<const Pointer&>(**itr);
    }
    this->index.insert(area);
    return *area;
}

const Function& MemoryManager::getFunction(const Type& returned, lib::Array<const Type*> params)
{
    auto area = this->function_area.alloc(returned, std::move(params));
    if (auto itr = this->index.find(area); itr != this->index.end()) {
        this->function_area.dealloc();
        return down_cast<const Function&>(**itr);
    }
    this->index.insert(area);
    return *area;
}

const Array& MemoryManager::getArray(const Type& element, size_t len)
{
    auto _area = this->trivial_deconstructed_area.alloc(sizeof(Array), alignof(Array));
    auto area = reinterpret_cast<Array*>(_area);
    new(area) Array{element, len};
    if (auto itr = this->index.find(area); itr != this->index.end()) {
        this->trivial_deconstructed_area.dealloc();
        return down_cast<const Array&>(**itr);
    }
    this->index.insert(area);
    return *area;
}

const Qualify& MemoryManager::getQualifiedType(const Type& qualified, Qualifier qualifier)
{
    auto _area = this->trivial_deconstructed_area.alloc(sizeof(Qualify), alignof(Qualify));
    auto area = reinterpret_cast<Qualify*>(_area);
    new(area) Qualify{qualified, qualifier};
    if (auto itr = this->index.find(area); itr != this->index.end()) {
        this->trivial_deconstructed_area.dealloc();
        return down_cast<const Qualify&>(**itr);
    }
    this->index.insert(area);
    return *area;
}

void MemoryManager::createStruct(std::string name, lib::Array<const Type*> members)
{
    auto itr = this->struct_mapper.find(name);
    Struct* area;
    if (itr != this->struct_mapper.end()) {
        area = itr->second;
        // must erase original map info (because the type of key is string_view which
        //   views the string deleted at line 232
        this->struct_mapper.erase(name);
        area->~Struct();
        new(area) Struct{std::move(name), std::move(members)};
    } else {
        area = this->struct_area.alloc(std::move(name), std::move(members));
    }
    this->struct_mapper.insert({area->name, area});
}

void MemoryManager::createUnion(std::string name, lib::Array<const Type*> members)
{
    auto itr = this->union_mapper.find(name);
    Union* area;
    if (itr != this->union_mapper.end()) {
        area = itr->second;
        this->union_mapper.erase(name);
        area->~Union();
        new(area) Union{std::move(name), std::move(members)};
    } else {
        area = this->union_area.alloc(std::move(name), std::move(members));
    }
    this->union_mapper.insert({area->name, area});
}

const Struct& MemoryManager::declareStruct(std::string name)
{
    if (auto itr = this->struct_mapper.find(name);itr != this->struct_mapper.end()) {
        return *itr->second;
    }
    auto area = this->struct_area.alloc(std::move(name), lib::Array<const Type*>{});
    this->struct_mapper.insert({area->name, area});
    return *area;
}

const Union& MemoryManager::declareUnion(std::string name)
{
    if (auto itr = this->union_mapper.find(name); itr != this->union_mapper.end()) {
        return *itr->second;
    }
    auto area = this->union_area.alloc(std::move(name), lib::Array<const Type*>{});
    this->union_mapper.insert({area->name, area});
    return *area;
}
