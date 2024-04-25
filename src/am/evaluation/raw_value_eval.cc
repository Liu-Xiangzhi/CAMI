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

#include <foundation/value.h>
#include <foundation/type/helper.h>
#include <exception.h>
#include <ub.h>

using namespace cami;
using namespace ts;
using am::Entity;
using am::UB;

uint64_t IntegerValue::extend(uint64_t value) const noexcept
{
    ASSERT(this->type->kind() != Kind::bool_, "cannot process bool type");
    auto kind = this->type->kind();
    kind = kind == Kind::char_ ? Kind::i8 : kind;
    if (isUnsigned(kind)) {
        return getMaxValue(kind) & value;
    } else {
        auto exponent = static_cast<kind_t>(kind) - static_cast<kind_t>(Kind::i8) + 3;
        auto width = 1 << exponent;
        auto shift = 64 - width;
        uint64_t tmp = value << shift;
        return *reinterpret_cast<const int64_t*>(&tmp) >> shift;
    }
}

uint64_t IntegerValue::complement() const
{
    return ~this->val;
}

uint64_t IntegerValue::negation() const
{
    return -this->val;
}

uint64_t IntegerValue::add(IntegerValue* rhs) const
{
    ASSERT(this->type->kind() == rhs->type->kind() && integerTypeRank(this->type->kind()) >= integerTypeRank(Kind::i32),
           "integer promotion or usual arithmetic conversion is not correctly performed");
    auto res = this->val + rhs->val;
    auto overflow = [&]() -> bool {
        if (isUnsigned(this->type->kind())) {
            return false;
        }
        if (this->type->kind() == Kind::i32) {
            return ((res >> 1) ^ res) & 0x8000'0000;
        }
        constexpr uint64_t MASK = ~(1ULL << 63);
        auto msb_carry = ((this->val & MASK) + (rhs->val & MASK)) >> 63;
        auto sb_carry = ((this->val >> 63) + (rhs->val >> 63) + msb_carry) >> 1;
        return msb_carry ^ sb_carry;
    }();
    if (overflow) {
        throw UBException{{UB::exceptional_condition}, lib::format(
                "Integer addition overflow. lhs = `${}`, rhs = `${}`", *this, *rhs)};
    }
    return res;
}

uint64_t IntegerValue::mul(IntegerValue* rhs) const
{
    ASSERT(this->type->kind() == rhs->type->kind() && integerTypeRank(this->type->kind()) >= integerTypeRank(Kind::i32),
           "integer promotion or usual arithmetic conversion is not correctly performed");
    if (isUnsigned(this->type->kind())) {
        return this->val * rhs->val;
    }
    const int64_t lhs_val = *reinterpret_cast<const int64_t*>(&this->val);
    const int64_t rhs_val = *reinterpret_cast<const int64_t*>(&rhs->val);
    const auto abs = [](int64_t v) -> uint64_t {
        return v > 0 ? v : -static_cast<uint64_t>(v);
    };
    auto [overflow, result] = [&]() -> std::pair<bool, uint64_t> {
        if (this->type->kind() == Kind::i32) {
            auto res = lhs_val * rhs_val;
            auto signed_res = *reinterpret_cast<int64_t*>(&res);
            return {signed_res > INT32_MAX || signed_res < INT32_MIN, res};
        }
        // treat `a * b` as `(a1 + a0 * 2**32) * (b1 + b0 * 2**32)`
        const uint64_t lhs_hi = abs(lhs_val) >> 32;
        const uint64_t lhs_lo = abs(lhs_val) & 0xffff'ffff;
        const uint64_t rhs_hi = abs(rhs_val) >> 32;
        const uint64_t rhs_lo = abs(rhs_val) & 0xffff'ffff;
        const bool positive = (lhs_val > 0) == (rhs_val > 0);
        if (lhs_hi != 0 && rhs_hi != 0) { // quadratic factor should be zero
            return {true, 0}; // value of `result` doesn't matter if overflow
        }
        const auto constant_factor = lhs_lo * rhs_lo;
        const auto linear_factor = lhs_lo * rhs_hi + lhs_hi * rhs_lo;
        if (linear_factor >> 32 != 0 || linear_factor + (constant_factor >> 32) > UINT32_MAX) {
            return {true, 0};
        }
        auto res_abs = (linear_factor << 32) + constant_factor;
        if (positive) {
            return {res_abs > INT64_MAX, res_abs};
        }
        // clangd says that the below expression is always `false`, it should be false alarm
        //   e.g. a1 = b1 = 0xcfff'ffff, a0 = b0 = 0
        return {res_abs > static_cast<uint64_t>(INT64_MIN), -res_abs};
    }();
    if (overflow) {
        throw UBException{{UB::exceptional_condition}, lib::format(
                "Integer multiply overflow. lhs = `${}`, rhs = `${}`", *this, *rhs)};
    }
    return result;
}

