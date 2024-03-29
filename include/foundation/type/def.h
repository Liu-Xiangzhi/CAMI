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

#ifndef CAMI_FOUNDATION_TYPE_DEF_H
#define CAMI_FOUNDATION_TYPE_DEF_H

#include <lib/assert.h>
#include <lib/array.h>
#include <lib/downcast.h>
#include <lib/utils.h>
#include <lib/format.h>
#include <cstdint>

namespace cami::ts {
using kind_t = int;
enum class Kind : kind_t
{
    bool_,
    char_,
    i8 = 0b0010, i16 = 0b0011, i32 = 0b0100, i64 = 0b0101,
    u8 = 0b1010, u16 = 0b1011, u32 = 0b1100, u64 = 0b1101,
    f32, f64,
    void_, null,
    _basic_max,
    pointer, array, function,
    struct_, union_,
    qualify,
    ivd, // invalid type
};

constexpr inline bool isInteger(Kind kind)
{
    return kind <= Kind::u64;
}

constexpr inline bool isFloat(Kind kind)
{
    return kind == Kind::f32 || kind == Kind::f64;
}

constexpr inline bool isArithmetic(Kind kind)
{
    return kind <= Kind::f64;
}

#ifndef NDEBUG
#define UNREACHABLE() ASSERT(false, "cannot reach here")
#else
#define UNREACHABLE() std::abort()
#endif

class Type
{
    friend class MemoryManager;

protected:
    Type() = default;
    ~Type() = default;
public:
    Type(const Type&) = delete;
    Type& operator=(const Type&) = delete;
    Type(Type&&) noexcept = delete;
    Type& operator=(Type&&) noexcept = delete;
    [[nodiscard]] virtual Kind kind() const noexcept = 0;

    [[nodiscard]] virtual size_t size() const noexcept
    {
        UNREACHABLE();
    }

    [[nodiscard]] virtual size_t align() const noexcept
    {
        UNREACHABLE();
    }

    // hash and equals is only used by MemoryManager (for convenience, these two function is not tagged as protected)
    [[nodiscard]] virtual bool equals(const Type& that) const noexcept = 0;
    [[nodiscard]] virtual size_t hash() const noexcept = 0;
};

class Basic : public Type
{
    friend class MemoryManager;

protected:
    Kind kd;

    explicit Basic(Kind kind) : kd(kind) {}

    ~Basic() = default;
public:
    [[nodiscard]] Kind kind() const noexcept final
    {
        return this->kd;
    }

    [[nodiscard]] bool equals(const Type& that) const noexcept final
    {
        return that.kind() == this->kd;
    }

    [[nodiscard]] size_t hash() const noexcept final
    {
        return static_cast<size_t>(this->kd);
    }
};

class Arith : public Basic
{
protected:
    explicit Arith(Kind kind) : Basic(kind)
    {
        ASSERT(isArithmetic(kind), "invalid param");
    }

    ~Arith() = default;
};

class Integer : public Arith
{
    friend class MemoryManager;

protected:
    explicit Integer(Kind kind) : Arith(kind)
    {
        ASSERT(isInteger(kind), "invalid param");
    }

    ~Integer() = default;

public:
    [[nodiscard]] size_t size() const noexcept override
    {
        if (this->kd == Kind::char_ || this->kd == Kind::bool_) {
            return 1;
        }
        return 1 << ((static_cast<kind_t>(this->kd) & 7) - 2);
    }

    [[nodiscard]] size_t align() const noexcept override
    {
        return this->size();
    }
};

class Float : public Arith
{
    friend class MemoryManager;

protected:
    explicit Float(Kind kind) : Arith(kind)
    {
        ASSERT(isFloat(kind), "invalid param");
    }

    ~Float() = default;

public:
    [[nodiscard]] size_t size() const noexcept override
    {
        return this->kd == Kind::f32 ? 4 : 8;
    }

    [[nodiscard]] size_t align() const noexcept override
    {
        return this->size();
    }
};

class Derived : public Type
{
};

class Pointer : public Derived
{
    friend class MemoryManager;

public:
    const Type& referenced;

protected:
    explicit Pointer(const Type& referenced) : referenced(referenced)
    {
        ASSERT(referenced.kind() != Kind::null, "invalid reference type");
    }

    ~Pointer() = default;

public:
    [[nodiscard]] Kind kind() const noexcept final
    {
        return Kind::pointer;
    }

    [[nodiscard]] size_t size() const noexcept override
    {
        return 16;
    }

    [[nodiscard]] size_t align() const noexcept override
    {
        return 8;
    }

    [[nodiscard]] bool equals(const Type& that) const noexcept final;
    [[nodiscard]] size_t hash() const noexcept final;
};

class Array : public Derived
{
    friend class MemoryManager;

public:
    const Type& element;
    const size_t len;

protected:
    Array(const Type& element, size_t len) : element(element), len(len)
    {
        ASSERT(this->len > 0, "invalid length");
        ASSERT(element.kind() != Kind::null, "invalid element type");
    }

