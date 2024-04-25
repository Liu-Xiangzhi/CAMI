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

#ifndef CAMI_FOUNDATION_VALUE_H
#define CAMI_FOUNDATION_VALUE_H

#include <foundation/type/def.h>
#include <foundation/type/mm.h>
#include <am/object.h>
#include <lib/optional.h>
#include <lib/downcast.h>
#include <lib/format.h>

namespace cami {
class ValueBox;
namespace am {
ValueBox operator+(ValueBox lhs, ValueBox rhs);
ValueBox operator-(ValueBox lhs, ValueBox rhs);
ValueBox operator*(ValueBox lhs, ValueBox rhs);
ValueBox operator/(ValueBox lhs, ValueBox rhs);
ValueBox operator%(ValueBox lhs, ValueBox rhs);
ValueBox operator<<(ValueBox lhs, ValueBox rhs);
ValueBox operator>>(ValueBox lhs, ValueBox rhs);
ValueBox operator<(ValueBox lhs, ValueBox rhs);
ValueBox operator<=(ValueBox lhs, ValueBox rhs);
ValueBox operator>(ValueBox lhs, ValueBox rhs);
ValueBox operator>=(ValueBox lhs, ValueBox rhs);
ValueBox operator==(ValueBox lhs, ValueBox rhs);
ValueBox operator!=(ValueBox lhs, ValueBox rhs);
ValueBox operator&(ValueBox lhs, ValueBox rhs);
ValueBox operator|(ValueBox lhs, ValueBox rhs);
ValueBox operator^(ValueBox lhs, ValueBox rhs);
void operator+=(ValueBox& lhs, ValueBox rhs);
void operator-=(ValueBox& lhs, ValueBox rhs);
void operator*=(ValueBox& lhs, ValueBox rhs);
void operator/=(ValueBox& lhs, ValueBox rhs);
void operator%=(ValueBox& lhs, ValueBox rhs);
void operator<<=(ValueBox& lhs, ValueBox rhs);
void operator>>=(ValueBox& lhs, ValueBox rhs);
void operator&=(ValueBox& lhs, ValueBox rhs);
void operator|=(ValueBox& lhs, ValueBox rhs);
void operator^=(ValueBox& lhs, ValueBox rhs);
}
#define DECLARE_FRIEND                                        \
    friend class ValueBox;                                    \
    friend ValueBox am::operator+(ValueBox lhs, ValueBox rhs);\
    friend ValueBox am::operator-(ValueBox lhs, ValueBox rhs);\
    friend ValueBox am::operator*(ValueBox lhs, ValueBox rhs);\
    friend ValueBox am::operator/(ValueBox lhs, ValueBox rhs);\
    friend ValueBox am::operator%(ValueBox lhs, ValueBox rhs);\
    friend ValueBox am::operator<<(ValueBox lhs, ValueBox rhs);\
    friend ValueBox am::operator>>(ValueBox lhs, ValueBox rhs);\
    friend ValueBox am::operator<(ValueBox lhs, ValueBox rhs);\
    friend ValueBox am::operator<=(ValueBox lhs, ValueBox rhs);\
    friend ValueBox am::operator>(ValueBox lhs, ValueBox rhs);\
    friend ValueBox am::operator>=(ValueBox lhs, ValueBox rhs);\
    friend ValueBox am::operator==(ValueBox lhs, ValueBox rhs);\
    friend ValueBox am::operator!=(ValueBox lhs, ValueBox rhs);\
    friend ValueBox am::operator&(ValueBox lhs, ValueBox rhs);\
    friend ValueBox am::operator|(ValueBox lhs, ValueBox rhs);\
    friend ValueBox am::operator^(ValueBox lhs, ValueBox rhs);\
    friend void am::operator+=(ValueBox& lhs, ValueBox rhs);\
    friend void am::operator-=(ValueBox& lhs, ValueBox rhs);\
    friend void am::operator*=(ValueBox& lhs, ValueBox rhs);\
    friend void am::operator/=(ValueBox& lhs, ValueBox rhs);\
    friend void am::operator%=(ValueBox& lhs, ValueBox rhs);\
    friend void am::operator<<=(ValueBox& lhs, ValueBox rhs);\
    friend void am::operator>>=(ValueBox& lhs, ValueBox rhs);\
    friend void am::operator&=(ValueBox& lhs, ValueBox rhs);\
    friend void am::operator|=(ValueBox& lhs, ValueBox rhs);\
    friend void am::operator^=(ValueBox& lhs, ValueBox rhs)

class NullValue;

class Value
{
protected:
    const ts::Type* type;