uint64_t IntegerValue::div(IntegerValue* rhs) const
{
    ASSERT(this->type->kind() == rhs->type->kind() && integerTypeRank(this->type->kind()) >= integerTypeRank(Kind::i32),
           "integer promotion or usual arithmetic conversion is not correctly performed");
    if (rhs->val == 0) {
        throw UBException{{UB::exceptional_condition, UB::div_or_mod_zero, UB::div_not_representable}, "Div zero"};
    }
    if (isUnsigned(this->type->kind())) {
        return this->val / rhs->val;
    }
    const int64_t lhs_val = *reinterpret_cast<const int64_t*>(&this->val);
    const int64_t rhs_val = *reinterpret_cast<const int64_t*>(&rhs->val);
    auto overflow = lhs_val == (this->type->kind() == Kind::i32 ? INT32_MIN : INT64_MIN) && rhs_val == -1;
    if (overflow) {
        throw UBException{{UB::exceptional_condition, UB::div_not_representable}, lib::format(
                "Integer division overflow. lhs = `${}`, rhs = -1", lhs_val)};
    }
    return lhs_val / rhs_val;
}

uint64_t IntegerValue::mod(IntegerValue* rhs) const
{
    ASSERT(this->type->kind() == rhs->type->kind() && integerTypeRank(this->type->kind()) >= integerTypeRank(Kind::i32),
           "integer promotion or usual arithmetic conversion is not correctly performed");
    if (rhs->val == 0) {
        throw UBException{{UB::exceptional_condition, UB::div_or_mod_zero, UB::div_not_representable}, "Mod zero"};
    }
    if (isUnsigned(this->type->kind())) {
        return this->val % rhs->val;
    }
    const int64_t lhs_val = *reinterpret_cast<const int64_t*>(&this->val);
    const int64_t rhs_val = *reinterpret_cast<const int64_t*>(&rhs->val);
    auto div_overflow = lhs_val == (this->type->kind() == Kind::i32 ? INT32_MIN : INT64_MIN) && rhs_val == -1;
    if (div_overflow) {
        throw UBException{{UB::exceptional_condition, UB::div_not_representable}, lib::format(
                "Integer division overflow when doing modular. lhs = `${}`, rhs = -1", lhs_val)};
    }
    return lhs_val % rhs_val;
}

bool IntegerValue::less(IntegerValue* rhs) const
{
    ASSERT(this->type->kind() == rhs->type->kind() && integerTypeRank(this->type->kind()) >= integerTypeRank(Kind::i32),
           "integer promotion or usual arithmetic conversion is not correctly performed");
    if (isUnsigned(this->type->kind())) {
        return this->val < rhs->val;
    }
    int64_t lhs_val = *reinterpret_cast<const int64_t*>(&this->val);
    int64_t rhs_val = *reinterpret_cast<const int64_t*>(&rhs->val);
    return lhs_val < rhs_val;
}

bool IntegerValue::lessEqual(IntegerValue* rhs) const
{
    ASSERT(this->type->kind() == rhs->type->kind() && integerTypeRank(this->type->kind()) >= integerTypeRank(Kind::i32),
           "integer promotion or usual arithmetic conversion is not correctly performed");
    if (isUnsigned(this->type->kind())) {
        return this->val <= rhs->val;
    }
    int64_t lhs_val = *reinterpret_cast<const int64_t*>(&this->val);
    int64_t rhs_val = *reinterpret_cast<const int64_t*>(&rhs->val);
    return lhs_val <= rhs_val;
}

bool IntegerValue::equal(IntegerValue* rhs) const
{
    ASSERT(this->type->kind() == rhs->type->kind() && integerTypeRank(this->type->kind()) >= integerTypeRank(Kind::i32),
           "integer promotion or usual arithmetic conversion is not correctly performed");
    return this->val == rhs->val;
}

uint64_t IntegerValue::bitwiseAnd(IntegerValue* rhs) const
{
    ASSERT(this->type->kind() == rhs->type->kind() && integerTypeRank(this->type->kind()) >= integerTypeRank(Kind::i32),
           "integer promotion or usual arithmetic conversion is not correctly performed");
    return this->val & rhs->val;
}

uint64_t IntegerValue::bitwiseOr(IntegerValue* rhs) const
{
    ASSERT(this->type->kind() == rhs->type->kind() && integerTypeRank(this->type->kind()) >= integerTypeRank(Kind::i32),
           "integer promotion or usual arithmetic conversion is not correctly performed");
    return this->val | rhs->val;
}

