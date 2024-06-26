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

#include <cfloat>
#include <execute.h>
#include <exception.h>
#include <foundation/type/helper.h>
#include <lib/compiler_guarantee.h>
#include <ub.h>

using namespace cami;
using namespace ts;
using am::Execute;
using am::Entity;
using am::Object;
using am::UB;

void ValueBox::castTo(const Type& type)
{
    ASSERT(isArithmetic(this->value->type->kind()) && isArithmetic(type.kind()), "invalid cast");
#ifdef __GNUC__
    static const void* const table[16][16] = {
            // bool
            {&&no_cast, &&cast_type_only, &&cast_type_only, &&cast_type_only, &&cast_type_only, &&cast_type_only,
            nullptr, nullptr, nullptr, nullptr, &&cast_type_only, &&cast_type_only, &&cast_type_only, &&cast_type_only,
            &&unsigned_int_to_f32, &&unsigned_int_to_f64},
            // char
            {&&int_to_bool, &&no_cast, &&cast_type_only, &&cast_type_only, &&cast_type_only, &&cast_type_only,
            nullptr, nullptr, nullptr, nullptr, &&cast_and_extend, &&cast_and_extend, &&cast_and_extend, &&cast_and_extend,
            &&signed_int_to_f32, &&signed_int_to_f64},
            // i8
            {&&int_to_bool, &&cast_type_only, &&no_cast, &&cast_type_only, &&cast_type_only, &&cast_type_only,
            nullptr, nullptr, nullptr, nullptr, &&cast_and_extend, &&cast_and_extend, &&cast_and_extend, &&cast_and_extend,
            &&signed_int_to_f32, &&signed_int_to_f64},
            // i16
            {&&int_to_bool, &&cast_and_extend, &&cast_and_extend, &&no_cast, &&cast_type_only, &&cast_type_only,
            nullptr, nullptr, nullptr, nullptr, &&cast_and_extend, &&cast_and_extend, &&cast_and_extend, &&cast_and_extend,
            &&signed_int_to_f32, &&signed_int_to_f64},
            // i32
            {&&int_to_bool, &&cast_and_extend, &&cast_and_extend, &&cast_and_extend, &&no_cast, &&cast_type_only,
            nullptr, nullptr, nullptr, nullptr, &&cast_and_extend, &&cast_and_extend, &&cast_and_extend, &&cast_and_extend,
            &&signed_int_to_f32, &&signed_int_to_f64},
            // i64
            {&&int_to_bool, &&cast_and_extend, &&cast_and_extend, &&cast_and_extend, &&cast_and_extend, &&no_cast,
            nullptr, nullptr, nullptr, nullptr, &&cast_and_extend, &&cast_and_extend, &&cast_and_extend, &&cast_and_extend,
            &&signed_int_to_f32, &&signed_int_to_f64},
            {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                    nullptr, nullptr, nullptr, nullptr, nullptr},
            {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                    nullptr, nullptr, nullptr, nullptr, nullptr},
            {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                    nullptr, nullptr, nullptr, nullptr, nullptr},
            {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                    nullptr, nullptr, nullptr, nullptr, nullptr},
            // u8
            {&&int_to_bool, &&cast_and_extend, &&cast_and_extend, &&cast_type_only, &&cast_type_only, &&cast_type_only,
            nullptr, nullptr, nullptr, nullptr, &&no_cast, &&cast_type_only, &&cast_type_only, &&cast_type_only,
            &&unsigned_int_to_f32, &&unsigned_int_to_f64},
            // u16
            {&&int_to_bool, &&cast_and_extend, &&cast_and_extend, &&cast_and_extend, &&cast_type_only, &&cast_type_only,
            nullptr, nullptr, nullptr, nullptr, &&cast_and_extend, &&no_cast, &&cast_type_only, &&cast_type_only,
            &&unsigned_int_to_f32, &&unsigned_int_to_f64},
            // u32
            {&&int_to_bool, &&cast_and_extend, &&cast_and_extend, &&cast_and_extend, &&cast_and_extend, &&cast_type_only,
            nullptr, nullptr, nullptr, nullptr, &&cast_and_extend, &&cast_and_extend, &&no_cast, &&cast_type_only,
            &&unsigned_int_to_f32, &&unsigned_int_to_f64},
            // u64
            {&&int_to_bool, &&cast_and_extend, &&cast_and_extend, &&cast_and_extend, &&cast_and_extend, &&cast_and_extend,
            nullptr, nullptr, nullptr, nullptr, &&cast_and_extend, &&cast_and_extend, &&cast_and_extend, &&no_cast,
            &&unsigned_int_to_f32, &&unsigned_int_to_f64},
            // f32
            {&&f32_to_bool, &&f32_to_int, &&f32_to_int, &&f32_to_int, &&f32_to_int, &&f32_to_int,
            nullptr, nullptr, nullptr, nullptr, &&f32_to_int, &&f32_to_int, &&f32_to_int, &&f32_to_int,
            &&no_cast, &&f32_to_f64},
            // f64
            {&&f64_to_bool, &&f32_to_int, &&f64_to_int, &&f64_to_int, &&f64_to_int, &&f64_to_int,
            nullptr, nullptr, nullptr, nullptr, &&f64_to_int, &&f64_to_int, &&f64_to_int, &&f64_to_int,
            &&f64_to_f32, &&no_cast},
    };
    const void* target = table[static_cast<kind_t>(this->value->type->kind())][static_cast<kind_t>(type.kind())];
    ASSERT(target != nullptr, "invalid type kind value");
    goto *target;
#else
    switch (this->value->type->kind()) {
    case Kind::bool_:
        switch (type.kind()) {
        case Kind::bool_:
            goto no_cast;
        case Kind::char_:
        case Kind::i8:
        case Kind::i16:
        case Kind::i32:
        case Kind::i64:
        case Kind::u8:
        case Kind::u16:
        case Kind::u32:
        case Kind::u64:
            goto cast_type_only;
        case Kind::f32:
            goto unsigned_int_to_f32;
        case Kind::f64:
            goto unsigned_int_to_f64;
        }
    case Kind::char_:
        switch (type.kind()) {
        case Kind::bool_:
            goto int_to_bool;
        case Kind::char_:
            goto no_cast;
        case Kind::i8:
        case Kind::i16:
        case Kind::i32:
        case Kind::i64:
            goto cast_type_only;
        case Kind::u8:
        case Kind::u16:
        case Kind::u32:
        case Kind::u64:
            goto cast_and_extend;
        case Kind::f32:
            goto signed_int_to_f32;
        case Kind::f64:
            goto signed_int_to_f64;
        }
    case Kind::i8:
        switch (type.kind()) {
        case Kind::bool_:
            goto int_to_bool;
        case Kind::i8:
            goto no_cast;
        case Kind::char_:
        case Kind::i16:
        case Kind::i32:
        case Kind::i64:
            goto cast_type_only;
        case Kind::u8:
        case Kind::u16:
        case Kind::u32:
        case Kind::u64:
            goto cast_and_extend;
        case Kind::f32:
            goto signed_int_to_f32;
        case Kind::f64:
            goto signed_int_to_f64;
        }
    case Kind::i16:
        switch (type.kind()) {
        case Kind::bool_:
            goto int_to_bool;
        case Kind::char_:
        case Kind::i16:
            goto no_cast;
        case Kind::i32:
        case Kind::i64:
            goto cast_type_only;
        case Kind::i8:
        case Kind::u8:
        case Kind::u16:
        case Kind::u32:
        case Kind::u64:
            goto cast_and_extend;
        case Kind::f32:
            goto signed_int_to_f32;
        case Kind::f64:
            goto signed_int_to_f64;
        }
    case Kind::i32:
        switch (type.kind()) {
        case Kind::bool_:
            goto int_to_bool;
        case Kind::char_:
        case Kind::i8:
        case Kind::i32:
            goto no_cast;
        case Kind::i64:
            goto cast_type_only;
        case Kind::i16:
        case Kind::u8:
        case Kind::u16:
        case Kind::u32:
        case Kind::u64:
            goto cast_and_extend;
        case Kind::f32:
            goto signed_int_to_f32;
        case Kind::f64:
            goto signed_int_to_f64;
        }
    case Kind::i64:
        switch (type.kind()) {
        case Kind::bool_:
            goto int_to_bool;
        case Kind::char_:
        case Kind::i8:
        case Kind::i16:
        case Kind::i64:
            goto no_cast;
        case Kind::i32:
        case Kind::u8:
        case Kind::u16:
        case Kind::u32:
        case Kind::u64:
            goto cast_and_extend;
        case Kind::f32:
            goto signed_int_to_f32;
        case Kind::f64:
            goto signed_int_to_f64;
        }
    case Kind::u8:
        switch (type.kind()) {
        case Kind::bool_:
            goto int_to_bool;
        case Kind::char_:
        case Kind::i8:
            goto cast_and_extend;
        case Kind::u8:
            goto no_cast;
        case Kind::i16:
        case Kind::i32:
        case Kind::i64:
        case Kind::u16:
        case Kind::u32:
        case Kind::u64:
            goto cast_type_only;
        case Kind::f32:
            goto unsigned_int_to_f32;
        case Kind::f64:
            goto unsigned_int_to_f64;
        }
    case Kind::u16:
        switch (type.kind()) {
        case Kind::bool_:
            goto int_to_bool;
        case Kind::char_:
        case Kind::i8:
        case Kind::i16:
        case Kind::u8:
            goto cast_and_extend;
        case Kind::u16:
            goto no_cast;
        case Kind::i32:
        case Kind::i64:
        case Kind::u32:
        case Kind::u64:
            goto cast_type_only;
        case Kind::f32:
            goto unsigned_int_to_f32;
        case Kind::f64:
            goto unsigned_int_to_f64;
        }
    case Kind::u32:
        switch (type.kind()) {
        case Kind::bool_:
            goto int_to_bool;
        case Kind::i64:
        case Kind::u64:
            goto cast_type_only;
        case Kind::char_:
        case Kind::i8:
        case Kind::i16:
        case Kind::i32:
        case Kind::u8:
        case Kind::u16:
            goto cast_and_extend;
        case Kind::u32:
            goto no_cast;
        case Kind::f32:
            goto unsigned_int_to_f32;
        case Kind::f64:
            goto unsigned_int_to_f64;
        }
    case Kind::u64:
        switch (type.kind()) {
        case Kind::bool_:
            goto int_to_bool;
        case Kind::char_:
        case Kind::i8:
        case Kind::i16:
        case Kind::i32:
        case Kind::i64:
        case Kind::u8:
        case Kind::u16:
        case Kind::u32:
            goto cast_and_extend;
        case Kind::u64:
            goto no_cast;
        case Kind::f32:
            goto unsigned_int_to_f32;
        case Kind::f64:
            goto unsigned_int_to_f64;
        }
    case Kind::f32:
        switch (type.kind()) {
        case Kind::bool_:
            goto f32_to_bool;
        case Kind::char_:
        case Kind::i8:
        case Kind::i16:
        case Kind::i32:
        case Kind::i64:
        case Kind::u8:
        case Kind::u16:
        case Kind::u32:
        case Kind::u64:
            goto f32_to_int;
        case Kind::f32:
            goto no_cast;
        case Kind::f64:
            goto f32_to_f64;
        }
    case Kind::f64:
        switch (type.kind()) {
        case Kind::bool_:
            goto f64_to_bool;
        case Kind::char_:
        case Kind::i8:
        case Kind::i16:
        case Kind::i32:
        case Kind::i64:
        case Kind::u8:
        case Kind::u16:
        case Kind::u32:
        case Kind::u64:
            goto f64_to_int;
        case Kind::f32:
            goto f64_to_f32;
        case Kind::f64:
            goto no_cast;
        }
    }
#endif
cast_and_extend:
    {
        auto& v = this->get<IntegerValue>();
        v.type = &type;
        v.val = v.extend(v.val);
    }
    return;
cast_type_only:
    this->value->type = &type;
    return;
int_to_bool:
    {
        auto& v = this->get<IntegerValue>();
        v.type = &type_manager.getBasicType(Kind::bool_);
        v.val = v.val != 0;
    }
    return;
f32_to_bool:
    {
        auto& v = this->get<F32Value>();
        this->value = new IntegerValue{&type_manager.getBasicType(Kind::bool_), v.val != 0};
        ValueBox::delete_(&v);
    }
    return;
f64_to_bool:
    {
        auto& v = this->get<F64Value>();
        this->value = new IntegerValue{&type_manager.getBasicType(Kind::bool_), v.val != 0};
        ValueBox::delete_(&v);
    }
    return;
signed_int_to_f32:
    {
        auto& v = this->get<IntegerValue>();
        int64_t val = *reinterpret_cast<int64_t*>(&v.val);
        this->value = new F32Value{static_cast<float>(val)};
        ValueBox::delete_(&v);
    }
    return;
unsigned_int_to_f32:
    {
        auto& v = this->get<IntegerValue>();
        this->value = new F32Value{static_cast<float>(v.val)};
        ValueBox::delete_(&v);
    }
    return;
signed_int_to_f64:
    {
        auto& v = this->get<IntegerValue>();
        int64_t val = *reinterpret_cast<int64_t*>(&v.val);
        this->value = new F64Value{static_cast<double>(val)};
        ValueBox::delete_(&v);
    }
    return;
unsigned_int_to_f64:
    {
        auto& v = this->get<IntegerValue>();
        this->value = new F64Value{static_cast<double>(v.val)};
        ValueBox::delete_(&v);
    }
    return;
f32_to_int:
    {
        auto& v = this->get<F32Value>();
        auto [min, max] = [&]() -> std::pair<float, float> {
            auto kind = type.kind() == Kind::char_ ? Kind::i8 : type.kind();
            return {static_cast<float>(getMinValue(kind)), static_cast<float>(getMaxValue(kind))};
        }();
        if (v.val < min || v.val > max) {
            throw UBException{{UB::cast_to_or_from_integer}, lib::format(
                    "result(${}) of float type cannot cast to integer type `${}`", v.val, type)};
        }
        uint64_t val = isUnsigned(type.kind()) ? static_cast<uint64_t>(v.val) : static_cast<int64_t>(v.val);
        this->value = new IntegerValue{&type, val};
        ValueBox::delete_(&v);
    }
    return;
f64_to_int:
    {
        auto& v = this->get<F64Value>();
        auto [min, max] = [&]() -> std::pair<double, double> {
            auto kind = type.kind() == Kind::char_ ? Kind::i8 : type.kind();
            return {static_cast<double>(getMinValue(kind)), static_cast<double>(getMaxValue(kind))};
        }();
        if (v.val < min || v.val > max) {
            throw UBException{{UB::cast_to_or_from_integer}, lib::format(
                    "result(${}) of double type cannot cast to integer type `${}`", v.val, type)};
        }
        uint64_t val = isUnsigned(type.kind()) ? static_cast<uint64_t>(v.val) : static_cast<int64_t>(v.val);
        this->value = new IntegerValue{&type, val};
        ValueBox::delete_(&v);
    }
    return;
f32_to_f64:
    {
        auto& v = this->get<F32Value>();
        this->value = new F64Value{v.val};
        ValueBox::delete_(&v);
    }
    return;
f64_to_f32:
    {
        auto& v = this->get<F64Value>();
        if (v.val < FLT_MIN || v.val > FLT_MAX) {
            throw UBException{{UB::real_float_demotion}, lib::format(
                    "result(${}) of double type cannot cast to float", v.val)};
        }
        this->value = new F32Value{static_cast<float>(v.val)};
        ValueBox::delete_(&v);
    }
    return;
no_cast:
    return;
}

