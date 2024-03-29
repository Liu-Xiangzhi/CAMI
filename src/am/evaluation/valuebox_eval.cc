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

#include <foundation/type/helper.h>
#include <exception.h>
#include <evaluation.h>

using namespace cami;
using namespace ts;

ValueBox am::operator+(ValueBox lhs, ValueBox rhs) // NOLINT
{
    lhs += std::move(rhs);
    return lhs;
}

ValueBox am::operator-(ValueBox lhs, ValueBox rhs) // NOLINT
{
    lhs -= std::move(rhs);
    return lhs;
}

ValueBox am::operator*(ValueBox lhs, ValueBox rhs) // NOLINT
{
    lhs *= std::move(rhs);
    return lhs;
}

ValueBox am::operator/(ValueBox lhs, ValueBox rhs) // NOLINT
{
    lhs /= std::move(rhs);
    return lhs;
}

ValueBox am::operator%(ValueBox lhs, ValueBox rhs) // NOLINT
{
    lhs %= std::move(rhs);
    return lhs;
}

ValueBox am::operator<<(ValueBox lhs, ValueBox rhs) // NOLINT
{
    lhs <<= std::move(rhs);
    return lhs;
}

ValueBox am::operator>>(ValueBox lhs, ValueBox rhs) // NOLINT
{
    lhs >>= std::move(rhs);
    return lhs;
}

ValueBox am::operator<(ValueBox lhs, ValueBox rhs) // NOLINT
{
    ASSERT(isArithmetic(lhs->type->kind()) && isArithmetic(rhs->type->kind()), "invalid type");
    ValueBox::usualArithmeticConvert(lhs, rhs);
    auto res = [&]() {
        switch (lhs->type->kind()) {
        case Kind::f32:
            return *static_cast<F32Value*>(lhs) < static_cast<F32Value*>(rhs);
        case Kind::f64:
            return *static_cast<F64Value*>(lhs) < static_cast<F64Value*>(rhs);
        default:
            ASSERT(isInteger(lhs->type->kind()), "no other type kind allowed");
            return *static_cast<IntegerValue*>(lhs) < static_cast<IntegerValue*>(rhs);
        }
    }();
    return ValueBox{res};
}

ValueBox am::operator<=(ValueBox lhs, ValueBox rhs) // NOLINT
{
    ASSERT(isArithmetic(lhs->type->kind()) && isArithmetic(rhs->type->kind()), "invalid type");
    ValueBox::usualArithmeticConvert(lhs, rhs);
    auto res = [&]() {
        switch (lhs->type->kind()) {
        case Kind::f32:
            return *static_cast<F32Value*>(lhs) <= static_cast<F32Value*>(rhs);
        case Kind::f64:
            return *static_cast<F64Value*>(lhs) <= static_cast<F64Value*>(rhs);
        default:
            ASSERT(isInteger(lhs->type->kind()), "no other type kind allowed");
            return *static_cast<IntegerValue*>(lhs) <= static_cast<IntegerValue*>(rhs);
        }
    }();
    return ValueBox{res};
}

ValueBox am::operator>(ValueBox lhs, ValueBox rhs)
{
    auto vb = std::move(lhs) <= std::move(rhs);
    auto& v = vb.get<IntegerValue>();
    v.val = !v.val;
    return vb;
}

ValueBox am::operator>=(ValueBox lhs, ValueBox rhs)
{
    auto vb = std::move(lhs) < std::move(rhs);
    auto& v = vb.get<IntegerValue>();
    v.val = !v.val;
    return vb;
}

ValueBox am::operator==(ValueBox lhs, ValueBox rhs) // NOLINT
{
    ASSERT((isPointerLike(lhs->type->kind()) && isPointerLike(rhs->type->kind())) ||
           (isArithmetic(lhs->type->kind()) || isArithmetic(rhs->type->kind())), "invalid type");
    ValueBox::usualArithmeticConvert(lhs, rhs);
    auto res = [&]() {
        switch (lhs->type->kind()) {
        case Kind::f32:
            return *static_cast<F32Value*>(lhs) == static_cast<F32Value*>(rhs);
        case Kind::f64:
            return *static_cast<F64Value*>(lhs) == static_cast<F64Value*>(rhs);
        case Kind::pointer:
            return rhs->type->kind() == Kind::pointer ?
                   *static_cast<PointerValue*>(lhs) == static_cast<PointerValue*>(rhs) :
                   *static_cast<PointerValue*>(lhs) == static_cast<NullValue*>(rhs);
        case Kind::null:
            return rhs->type->kind() == Kind::pointer ?
                   *static_cast<NullValue*>(lhs) == static_cast<PointerValue*>(rhs) :
                   *static_cast<NullValue*>(lhs) == static_cast<NullValue*>(rhs);
        default:
            ASSERT(isInteger(lhs->type->kind()), "no other type kind allowed");
            return *static_cast<IntegerValue*>(lhs) == static_cast<IntegerValue*>(rhs);
        }
    }();
    return ValueBox{res};
}