    DECLARE_FRIEND;

public:
    explicit Value(const ts::Type* type) : type(type) {}

    DEBUG_VIRTUAL ~Value() = default;

    [[nodiscard]] const ts::Type& getType() const noexcept
    {
        return *this->type;
    }
};

class IntegerValue : public Value
{
    uint64_t val;
public:
    IntegerValue(const ts::Type* type, uint64_t val) : Value(type), val(this->extend(val))
    {
        ASSERT(ts::isInteger(type->kind()), "invalid type");
    }

    explicit IntegerValue(bool val) : Value(&ts::type_manager.getBasicType(ts::Kind::bool_)), val(val) {}

    DECLARE_FRIEND;
public:
    [[nodiscard]] uint64_t uint64() const noexcept
    {
        return this->val;
    }

private:
    IntegerValue* operator~() const;
    IntegerValue* operator-() const;
    IntegerValue* operator+(IntegerValue* rhs) const;
    IntegerValue* operator*(IntegerValue* rhs) const;
    IntegerValue* operator/(IntegerValue* rhs) const;
    IntegerValue* operator%(IntegerValue* rhs) const;
    IntegerValue* operator<(IntegerValue* rhs) const;
    IntegerValue* operator<=(IntegerValue* rhs) const;
    IntegerValue* operator==(IntegerValue* rhs) const;
    IntegerValue* operator&(IntegerValue* rhs) const;
    IntegerValue* operator|(IntegerValue* rhs) const;
    IntegerValue* operator^(IntegerValue* rhs) const;
    IntegerValue* operator<<(IntegerValue* rhs) const;
    IntegerValue* operator>>(IntegerValue* rhs) const;
    IntegerValue* inplaceComplement();
    IntegerValue* inplaceNegation();
    void operator+=(IntegerValue* rhs);
    void operator*=(IntegerValue* rhs);
    void operator/=(IntegerValue* rhs);
    void operator%=(IntegerValue* rhs);
    void operator&=(IntegerValue* rhs);
    void operator|=(IntegerValue* rhs);
    void operator^=(IntegerValue* rhs);
    void operator<<=(IntegerValue* rhs);
    void operator>>=(IntegerValue* rhs);
    [[nodiscard]] bool isZero() const noexcept;
private:
    [[nodiscard]] uint64_t complement() const;
    [[nodiscard]] uint64_t negation() const;
    uint64_t add(IntegerValue* rhs) const;
    uint64_t mul(IntegerValue* rhs) const;
    uint64_t div(IntegerValue* rhs) const;
    uint64_t mod(IntegerValue* rhs) const;
    bool less(IntegerValue* rhs) const;
    bool lessEqual(IntegerValue* rhs) const;
    bool equal(IntegerValue* rhs) const;
    uint64_t bitwiseAnd(IntegerValue* rhs) const;
    uint64_t bitwiseOr(IntegerValue* rhs) const;
    uint64_t bitwiseXor(IntegerValue* rhs) const;
    uint64_t leftShift(IntegerValue* rhs) const;
    uint64_t rightShift(IntegerValue* rhs) const;
    [[nodiscard]] uint64_t extend(uint64_t value) const noexcept;
};

class F32Value : public Value
{
    float val;
public:
    explicit F32Value(float val) : Value(&ts::type_manager.getBasicType(ts::Kind::f32)), val(val) {}