namespace {
// determine whether `object` or its subobject or subsubobject ...(all with the same address) is designated
lib::Optional<Object*> resolveObjectDesignation(Object* object, const Type& type) // NOLINT
{
    ASSERT(type.kind() != Kind::function, "precondition violation");
    if (&object->effective_type == &type) {
        return object;
    }
    switch (removeQualify(object->effective_type).kind()) {
    case Kind::array:
        ASSERT(object->sub_objects.length() > 0, "length of array cannot be zero");
        return resolveObjectDesignation(object->sub_objects[0], type);
    case Kind::struct_:
        if (object->sub_objects.empty()) {
            return {};
        }
        return resolveObjectDesignation(object->sub_objects[0], type);
    case Kind::union_:
        for (Object* item: object->sub_objects) {
            if (resolveObjectDesignation(item, type)) {
                return item;
            }
        }
        return {};
    default:
        // match failed
        return {};
    }
}

lib::Optional<Object*> designateObject(Object* object, uint64_t offset, const Type& type) // NOLINT
{
    if (offset == 0) {
        return resolveObjectDesignation(object, type);
    }
    auto& obj_type = removeQualify(object->effective_type);
    if (obj_type.kind() == Kind::array) {
        auto sub_obj_size = down_cast<const Array&>(obj_type).element.size();
        // this situation may happen due to internal padding of struct
        if (offset / sub_obj_size >= object->sub_objects.length()) {
            return {};
        }
        return designateObject(object->sub_objects[offset / sub_obj_size], offset % sub_obj_size, type);
    } else if (obj_type.kind() == Kind::struct_) {
        auto& t = down_cast<const Struct&>(obj_type);
        uint64_t off = 0;
        for (size_t i = 0; i < t.members.length(); ++i) {
            off = lib::roundUp(off, t.members[i]->align()) + t.members[i]->size();
            if (off > offset) {
                ASSERT(i < object->sub_objects.length(), "invalid offset or sub_object size miss matches the type size");
                return designateObject(object->sub_objects[i], offset - (off - t.members[i]->size()), type);
            }
        }
        return nullptr;
    } else if (obj_type.kind() == Kind::union_) {
        for (auto item: object->sub_objects) {
            if (auto o = designateObject(item, offset, type); o) {
                return o;
            }
        }
        return {};
    } else if (isCCharacter(type.kind()) && offset <= object->effective_type.size()) {
        return object;
    }
    return {};
}

void checkPointerToInteger(uint64_t pointer_address, const ts::Type& type)
{
    auto kind = type.kind() == Kind::char_ ? Kind::i8 : type.kind();
    ASSERT(isStrictInteger(kind), "precondition violation");
    uint64_t max = getMaxValue(kind);
    if (pointer_address > max) {
        throw UBException{{UB::cast_to_or_from_integer}, lib::format(
                "result(${x}) of pointer type cannot cast to integer type `${}`", pointer_address, type)};
    }
}
} // anonymous namespace

