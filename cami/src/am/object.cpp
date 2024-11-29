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

#include <object.h>
#include <foundation/type/helper.h>

using namespace cami;
using namespace am;
using namespace ts;

Object& Object::top() noexcept
{
    auto o = this;
    for (; o->super_object; o = *o->super_object) {}
    return *o;
}

Object& Object::topOfSameAddress() noexcept
{
    auto o = this;
    for (; o->super_object && o->address == (*o->super_object)->address; o = *o->super_object) {}
    return *o;
}

void am::applyRecursively(Object& object, const std::function<void(Object&)>& func) // NOLINT
{
    func(object);
    for (Object* item: object.sub_objects) {
        applyRecursively(*item, func);
    }
}

void am::applyBottom(cami::am::Object& object, const std::function<void(Object&)>& func)
{
    if (object.sub_objects.empty()) {
        func(object);
        return;
    }
    for (Object* item: object.sub_objects) {
        applyRecursively(*item, func);
    }
}

void am::copyStatus(Object& from, Object& to) // NOLINT
{
    ASSERT(isCompatible(from.effective_type, to.effective_type), "copy status between objects of incompatible type");
    to.status = from.status;
    for (size_t i = 0; i < to.sub_objects.length(); ++i) {
        copyStatus(*from.sub_objects[i], *to.sub_objects[i]);
    }
}

bool am::checkStatusForRead(Object& object) // NOLINT
{
    auto& type = removeQualify(object.effective_type);
    if (type.kind() == Kind::array) {
        // lvalue_conversion, just read address
        return true;
    }
    if (isScalar(type.kind())) {
        return object.status == Object::Status::well;
    }
    if (type.kind() == Kind::struct_) {
        return std::all_of(object.sub_objects.begin(), object.sub_objects.end(), [](Object* o) {
            return checkStatusForRead(*o);
        });
    }
    ASSERT(type.kind() == Kind::union_, "no other type should occur");
    return std::any_of(object.sub_objects.begin(), object.sub_objects.end(), [](Object* o) {
        return checkStatusForRead(*o);
    });
}

static void do_updateCommonInitialSequenceStatus(Object& cur_obj, Object& modified) // NOLINT
{
    auto& cur_obj_type = removeQualify(cur_obj.effective_type);
    if (isScalar(cur_obj_type.kind())) {
        if (isCompatible(cur_obj_type, modified.effective_type)) {
            copyStatus(modified, cur_obj);
        }
        return;
    }
    if (cur_obj_type.kind() == Kind::array || cur_obj_type.kind() == Kind::struct_) {
        if (cur_obj.sub_objects.length() > 0) {
            do_updateCommonInitialSequenceStatus(*cur_obj.sub_objects[0], modified);
        }
        return;
    }
    ASSERT(removeQualify(cur_obj.effective_type).kind() == Kind::union_, "no other type should occur");
    for (Object* item: cur_obj.sub_objects) {
        do_updateCommonInitialSequenceStatus(*item, modified);
    }
}

void am::updateCommonInitialSequenceStatus(Object& object)
{
    do_updateCommonInitialSequenceStatus(object.topOfSameAddress(), object);
}