ValueBox am::operator!=(ValueBox lhs, ValueBox rhs)
{
    auto vb = std::move(lhs) == std::move(rhs);
    auto& v = vb.get<IntegerValue>();
    v.val = !v.val;
    return vb;
}

ValueBox am::operator&(ValueBox lhs, ValueBox rhs) // NOLINT
{
    lhs &= std::move(rhs);
    return lhs;
}

ValueBox am::operator|(ValueBox lhs, ValueBox rhs) // NOLINT
{
    lhs |= std::move(rhs);
    return lhs;
}

ValueBox am::operator^(ValueBox lhs, ValueBox rhs) // NOLINT
{
    lhs ^= std::move(rhs);
    return lhs;
}

void am::operator+=(ValueBox& lhs, ValueBox rhs)
{
    ASSERT(isArithmetic(lhs->type->kind()) && isArithmetic(rhs->type->kind()), "invalid type");
    ValueBox::usualArithmeticConvert(lhs, rhs);
    switch (lhs->type->kind()) {
    case Kind::f32:
        *static_cast<F32Value*>(lhs) += static_cast<F32Value*>(rhs);
        break;
    case Kind::f64:
        *static_cast<F64Value*>(lhs) += static_cast<F64Value*>(rhs);
        break;
    default:
        ASSERT(isInteger(lhs->type->kind()), "no other type kind allowed");
        *static_cast<IntegerValue*>(lhs) += static_cast<IntegerValue*>(rhs);
        break;
    }
}

void am::operator-=(ValueBox& lhs, ValueBox rhs) // NOLINT
{
    ASSERT(isArithmetic(lhs->type->kind()) && isArithmetic(rhs->type->kind()), "invalid type");
    ValueBox::usualArithmeticConvert(lhs, rhs);
    switch (lhs->type->kind()) {
    case Kind::f32:
        *static_cast<F32Value*>(lhs) += static_cast<F32Value*>(rhs)->inplaceNegation();
        break;
    case Kind::f64:
        *static_cast<F64Value*>(lhs) += static_cast<F64Value*>(rhs)->inplaceNegation();
        break;
    default:
        ASSERT(isInteger(lhs->type->kind()), "no other type kind allowed");
        *static_cast<IntegerValue*>(lhs) += static_cast<IntegerValue*>(rhs)->inplaceNegation();
        break;
    }
}

void am::operator*=(ValueBox& lhs, ValueBox rhs) // NOLINT
{
    ASSERT(isArithmetic(lhs->type->kind()) && isArithmetic(rhs->type->kind()), "invalid type");
    ValueBox::usualArithmeticConvert(lhs, rhs);
    switch (lhs->type->kind()) {
    case Kind::f32:
        *static_cast<F32Value*>(lhs) *= static_cast<F32Value*>(rhs);
        break;
    case Kind::f64:
        *static_cast<F64Value*>(lhs) *= static_cast<F64Value*>(rhs);
        break;
    default:
        ASSERT(isInteger(lhs->type->kind()), "no other type kind allowed");
        *static_cast<IntegerValue*>(lhs) *= static_cast<IntegerValue*>(rhs);
        break;
    }
}

void am::operator/=(ValueBox& lhs, ValueBox rhs) // NOLINT
{
    ASSERT(isArithmetic(lhs->type->kind()) && isArithmetic(rhs->type->kind()), "invalid type");
    ValueBox::usualArithmeticConvert(lhs, rhs);
    switch (lhs->type->kind()) {
    case Kind::f32:
        *static_cast<F32Value*>(lhs) /= static_cast<F32Value*>(rhs);
        break;
    case Kind::f64:
        *static_cast<F64Value*>(lhs) /= static_cast<F64Value*>(rhs);
        break;
    default:
        ASSERT(isInteger(lhs->type->kind()), "no other type kind allowed");
        *static_cast<IntegerValue*>(lhs) /= static_cast<IntegerValue*>(rhs);
        break;
    }
}