void Execute::castIntegerToPointer(AbstractMachine& am, ValueBox& operand, const Type& type)
{
    ASSERT(type.kind() == Kind::pointer, "precondition violation");
    auto int_val = operand.get<IntegerValue>().uint64();
    auto itr = am.state.entities.upper_bound(int_val);
    if (itr == am.state.entities.begin()) {
        operand = ValueBox{new DissociativePointerValue{&type, int_val}};
        return;
    }
    --itr;
    auto entity = itr->second;
    auto& ref_type = removeQualify(down_cast<const Pointer&>(type).referenced);
    if (entity->effective_type.kind() == Kind::function || ref_type.kind() == Kind::function) {
        if (int_val != itr->first) {
            operand = ValueBox{new DissociativePointerValue{&type, int_val}};
        } else {
            operand = ValueBox{new PointerValue{&type, entity, 0}};
        }
        return;
    }
    auto* obj = down_cast<Object*>(entity);
    if (int_val - itr->first >= obj->effective_type.size()) {
        operand = ValueBox{new DissociativePointerValue{&type, int_val}};
        return;
    }
    if (auto ref_obj = designateObject(obj, int_val - itr->first, ref_type);ref_obj) {
        operand = ValueBox{new PointerValue{&type, *ref_obj, int_val - (*ref_obj)->address}};
        return;
    }
    operand = ValueBox{new DissociativePointerValue{&type, int_val}};
}