uint64_t IntegerValue::bitwiseXor(IntegerValue* rhs) const
{
    ASSERT(this->type->kind() == rhs->type->kind() && integerTypeRank(this->type->kind()) >= integerTypeRank(Kind::i32),
           "integer promotion or usual arithmetic conversion is not correctly performed");
    return this->val ^ rhs->val;
}

uint64_t IntegerValue::leftShift(IntegerValue* rhs) const
{
    ASSERT(isSameIntegerWithoutSigness(this->type->kind(), rhs->type->kind()) &&
           integerTypeRank(this->type->kind()) >= integerTypeRank(Kind::i32),
           "integer promotion or usual arithmetic conversion is not correctly performed");
    bool _32bit = correspondingUnsignedKind(this->type->kind()) == Kind::u32;
    auto isNegative = [](uint64_t value) -> bool { return value >> 63; };
    auto is32bitNegative = [](uint64_t value) -> bool { return value >> 31; };
    if (isNegative(rhs->val) || rhs->val >= (_32bit ? 32 : 64)) {
        throw UBException{{UB::ivd_rhs_of_shift}, lib::format(
                "Value(${}) of right hand side operand of logical shift is negative or out of the range representable by the promoted type",
                *reinterpret_cast<int64_t*>(&rhs->val))};
    }
    auto res = this->val << rhs->val;
    if (isSigned(this->type->kind())) {
        if (isNegative(this->val)) {
            throw UBException{{UB::ivd_result_of_left_shit}, lib::format(
                    "invalid left hand side value(${}) of left shift operator", *reinterpret_cast<const int64_t*>(&this->val))};
        }
        if (_32bit ? is32bitNegative(res) : isNegative(res)) {
            throw UBException{{UB::ivd_result_of_left_shit}, lib::format(
                    "invalid result value(${}) of left shift operator", *reinterpret_cast<const int64_t*>(&res))};
        }
    }
    return res;
}

uint64_t IntegerValue::rightShift(IntegerValue* rhs) const
{
    ASSERT(isSameIntegerWithoutSigness(this->type->kind(), rhs->type->kind()) &&
           integerTypeRank(this->type->kind()) >= integerTypeRank(Kind::i32),
           "integer promotion or usual arithmetic conversion is not correctly performed");
    bool _32bit = correspondingUnsignedKind(this->type->kind()) == Kind::u32;
    auto isNegative = [](uint64_t value) -> bool { return value >> 63; };
    if (isNegative(rhs->val) || rhs->val >= (_32bit ? 32 : 64)) {
        throw UBException{{UB::ivd_rhs_of_shift}, lib::format(
                "Value(${}) of right hand side operand of logical shift is negative or out of the range representable by the promoted type",
                *reinterpret_cast<int64_t*>(&rhs->val))};
    }
    return this->val >> rhs->val;
}

bool IntegerValue::isZero() const noexcept
{
    return this->val == 0;
}

IntegerValue* IntegerValue::operator~() const
{
    return new IntegerValue{this->type, this->complement()};
}

IntegerValue* IntegerValue::operator-() const
{
    return new IntegerValue{this->type, this->negation()};
}

IntegerValue* IntegerValue::operator+(IntegerValue* rhs) const
{
    return new IntegerValue{this->type, this->add(rhs)};
}

IntegerValue* IntegerValue::operator*(IntegerValue* rhs) const
{
    return new IntegerValue{this->type, this->mul(rhs)};
}

IntegerValue* IntegerValue::operator/(IntegerValue* rhs) const
{
    return new IntegerValue{this->type, this->div(rhs)};
}

IntegerValue* IntegerValue::operator%(IntegerValue* rhs) const
{
    return new IntegerValue{this->type, this->mod(rhs)};
}

IntegerValue* IntegerValue::operator<(IntegerValue* rhs) const
{
    return new IntegerValue{&type_manager.getBasicType(Kind::i32), this->less(rhs)};
}

IntegerValue* IntegerValue::operator<=(IntegerValue* rhs) const
{
    return new IntegerValue{&type_manager.getBasicType(Kind::i32), this->lessEqual(rhs)};
}

IntegerValue* IntegerValue::operator==(IntegerValue* rhs) const
{
    return new IntegerValue{&type_manager.getBasicType(Kind::i32), this->equal(rhs)};
}

IntegerValue* IntegerValue::operator&(IntegerValue* rhs) const
{
    return new IntegerValue{this->type, this->bitwiseAnd(rhs)};
}