void am::operator%=(ValueBox& lhs, ValueBox rhs) // NOLINT
{
    ASSERT(isInteger(lhs->type->kind()) && isInteger(rhs->type->kind()), "invalid type");
    ValueBox::usualArithmeticConvert(lhs, rhs);
    *static_cast<IntegerValue*>(lhs) %= static_cast<IntegerValue*>(rhs);
}

void am::operator<<=(ValueBox& lhs, ValueBox rhs) // NOLINT
{
    ASSERT(isInteger(lhs->type->kind()) && isInteger(rhs->type->kind()), "invalid type");
    ValueBox::integerPromote(lhs);
    ValueBox::integerPromote(rhs);
    *static_cast<IntegerValue*>(lhs) <<= static_cast<IntegerValue*>(rhs);
}

void am::operator>>=(ValueBox& lhs, ValueBox rhs) // NOLINT
{
    ASSERT(isInteger(lhs->type->kind()) && isInteger(rhs->type->kind()), "invalid type");
    ValueBox::integerPromote(lhs);
    ValueBox::integerPromote(rhs);
    *static_cast<IntegerValue*>(lhs) >>= static_cast<IntegerValue*>(rhs);
}

void am::operator&=(ValueBox& lhs, ValueBox rhs) // NOLINT
{
    ASSERT(isInteger(lhs->type->kind()) && isInteger(rhs->type->kind()), "invalid type");
    ValueBox::usualArithmeticConvert(lhs, rhs);
    *static_cast<IntegerValue*>(lhs) &= static_cast<IntegerValue*>(rhs);
}

void am::operator|=(ValueBox& lhs, ValueBox rhs) // NOLINT
{
    ASSERT(isInteger(lhs->type->kind()) && isInteger(rhs->type->kind()), "invalid type");
    ValueBox::usualArithmeticConvert(lhs, rhs);
    *static_cast<IntegerValue*>(lhs) |= static_cast<IntegerValue*>(rhs);
}

void am::operator^=(ValueBox& lhs, ValueBox rhs) // NOLINT
{
    ASSERT(isInteger(lhs->type->kind()) && isInteger(rhs->type->kind()), "invalid type");
    ValueBox::usualArithmeticConvert(lhs, rhs);
    *static_cast<IntegerValue*>(lhs) ^= static_cast<IntegerValue*>(rhs);
}

ValueBox ValueBox::operator+() const
{
    ValueBox vb = *this;
    vb.inplacePositive();
    return vb;
}

ValueBox ValueBox::operator-() const
{
    ValueBox vb = *this;
    vb.inplaceNegation();
    return vb;
}

ValueBox ValueBox::operator~() const
{
    ValueBox vb = *this;
    vb.inplaceComplement();
    return vb;
}

ValueBox ValueBox::operator!() const
{
    ASSERT(isArithmetic(this->value->type->kind()) || this->value->type->kind() == Kind::pointer, "invalid type");
    return ValueBox{new IntegerValue{&type_manager.getBasicType(Kind::i32), this->isZero()}};
}

void ValueBox::inplacePositive()
{
    ASSERT(isArithmetic(this->value->type->kind()) || this->value->type->kind() == Kind::pointer, "invalid type");
    if (isInteger(this->value->type->kind())) {
        integerPromote(this->value);
    }
}

void ValueBox::inplaceNegation()
{
    ASSERT(isArithmetic(this->value->type->kind()), "invalid type");
    auto kind = this->value->type->kind();
    if (kind == Kind::f32) {
        this->get<F32Value>().inplaceNegation();
        return;
    } else if (kind == Kind::f64) {
        this->get<F64Value>().inplaceNegation();
        return;
    }
    ASSERT(isInteger(kind), "no other type allowed");
    auto& v = this->get<IntegerValue>();
    integerPromote(&v);
    v.inplaceNegation();
}

bool ValueBox::isZero() const
{
    ASSERT(isArithmetic(this->value->type->kind()) || this->value->type->kind() == Kind::pointer, "invalid type");
    switch (this->value->type->kind()) {
    case Kind::f32:
        return this->get<F32Value>().isZero();
    case Kind::f64:
        return this->get<F64Value>().isZero();
    case Kind::pointer:
        return this->get<PointerValue>().isZero();
    case Kind::null:
        return true;
    default:
        ASSERT(isInteger(this->value->type->kind()), "no other type kind allowed");
        return this->get<IntegerValue>().isZero();
    }
}

void ValueBox::inplaceComplement()
{
    ASSERT(isInteger(this->value->type->kind()), "invalid type");
    auto& v = this->get<IntegerValue>();
    integerPromote(&v);
    v.inplaceComplement();
}

