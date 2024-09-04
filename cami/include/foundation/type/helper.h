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

#ifndef CAMI_FOUNDATION_TYPE_HELPER_H
#define CAMI_FOUNDATION_TYPE_HELPER_H

#include "def.h"
#include "mm.h"
#include <utility>

namespace cami::ts {
constexpr inline bool isCCharacter(Kind kind)
{
    return kind == Kind::char_ || kind == Kind::i8 || kind == Kind::u8;
}

constexpr inline bool isStrictInteger(Kind kind)
{
    return kind >= Kind::i8 && kind <= Kind::u64;
}

constexpr inline bool isUnsigned(Kind kind)
{
    return static_cast<kind_t>(kind) & 0b1000;
}

constexpr inline bool isSigned(Kind kind)
{
    return !isUnsigned(kind);
}

constexpr inline bool isReal(Kind kind)
{
    return isArithmetic(kind);
}

constexpr inline bool isScalar(Kind kind)
{
    return isArithmetic(kind) || kind == Kind::pointer || kind == Kind::null;
}

constexpr inline bool isBasic(Kind kind)
{
    return kind < Kind::_basic_max;
}

constexpr inline bool isPointerLike(Kind kind)
{
    return kind == Kind::pointer || kind == Kind::null;
}

constexpr inline size_t integerTypeRank(Kind kind)
{
    return static_cast<kind_t>(kind) & 0x7;
}

constexpr inline kind_t correspondingSignedValue(Kind kind)
{
    ASSERT(kind >= Kind::i8 && kind <= Kind::u64, "invalid param");
    return static_cast<kind_t>(kind) & 0x7;
}

constexpr inline kind_t correspondingUnsignedValue(Kind kind)
{
    ASSERT(kind >= Kind::i8 && kind <= Kind::u64, "invalid param");
    return static_cast<kind_t>(kind) | 0x8;
}

constexpr inline Kind correspondingSignedKind(Kind kind)
{
    return static_cast<Kind>(correspondingSignedValue(kind));
}

constexpr inline Kind correspondingUnsignedKind(Kind kind)
{
    return static_cast<Kind>(correspondingUnsignedValue(kind));
}

constexpr inline bool isSameIntegerWithoutSigness(Kind a, Kind b)
{
    ASSERT(isStrictInteger(a) && isStrictInteger(b), "precondition violation");
    return correspondingUnsignedKind(a) == correspondingUnsignedKind(b);
}

inline const Type& removeQualify(const Type& type)
{
    if (type.kind() == Kind::qualify) {
        auto& t = down_cast<const Qualify&>(type).qualified;
        ASSERT(t.kind() != Kind::qualify, "shouldn't qualify derivation from qualify type");
        return t;
    }
    return type;
}

inline std::pair<Qualifier, const Type&> peelQualify(const Type& type)
{
    if (type.kind() == Kind::qualify) {
        auto& t = down_cast<const Qualify&>(type);
        ASSERT(t.qualified.kind() != Kind::qualify, "shouldn't qualify derivation from qualify type");
        return {t.qualifier, t.qualified};
    }
    return {Qualifier::none, type};
}

inline const Type& addQualify(const Type& type, Qualifier qualifier)
{
    return qualifier == Qualifier::none ? type : type_manager.getQualifiedType(type, qualifier);
}

// A is compatible with B is equivalent to A is same as B
//  except the difference in qualifier (and treat `char` as `i8`)
bool isCompatible(const Type& a, const Type& b);
// A is looser compatible with B is equivalent to A is same as B
//  except the difference in qualifier and signedness (and treat `char` and `bool` as `i8`)
bool isLooserCompatible(const Type& a, const Type& b);
// A is the loosest compatible with B equivalent to
//  A is looser compatible B, or
//  A is qualified or unqualified pointer type and B is qualified or unqualified pointer type
bool isLoosestCompatible(const Type& a, const Type& b);

inline bool isAllowed(const Type& lvalue_type, const Type& object_type)
{
    if (isCCharacter(removeQualify(lvalue_type).kind())) {
        return object_type.kind() != Kind::function;
    }
    return isLooserCompatible(lvalue_type, object_type);
}

inline uint64_t getMaxSignedValue(Kind kind)
{
    ASSERT(kind >= Kind::i8 && kind <= Kind::i64, "precondition violation");
    auto exponent = static_cast<kind_t>(kind) - static_cast<kind_t>(Kind::i8) + 3;
    auto width = 1ULL << exponent;
    return (1ULL << (width - 1)) - 1;
}

inline uint64_t getMaxUnsignedValue(Kind kind)
{
    ASSERT(kind >= Kind::u8 && kind <= Kind::u64, "precondition violation");
    auto exponent = static_cast<kind_t>(kind) - static_cast<kind_t>(Kind::u8) + 3;
    auto width = 1ULL << exponent;
    return (1ULL << (width - 1)) + ((1ULL << (width - 1)) - 1);
}

inline uint64_t getMaxValue(Kind kind)
{
    ASSERT(isStrictInteger(kind), "precondition violation");
    return isSigned(kind) ? getMaxSignedValue(kind) : getMaxUnsignedValue(kind);
}

inline int64_t getMinValue(Kind kind)
{
    ASSERT(isStrictInteger(kind), "precondition violation");
    auto max_sign_val = getMaxSignedValue(kind);
    return isSigned(kind) ? -*reinterpret_cast<int64_t*>(&max_sign_val) - 1 : 0;
}

// count the number of instances of all objects generated when object `O` is created as type `type`
//   (i.e. 1(object `O`) + number of sub object `O` + number of sub object of all sub object of `O` + ..., recursively)
uint64_t countCorrespondingObjectFamily(const Type& type);
} // namespace cami::ts

#endif //CAMI_FOUNDATION_TYPE_HELPER_H