    DECLARE_FRIEND;
public:
    [[nodiscard]] float f32() const noexcept
    {
        return this->val;
    }

private:
    F32Value* operator-() const;
    F32Value* operator+(F32Value* rhs) const;
    F32Value* operator*(F32Value* rhs) const;
    F32Value* operator/(F32Value* rhs) const;
    IntegerValue* operator<(F32Value* rhs) const;
    IntegerValue* operator<=(F32Value* rhs) const;
    IntegerValue* operator==(F32Value* rhs) const;
    F32Value* inplaceNegation();
    void operator+=(F32Value* rhs);
    void operator*=(F32Value* rhs);
    void operator/=(F32Value* rhs);
    [[nodiscard]] bool isZero() const noexcept;
private:
    [[nodiscard]] float negation() const;
    float add(F32Value* rhs) const;
    float mul(F32Value* rhs) const;
    float div(F32Value* rhs) const;
    bool less(F32Value* rhs) const;
    bool lessEqual(F32Value* rhs) const;
    bool equal(F32Value* rhs) const;
};

class F64Value : public Value
{
    double val;
public:
    explicit F64Value(double val) : Value(&ts::type_manager.getBasicType(ts::Kind::f64)), val(val) {}

    DECLARE_FRIEND;
public:
    [[nodiscard]] double f64() const noexcept
    {
        return this->val;
    }

private:
    F64Value* operator-() const;
    F64Value* operator+(F64Value* rhs) const;
    F64Value* operator*(F64Value* rhs) const;
    F64Value* operator/(F64Value* rhs) const;
    IntegerValue* operator<(F64Value* rhs) const;
    IntegerValue* operator<=(F64Value* rhs) const;
    IntegerValue* operator==(F64Value* rhs) const;
    F64Value* inplaceNegation();
    void operator+=(F64Value* rhs);
    void operator*=(F64Value* rhs);
    void operator/=(F64Value* rhs);
    [[nodiscard]] bool isZero() const noexcept;
private:
    [[nodiscard]] double negation() const;
    double add(F64Value* rhs) const;
    double mul(F64Value* rhs) const;
    double div(F64Value* rhs) const;
    bool less(F64Value* rhs) const;
    bool lessEqual(F64Value* rhs) const;
    bool equal(F64Value* rhs) const;
};

class PointerValue : public Value
{
    lib::Optional<am::Entity*> entity;
    size_t offset;

    friend class ValueBox;

public:
    PointerValue(const ts::Type* type, am::Entity* entity, size_t offset)
            : Value(type), entity(entity), offset(offset)
    {
        ASSERT(this->checkInvariant(), "invalid param");
    }

    IntegerValue* operator==(PointerValue* rhs) const;
    IntegerValue* operator==(NullValue* rhs) const;
    [[nodiscard]] bool isZero() const noexcept;
    [[nodiscard]] uint64_t getAddress() const noexcept;

    [[nodiscard]] lib::Optional<am::Entity*> getReferenced() const noexcept
    {
        return this->entity;
    }

    [[nodiscard]] size_t getOffset() const noexcept
    {
        return this->offset;
    }

    void set(const ts::Type* type);
    void set(am::Entity* entity);
    void set(size_t offset);
    void set(am::Entity* entity, size_t offset);
    void set(const ts::Type* type, am::Entity* entity, size_t offset);
    static bool isValidOffset(const ts::Type* lvalue_type, am::Entity* entity, uint64_t offset);
private:
#ifndef NDEBUG
    bool checkInvariant();
#endif
};

class DissociativePointerValue : public Value
{
public:
    const ts::Type* pointer_type;
    uint64_t address;

    DissociativePointerValue(const ts::Type* type, uint64_t address)
            : Value(&ts::type_manager.getDissociativePointer()), pointer_type(type), address(address)
    {
        ASSERT(type->kind() == ts::Kind::pointer, "invalid type");
    }
};

class StructOrUnionValue : public Value
{
public:
    am::Object* obj;