IntegerValue* IntegerValue::operator|(IntegerValue* rhs) const
{
    return new IntegerValue{this->type, this->bitwiseOr(rhs)};
}

IntegerValue* IntegerValue::operator^(IntegerValue* rhs) const
{
    return new IntegerValue{this->type, this->bitwiseXor(rhs)};
}

IntegerValue* IntegerValue::operator<<(IntegerValue* rhs) const
{
    return new IntegerValue{this->type, this->leftShift(rhs)};
}

IntegerValue* IntegerValue::operator>>(IntegerValue* rhs) const
{
    return new IntegerValue{this->type, this->rightShift(rhs)};
}

IntegerValue* IntegerValue::inplaceComplement()
{
    this->val = this->extend(this->complement());
    return this;
}

IntegerValue* IntegerValue::inplaceNegation()
{
    this->val = this->extend(this->negation());
    return this;
}

void IntegerValue::operator+=(IntegerValue* rhs)
{
    this->val = this->extend(this->add(rhs));
}

void IntegerValue::operator*=(IntegerValue* rhs)
{
    this->val = this->extend(this->mul(rhs));
}

void IntegerValue::operator/=(IntegerValue* rhs)
{
    this->val = this->extend(this->div(rhs));
}

void IntegerValue::operator%=(IntegerValue* rhs)
{
    this->val = this->extend(this->mod(rhs));
}

void IntegerValue::operator&=(IntegerValue* rhs)
{
    this->val = this->extend(this->bitwiseAnd(rhs));
}

void IntegerValue::operator|=(IntegerValue* rhs)
{
    this->val = this->extend(this->bitwiseOr(rhs));
}

void IntegerValue::operator^=(IntegerValue* rhs)
{
    this->val = this->extend(this->bitwiseXor(rhs));
}

void IntegerValue::operator<<=(IntegerValue* rhs)
{
    this->val = this->extend(this->leftShift(rhs));
}

void IntegerValue::operator>>=(IntegerValue* rhs)
{
    this->val = this->extend(this->rightShift(rhs));
}

float F32Value::negation() const
{
    return -this->val;
}

float F32Value::add(F32Value* rhs) const
{
    return this->val + rhs->val;
}

float F32Value::mul(F32Value* rhs) const
{
    return this->val * rhs->val;
}

float F32Value::div(F32Value* rhs) const
{
    if (rhs->isZero()) {
        throw UBException{{UB::exceptional_condition, UB::div_or_mod_zero, UB::div_not_representable}, "Div zero"};
    }
    return this->val / rhs->val;
}

bool F32Value::less(F32Value* rhs) const
{
    return this->val < rhs->val;
}

bool F32Value::lessEqual(F32Value* rhs) const
{
    return this->val <= rhs->val;
}

bool F32Value::equal(F32Value* rhs) const
{
    return this->val == rhs->val;
}

bool F32Value::isZero() const noexcept
{
    return this->val == 0;
}

F32Value* F32Value::operator-() const
{
    return new F32Value{this->negation()};
}

F32Value* F32Value::operator+(F32Value* rhs) const
{
    return new F32Value{this->add(rhs)};
}

F32Value* F32Value::operator*(F32Value* rhs) const
{
    return new F32Value{this->mul(rhs)};
}

F32Value* F32Value::operator/(F32Value* rhs) const
{
    return new F32Value{this->div(rhs)};
}

IntegerValue* F32Value::operator<(F32Value* rhs) const
{
    return new IntegerValue{&type_manager.getBasicType(Kind::i32), this->less(rhs)};
}

IntegerValue* F32Value::operator<=(F32Value* rhs) const
{
    return new IntegerValue{&type_manager.getBasicType(Kind::i32), this->lessEqual(rhs)};
}

IntegerValue* F32Value::operator==(F32Value* rhs) const
{
    return new IntegerValue{&type_manager.getBasicType(Kind::i32), this->equal(rhs)};
}

F32Value* F32Value::inplaceNegation()
{
    this->val = this->negation();
    return this;
}

void F32Value::operator+=(F32Value* rhs)
{
    this->val = this->add(rhs);
}

void F32Value::operator*=(F32Value* rhs)
{
    this->val = this->mul(rhs);
}

void F32Value::operator/=(F32Value* rhs)
{
    this->val = this->div(rhs);
}

double F64Value::negation() const
{
    return -this->val;
}

double F64Value::add(F64Value* rhs) const
{
    return this->val + rhs->val;
}

double F64Value::mul(F64Value* rhs) const
{
    return this->val * rhs->val;
}