void Execute::castPointerToPointer(ValueBox& operand, const ts::Type& type)
{
    ASSERT(type.kind() == Kind::pointer, "precondition violation");
    auto& ptr = operand.get<PointerValue>();
    auto& ptr_ref_type = down_cast<const Pointer&>(ptr.getType()).referenced;
    auto& cast_type_ref_type = down_cast<const Pointer&>(type).referenced;
    if (ptr_ref_type.kind() == Kind::function) {
        ptr.set(&type);
        return;
    }
    if (cast_type_ref_type.kind() == Kind::function) {
        if (ptr.getOffset() != 0) {
            throw ConstraintViolationException{lib::format(
                    "Cannot cast pointer points past object(${}) to a pointer points function", ptr)};
        }
        ptr.set(&type);
        return;
    }
    // equivalent to `ptr.getAddress() % type.align()`
    if (cast_type_ref_type.kind() != Kind::void_ && ptr.getAddress() & (cast_type_ref_type.align() - 1)) {
        throw UBException{{UB::unaligned_ptr_cast}, lib::format(
                "pointer cast from `${}` to an unaligned type `${}`", ptr.getType(), type)};
    }
    ASSERT(cast_type_ref_type.kind() == Kind::void_ || ((cast_type_ref_type.align() - 1) & cast_type_ref_type.align()) == 0,
           "align must be the power of 2");
    // cast may cause change of referenced object
    //  e.g. `int (*) [2]` cast to `int*`, referenced object changes from array to int
    if (auto obj = ptr.getReferenced(); obj) {
        auto top_obj = &down_cast<Object&>(**obj).top();
        auto _new_ref_obj = designateObject(top_obj, ptr.getAddress() - top_obj->address, cast_type_ref_type);
        auto new_ref_obj = _new_ref_obj ? *_new_ref_obj : *obj;
        auto offset = ptr.getAddress() - new_ref_obj->address;
        if (!isCCharacter(type.kind()) && offset != 0 && offset != new_ref_obj->effective_type.size()) {
            throw ConstraintViolationException{
                    lib::format("pointer cast from `${}` to type `${}` caused indeterminate object designation", ptr.getType(), type)};
        }
        ptr.set(&type, new_ref_obj, offset);
    } else {
        ptr.set(&type);
    }
}

