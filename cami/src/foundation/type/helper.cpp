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

using namespace cami;
using namespace ts;

bool ts::isCompatible(const Type& _a, const Type& _b) // NOLINT
{
    auto& a = removeQualify(_a);
    auto& b = removeQualify(_b);
    if (isBasic(a.kind()) && isBasic(b.kind())) {
        auto kind_a = a.kind() == Kind::char_ ? Kind::i8 : a.kind();
        auto kind_b = b.kind() == Kind::char_ ? Kind::i8 : b.kind();
        return kind_a == kind_b;
    }
    if (a.kind() != b.kind()) {
        return false;
    }
    switch (a.kind()) {
    case Kind::pointer:
        return isCompatible(down_cast<const Pointer&>(a).referenced, down_cast<const Pointer&>(b).referenced);
    case Kind::array: {
        auto& aa = down_cast<const Array&>(a);
        auto& bb = down_cast<const Array&>(b);
        return aa.len == bb.len && isCompatible(aa.element, bb.element);
    }
    case Kind::function: {
        auto& aa = down_cast<const Function&>(a);
        auto& bb = down_cast<const Function&>(b);
        if (aa.params.length() != bb.params.length()) {
            return false;
        }
        if (!isCompatible(aa.returned, bb.returned)) {
            return false;
        }
        for (size_t i = 0; i < aa.params.length(); ++i) {
            if (!isCompatible(*aa.params[i], *bb.params[i])) {
                return false;
            }
        }
        return true;
    }
    case Kind::struct_:
        return down_cast<const Struct&>(a).name == down_cast<const Struct&>(b).name;
    default:
        ASSERT(a.kind() == Kind::union_, "no other case should happened");
        return down_cast<const Union&>(a).name == down_cast<const Union&>(b).name;
    }
}

bool ts::isLooserCompatible(const Type& _a, const Type& _b) // NOLINT
{
    auto& a = removeQualify(_a);
    auto& b = removeQualify(_b);
    if (isBasic(a.kind()) && isBasic(b.kind())) {
        auto kind_a = a.kind() < Kind::i8 ? Kind::i8 : a.kind();
        auto kind_b = b.kind() < Kind::i8 ? Kind::i8 : b.kind();
        if (isStrictInteger(kind_a) && isStrictInteger(kind_b)) {
            return correspondingUnsignedKind(kind_a) == correspondingUnsignedKind(kind_b);
        }
        return kind_a == kind_b;
    }
    if (a.kind() != b.kind()) {
        return false;
    }
    switch (a.kind()) {
    case Kind::pointer:
        return isLooserCompatible(down_cast<const Pointer&>(a).referenced, down_cast<const Pointer&>(b).referenced);
    case Kind::array: {
        auto& aa = down_cast<const Array&>(a);
        auto& bb = down_cast<const Array&>(b);
        return aa.len == bb.len && isLooserCompatible(aa.element, bb.element);
    }
    case Kind::function: {
        auto& aa = down_cast<const Function&>(a);
        auto& bb = down_cast<const Function&>(b);
        if (aa.params.length() != bb.params.length()) {
            return false;
        }
        if (!isLooserCompatible(aa.returned, bb.returned)) {
            return false;
        }
        for (size_t i = 0; i < aa.params.length(); ++i) {
            if (!isLooserCompatible(*aa.params[i], *bb.params[i])) {
                return false;
            }
        }
        return true;
    }
    case Kind::struct_:
        return down_cast<const Struct&>(a).name == down_cast<const Struct&>(b).name;
    default:
        ASSERT(a.kind() == Kind::union_, "no other case should happened");
        return down_cast<const Union&>(a).name == down_cast<const Union&>(b).name;
    }
}

bool ts::isLoosestCompatible(const Type& _a, const Type& _b) // NOLINT
{
    auto& a = removeQualify(_a);
    auto& b = removeQualify(_b);
    if (isScalar(a.kind()) && isScalar(b.kind())) {
        auto kind_a = a.kind() < Kind::i8 ? Kind::i8 : a.kind();
        auto kind_b = b.kind() < Kind::i8 ? Kind::i8 : b.kind();
        if (isStrictInteger(kind_a) && isStrictInteger(kind_b)) {
            return correspondingUnsignedKind(kind_a) == correspondingUnsignedKind(kind_b);
        }
        return kind_a == kind_b;
    }
    if (a.kind() != b.kind()) {
        return false;
    }
    switch (a.kind()) {
    case Kind::array: {
        auto& aa = down_cast<const Array&>(a);
        auto& bb = down_cast<const Array&>(b);
        return aa.len == bb.len && isLooserCompatible(aa.element, bb.element);
    }
    case Kind::function: {
        auto& aa = down_cast<const Function&>(a);
        auto& bb = down_cast<const Function&>(b);
        if (aa.params.length() != bb.params.length()) {
            return false;
        }
        if (!isLooserCompatible(aa.returned, bb.returned)) {
            return false;
        }
        for (size_t i = 0; i < aa.params.length(); ++i) {
            if (!isLooserCompatible(*aa.params[i], *bb.params[i])) {
                return false;
            }
        }
        return true;
    }
    case Kind::struct_:
        return down_cast<const Struct&>(a).name == down_cast<const Struct&>(b).name;
    default:
        ASSERT(a.kind() == Kind::union_, "no other case should happened");
        return down_cast<const Union&>(a).name == down_cast<const Union&>(b).name;
    }
}

uint64_t ts::countCorrespondingObjectFamily(const Type& _type) // NOLINT
{
    auto& type = removeQualify(_type);
    if (isScalar(type.kind())) {
        return 1;
    }
    if (type.kind() == Kind::array) {
        auto& t = down_cast<const Array&>(type);
        return 1 + t.len * countCorrespondingObjectFamily(t.element);
    }
    if (type.kind() == Kind::struct_) {
        auto& t = down_cast<const Struct&>(type);
        uint64_t sum = 0;
        for (const Type* item: t.members) {
            sum += countCorrespondingObjectFamily(*item);
        }
        return 1 + sum;
    }
    ASSERT(type.kind() == Kind::union_, "invalid object type");
    auto& t = down_cast<const Union&>(type);
    uint64_t sum = 0;
    for (const Type* item: t.members) {
        sum += countCorrespondingObjectFamily(*item);
    }
    return 1 + sum;
}