    ~Array() = default;

public:
    [[nodiscard]] Kind kind() const noexcept final
    {
        return Kind::array;
    }

    [[nodiscard]] size_t size() const noexcept override
    {
        return this->len * this->element.size();
    }

    [[nodiscard]] size_t align() const noexcept override
    {
        return this->element.align();
    }

    [[nodiscard]] bool equals(const Type& _that) const noexcept final;
    [[nodiscard]] size_t hash() const noexcept final;
};

class Function : public Derived
{
    friend class MemoryManager;

public:
    const Type& returned;
    const lib::Array<const Type*> params;

protected:
    Function(const Type& returned, lib::Array<const Type*> params)
            : returned(returned), params(std::move(params)) {}

    ~Function() = default;

public:
    [[nodiscard]] Kind kind() const noexcept final
    {
        return Kind::function;
    }

    [[nodiscard]] bool equals(const Type& _that) const noexcept final;
    [[nodiscard]] size_t hash() const noexcept final;
};

class Struct : public Derived
{
    friend class MemoryManager;

public:
    const std::string name;
    const lib::Array<const Type*> members;

protected:
    Struct(std::string name, lib::Array<const Type*> members)
            : name(std::move(name)), members(std::move(members)) {}

    ~Struct() = default;
public:
    [[nodiscard]] Kind kind() const noexcept final
    {
        return Kind::struct_;
    }

    [[nodiscard]] size_t size() const noexcept override;
    [[nodiscard]] size_t align() const noexcept override;

    [[nodiscard]] bool equals(const Type& that) const noexcept final
    {
        return that.kind() == Kind::struct_ && this->name == down_cast<const Struct&>(that).name;
    }

    [[nodiscard]] size_t hash() const noexcept final
    {
        return std::hash<std::string>{}(this->name);
    }
};

class Union : public Derived
{
    friend class MemoryManager;

public:
    const std::string name;
    const lib::Array<const Type*> members;

protected:
    Union(std::string name, lib::Array<const Type*> members)
            : name(std::move(name)), members(std::move(members)) {}

    ~Union() = default;
public:
    [[nodiscard]] Kind kind() const noexcept final
    {
        return Kind::union_;
    }

    [[nodiscard]] size_t size() const noexcept override;
    [[nodiscard]] size_t align() const noexcept override;

    [[nodiscard]] bool equals(const Type& that) const noexcept final
    {
        return that.kind() == Kind::union_ && this->name == down_cast<const Union&>(that).name;
    }

    [[nodiscard]] size_t hash() const noexcept final
    {
        return std::hash<std::string>{}(this->name);
    }
};

enum class Qualifier
{
    none = 0, volatile_ = 1, const_ = 2, restrict = 4, atomic = 8,
};

inline bool operator&(Qualifier q1, Qualifier q2)
{
    return static_cast<int>(q1) & static_cast<int>(q2);
}

inline Qualifier operator|(Qualifier q1, Qualifier q2)
{
    return static_cast<Qualifier>(static_cast<int>(q1) | static_cast<int>(q2));
}

class Qualify : public Derived
{
    friend class MemoryManager;

public:
    const Type& qualified;
    const Qualifier qualifier;

protected:
    Qualify(const Type& qualified, Qualifier qualifier)
            : qualified(qualified), qualifier(qualifier)
    {
        ASSERT(qualified.kind() != Kind::qualify && qualified.kind() != Kind::function, "invalid qualify derivation");
    }

    ~Qualify() = default;

public:
    [[nodiscard]] Kind kind() const noexcept final
    {
        return Kind::qualify;
    }

    [[nodiscard]] size_t size() const noexcept override
    {
        return this->qualified.size();
    }

    [[nodiscard]] size_t align() const noexcept override
    {
        return this->qualified.align();
    }

    [[nodiscard]] bool equals(const Type& _that) const noexcept final;
    [[nodiscard]] size_t hash() const noexcept final;
};
} // namespace cami::ts

CAMI_DECLARE_FORMATTER(cami::ts::Type);

#define TYPE_FORMATTER(clazz)                                                              \
    template<>                                                                             \
    struct cami::lib::ToString<cami::ts::clazz>                                            \
    {                                                                                      \
        static std::string invoke(const cami::ts::clazz& type, std::string_view specifier) \
        {                                                                                  \
            return ToString<cami::ts::Type>::invoke(type, specifier);                      \
        }                                                                                  \
    }

TYPE_FORMATTER(Basic);

TYPE_FORMATTER(Arith);

TYPE_FORMATTER(Integer);

TYPE_FORMATTER(Float);

TYPE_FORMATTER(Derived);

TYPE_FORMATTER(Pointer);

TYPE_FORMATTER(Array);

TYPE_FORMATTER(Function);

TYPE_FORMATTER(Struct);

TYPE_FORMATTER(Union);

TYPE_FORMATTER(Qualify);

#undef TYPE_FORMATTER
#endif //CAMI_FOUNDATION_TYPE_DEF_H