    StructOrUnionValue(const ts::Type* type, am::Object* obj) : Value(type), obj(obj)
    {
        ASSERT(type->kind() == ts::Kind::struct_ || type->kind() == ts::Kind::union_, "invalid type");
    }
};

class NullValue : public Value
{
public:
    NullValue() : Value(&ts::type_manager.getBasicType(ts::Kind::null)) {}

    IntegerValue* operator==(PointerValue* rhs) const;
    IntegerValue* operator==(NullValue* rhs) const;
};

class UndefinedValue : public Value
{
public:
    UndefinedValue() : Value(&ts::type_manager.getInvalid()) {}
};

class ValueBox
{
    Value* value;
public:
    explicit ValueBox(Value* value) : value(value) {}

    ValueBox(ValueBox&& that) noexcept : value(std::exchange(that.value, nullptr)) {};

    ValueBox& operator=(ValueBox&& that) noexcept
    {
        if (&that != this) [[likely]] {
            this->~ValueBox();
            this->value = that.value;
            that.value = nullptr;
        }
        return *this;
    }

    ValueBox(const ValueBox& that) : value(ValueBox::copy(that.value)) {}

    ValueBox& operator=(const ValueBox& that)
    {
        if (this != &that) [[likely]] {
            this->~ValueBox();
            this->value = ValueBox::copy(that.value);
        }
        return *this;
    }

    ~ValueBox()
    {
        if (this->value != nullptr) {
            ValueBox::delete_(this->value);
        }
    }

public:
    // convert arithmetic types only
    void castTo(const ts::Type& type);
    ValueBox cast(const ts::Type& type);

    Value& operator*() const noexcept
    {
        return *this->value;
    }

    Value* operator->() const noexcept
    {
        return this->value;
    }

    ValueBox operator+() const;
    ValueBox operator-() const;
    ValueBox operator~() const;
    ValueBox operator!() const;
    void inplacePositive();
    void inplaceNegation();
    void inplaceComplement();
    [[nodiscard]] bool isZero() const;

    explicit operator Value*() const noexcept
    {
        return this->value;
    }

    explicit operator IntegerValue*() const noexcept
    {
        return down_cast<IntegerValue*>(this->value);
    }

    explicit operator F32Value*() const noexcept
    {
        return down_cast<F32Value*>(this->value);
    }

    explicit operator F64Value*() const noexcept
    {
        return down_cast<F64Value*>(this->value);
    }

    explicit operator PointerValue*() const noexcept
    {
        return down_cast<PointerValue*>(this->value);
    }

    explicit operator DissociativePointerValue*() const noexcept
    {
        return down_cast<DissociativePointerValue*>(this->value);
    }

    explicit operator StructOrUnionValue*() const noexcept
    {
        return down_cast<StructOrUnionValue*>(this->value);
    }

    explicit operator NullValue*() const noexcept
    {
        return down_cast<NullValue*>(this->value);
    }

    template<typename T>
    T& get() const noexcept
    {
        return *static_cast<T*>(*this);
    }

private:
    static void integerPromote(Value* v);
    static void toFloat_uac(ValueBox& lhs, ValueBox& rhs);
    static void delete_(Value* value);
    static Value* copy(Value* value);
public:
    static void integerPromote(const ValueBox& vb)
    {
        integerPromote(vb.value);
    }

    static void usualArithmeticConvert(ValueBox& lhs, ValueBox& rhs);
};

} // namespace cami

#undef DECLARE_FRIEND

CAMI_DECLARE_FORMATTER(cami::IntegerValue);

CAMI_DECLARE_FORMATTER(cami::F32Value);

CAMI_DECLARE_FORMATTER(cami::F64Value);

CAMI_DECLARE_FORMATTER(cami::PointerValue);

CAMI_DECLARE_FORMATTER(cami::StructOrUnionValue);

CAMI_DECLARE_FORMATTER(cami::NullValue);

CAMI_DECLARE_FORMATTER(cami::DissociativePointerValue);

CAMI_DECLARE_FORMATTER(cami::UndefinedValue);

CAMI_DECLARE_FORMATTER(cami::ValueBox);

#endif //CAMI_FOUNDATION_VALUE_H