void Execute::cast(AbstractMachine& am, InstrInfo info)
{
    // evaluation of cast expression will discard qualifier of the type of the result
    //  so type to which cast has no meaning to be qualified
    COMPILER_GUARANTEE(info.getTypeID() < am.static_info.types.length(),
                       lib::format("Value(${}) of type id out of boundary(${})", info.getTypeID(), am.static_info.types.length()));
    auto& type = removeQualify(*am.static_info.types[info.getTypeID()]);
    auto& rich_value = am.operand_stack.top();
    if (rich_value.attr.indeterminate) {
        return;
    }
    auto& operand = rich_value.vb;
    COMPILER_GUARANTEE(isScalar(type.kind()) &&
                       (isScalar(operand->getType().kind()) || operand->getType().kind() == Kind::dissociative_pointer),
                       lib::format("invalid type in cast operator. cast from `${}` to `${}`", operand->getType(), type));
    if (operand->getType().kind() == Kind::null) {
        if (type.kind() == Kind::pointer) {
            operand = ValueBox{new PointerValue{&type, nullptr, 0}};
        } else if (type.kind() == Kind::bool_) {
            operand = ValueBox{new IntegerValue{false}};
        } else {
            COMPILER_GUARANTEE(type.kind() == Kind::null, lib::format("cast nullptr to a non-nullptr_t type `${}`", type));
        }
    } else if (operand->getType().kind() == Kind::pointer) {
        if (type.kind() == Kind::pointer) {
            Execute::castPointerToPointer(operand, type);
        } else {
            COMPILER_GUARANTEE(isInteger(type.kind()), lib::format("cast pointer to a non-pointer non-integer type `${}`", type));
            if (type.kind() == Kind::bool_) {
                operand = ValueBox{new IntegerValue{!operand.get<PointerValue>().isZero()}};
            } else {
                auto addr = operand.get<PointerValue>().getAddress();
                checkPointerToInteger(addr, type);
                operand = ValueBox{new IntegerValue{&type, addr}};
            }
        }
    } else if (operand->getType().kind() == Kind::dissociative_pointer) {
        if (type.kind() == Kind::pointer) {
            auto& op_type = operand.get<DissociativePointerValue>().pointer_type;
            auto& op_ref_type = down_cast<const Pointer&>(*op_type).referenced;
            auto& cast_ref_type = down_cast<const Pointer&>(type).referenced;
            if (op_ref_type.kind() != Kind::function && cast_ref_type.kind() != Kind::function && cast_ref_type.kind() != Kind::void_ &&
                // equivalent to `address % align != 0`
                (operand.get<DissociativePointerValue>().address & (cast_ref_type.align() - 1))) {
                throw UBException{{UB::unaligned_ptr_cast}, lib::format(
                        "pointer cast from `${}` to an unaligned type `${}`", *op_type, type)};
            }
            op_type = &type;
        } else {
            COMPILER_GUARANTEE(isInteger(type.kind()), lib::format("cast pointer to a non-pointer non-integer type `${}`", type));
            if (type.kind() == Kind::bool_) {
                operand = ValueBox{new IntegerValue{operand.get<DissociativePointerValue>().address != 0}};
            } else {
                auto addr = operand.get<DissociativePointerValue>().address;
                checkPointerToInteger(addr, type);
                operand = ValueBox{new IntegerValue{&type, addr}};
            }
        }
    } else {
        // isArithmetic(operand->getType().kind()) == true
        COMPILER_GUARANTEE(isArithmetic(type.kind()) || type.kind() == Kind::pointer, lib::format(
                "cast arithmetic type object to a non-arithmetic non-pointer type `${}`", type));
        if (type.kind() == Kind::pointer) {
            COMPILER_GUARANTEE(isInteger(operand->getType().kind()), "cast non-integer arithmetic type object to pointer");
            Execute::castIntegerToPointer(am, operand, type);
        } else {
            operand.castTo(type);
        }
    }
}