double F64Value::div(F64Value* rhs) const
{
    if (rhs->isZero()) {
        throw UBException{{UB::exceptional_condition, UB::div_or_mod_zero, UB::div_not_representable}, "Div zero"};
    }
    return this->val / rhs->val;
}

bool F64Value::less(F64Value* rhs) const
{
    return this->val < rhs->val;
}

bool F64Value::lessEqual(F64Value* rhs) const
{
    return this->val <= rhs->val;
}

bool F64Value::equal(F64Value* rhs) const
{
    return this->val == rhs->val;
}

bool F64Value::isZero() const noexcept
{
    return this->val == 0;
}

F64Value* F64Value::operator-() const
{
    return new F64Value{this->negation()};
}

F64Value* F64Value::operator+(F64Value* rhs) const
{
    return new F64Value{this->add(rhs)};
}

F64Value* F64Value::operator*(F64Value* rhs) const
{
    return new F64Value{this->mul(rhs)};
}

F64Value* F64Value::operator/(F64Value* rhs) const
{
    return new F64Value{this->div(rhs)};
}

IntegerValue* F64Value::operator<(F64Value* rhs) const
{
    return new IntegerValue{&type_manager.getBasicType(Kind::i32), this->less(rhs)};
}

IntegerValue* F64Value::operator<=(F64Value* rhs) const
{
    return new IntegerValue{&type_manager.getBasicType(Kind::i32), this->lessEqual(rhs)};
}

IntegerValue* F64Value::operator==(F64Value* rhs) const
{
    return new IntegerValue{&type_manager.getBasicType(Kind::i32), this->equal(rhs)};
}

F64Value* F64Value::inplaceNegation()
{
    this->val = this->negation();
    return this;
}

void F64Value::operator+=(F64Value* rhs)
{
    this->val = this->add(rhs);
}

void F64Value::operator*=(F64Value* rhs)
{
    this->val = this->mul(rhs);
}

void F64Value::operator/=(F64Value* rhs)
{
    this->val = this->div(rhs);
}

bool PointerValue::isValidOffset(const ts::Type* lvalue_type, am::Entity* entity, uint64_t offset)
{
    if (!entity) {
        return offset == 0;
    }
    auto& ref_type = removeQualify(down_cast<const Pointer*>(lvalue_type)->referenced);
    auto& ent_type = removeQualify(entity->effective_type);
    if (ent_type.kind() == Kind::function || ref_type.kind() == Kind::function) {
        return offset == 0;
    }
    if (isCCharacter(ref_type.kind())) {
        return offset <= ent_type.size();
    }
    return offset == 0 || offset == ent_type.size();
}

#ifndef NDEBUG

bool PointerValue::checkInvariant()
{
    if (type->kind() != Kind::pointer) {
        return false;
    }
    if (!this->entity) {
        return this->offset == 0;
    }
    ASSERT((*this->entity)->address < UINT64_MAX - offset, "invalid offset");
    return PointerValue::isValidOffset(this->type, *this->entity, this->offset);
}

#endif

IntegerValue* PointerValue::operator==(PointerValue* rhs) const
{
    return new IntegerValue{&type_manager.getBasicType(Kind::i32), this->getAddress() == rhs->getAddress()};
}

IntegerValue* PointerValue::operator==(NullValue*) const
{
    return new IntegerValue{&type_manager.getBasicType(Kind::i32), this->isZero()};
}

void PointerValue::set(const Type* type)
{
    this->type = type;
    ASSERT(this->checkInvariant(), "invalid type");
}

void PointerValue::set(Entity* ent)
{
    this->entity = ent;
    ASSERT(this->checkInvariant(), "invalid entity");
}

void PointerValue::set(size_t off)
{
    this->offset = off;
    ASSERT(this->checkInvariant(), "invalid offset");
}

void PointerValue::set(Entity* ent, size_t off)
{
    this->entity = ent;
    this->offset = off;
    ASSERT(this->checkInvariant(), "invalid entity or offset");
}

void PointerValue::set(const Type* type, Entity* ent, size_t off)
{
    this->entity = ent;
    this->type = type;
    this->offset = off;
    ASSERT(this->checkInvariant(), "invalid entity or offset");
}

bool PointerValue::isZero() const noexcept
{
    return !this->entity;
}

uint64_t PointerValue::getAddress() const noexcept
{
    if (!this->entity) {
        return 0;
    }
    return (*this->entity)->address + this->offset;
}

IntegerValue* NullValue::operator==(PointerValue* rhs) const
{
    return new IntegerValue{&type_manager.getBasicType(Kind::i32), rhs->isZero()};
}

IntegerValue* NullValue::operator==(NullValue*) const
{
    return new IntegerValue{&type_manager.getBasicType(Kind::i32), true};
}