void ValueBox::integerPromote(Value* v)
{
    ASSERT(isInteger(v->type->kind()), "invalid param");
    switch (v->type->kind()) {
    case Kind::char_:
    case Kind::i8:
    case Kind::i16:
        v->type = &type_manager.getBasicType(Kind::i32);
        break;
    case Kind::bool_:
    case Kind::u8:
    case Kind::u16:
        v->type = &type_manager.getBasicType(Kind::u32);
        break;
    default:
        break;
    }
}

void ValueBox::toFloat_uac(ValueBox& lhs, ValueBox& rhs)
{
    auto lhs_kind = lhs->type->kind();
    auto rhs_kind = rhs->type->kind();
    ASSERT(isFloat(lhs_kind) || isFloat(rhs_kind), "invalid param");
    if (lhs_kind == rhs_kind) {
        return;
    }
    if (lhs_kind == Kind::f64) {
        rhs.castTo(*lhs->type);
    } else if (lhs_kind == Kind::f32) {
        if (rhs_kind == Kind::f64) {
            lhs.castTo(*rhs->type);
        } else {
            rhs.castTo(*lhs->type);
        }
    } else {
        lhs.castTo(*rhs->type);
    }
}

void ValueBox::usualArithmeticConvert(ValueBox& lhs, ValueBox& rhs)
{
    const auto lhs_kind = lhs->type->kind();
    const auto rhs_kind = rhs->type->kind();
    ASSERT(isArithmetic(lhs_kind) && isArithmetic(rhs_kind), "invalid param");
    if (isFloat(lhs_kind) || isFloat(rhs_kind)) {
        return ValueBox::toFloat_uac(lhs, rhs);
    }
    integerPromote(lhs.value);
    integerPromote(rhs.value);
    if (lhs_kind == rhs_kind) {
        return;
    }
    if (isUnsigned(lhs_kind) == isUnsigned(rhs_kind)) {
        if (integerTypeRank(lhs_kind) > integerTypeRank(rhs_kind)) {
            rhs->type = lhs->type;
        } else {
            lhs->type = rhs->type;
        }
        return;
    }
    auto& signed_ = isSigned(lhs_kind) ? lhs : rhs;
    auto& unsigned_ = isSigned(lhs_kind) ? rhs : lhs;
    if (integerTypeRank(unsigned_->type->kind()) > integerTypeRank(signed_->type->kind())) {
        signed_->type = unsigned_->type;
        return;
    }
    // if the type of the operand with signed integer type can represent all the values
    // of the type of the operand with unsigned integer type, then the operand with unsigned
    // integer type is converted to the type of the operand with signed integer type.
    if (signed_->type->kind() == Kind::i64 && unsigned_->type->kind() == Kind::u32) {
        unsigned_->type = signed_->type;
        return;
    }
    auto common_type = &type_manager.getBasicType(correspondingUnsignedKind(signed_->type->kind()));
    signed_->type = common_type;
    unsigned_->type = common_type;
}

ValueBox ValueBox::cast(const ts::Type& type)
{
    ValueBox vb = *this;
    vb.castTo(type);
    return std::move(vb);
}

void ValueBox::delete_(Value* value)
{
    ASSERT(value != nullptr, "Value::delete_ is not nullptr-safe");
    switch (value->type->kind()) {
    case Kind::f32:
        delete down_cast<F32Value*>(value);
        break;
    case Kind::f64:
        delete down_cast<F64Value*>(value);
        break;
    case Kind::pointer:
        delete down_cast<PointerValue*>(value);
        break;
    case Kind::struct_:
    case Kind::union_:
        delete down_cast<StructOrUnionValue*>(value);
        break;
    default:
        ASSERT(isInteger(value->type->kind()), "no other type kind allowed");
        delete down_cast<IntegerValue*>(value);
        break;
    }
}

Value* ValueBox::copy(Value* value)
{
    ASSERT(value != nullptr, "Value::copy is not nullptr-safe");
    switch (value->type->kind()) {
    case Kind::f32:
        return new F32Value{down_cast<F32Value*>(value)->val};
    case Kind::f64:
        return new F64Value{down_cast<F64Value*>(value)->val};
    case Kind::pointer: {
        auto v = down_cast<PointerValue*>(value);
        return new PointerValue{v->type, *v->entity, v->offset};
    }
    case Kind::struct_:
    case Kind::union_:
        return new StructOrUnionValue{value->type, down_cast<StructOrUnionValue*>(value)->address};
    default:
        ASSERT(isInteger(value->type->kind()), "no other type kind allowed");
        return new IntegerValue{value->type, down_cast<IntegerValue*>(value)->val};
    }
}
