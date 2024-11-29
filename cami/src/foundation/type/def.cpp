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

#include <type/def.h>

using namespace cami;
using namespace ts;

namespace {
uint32_t fmix32(uint32_t h)
{
    constexpr uint32_t FMIX1 = 0x85eb'ca6b;
    constexpr uint32_t FMIX2 = 0xc2b2'ae35;
    h ^= h >> 16;
    h *= FMIX1;
    h ^= h >> 13;
    h *= FMIX2;
    h ^= h >> 16;
    return h;
}

uint32_t murmurhash3(const lib::Array<uint32_t>& key, uint32_t seed)
{
    constexpr uint32_t C1 = 0xcc9e'2d51;
    constexpr uint32_t C2 = 0x1b87'3593;
    constexpr uint32_t D = 0xe654'6b64;
    auto h = seed;
    for (auto k: key) {
        k *= C1;
        k = k << 15 | k >> 17;
        k *= C2;
        h ^= k;
        h = h << 13 | h >> 19;
        h = h * 5 + D;
    }
    return fmix32(h ^ key.length());
}
} // anonymous namespace

bool Pointer::equals(const ts::Type& that) const noexcept
{
    return that.kind() == Kind::pointer && this->referenced.equals(down_cast<const Pointer&>(that).referenced);
}

size_t Pointer::hash() const noexcept
{
    return murmurhash3({static_cast<uint32_t>(this->referenced.hash())}, 0x1234);
}

bool Array::equals(const ts::Type& _that) const noexcept
{
    if (_that.kind() != Kind::array) {
        return false;
    }
    auto& that = down_cast<const Array&>(_that);
    return this->len == that.len && this->element.equals(that.element);
}

size_t Array::hash() const noexcept
{
    return murmurhash3({static_cast<uint32_t>(this->element.hash()), static_cast<uint32_t>(this->len >> 32),
                        static_cast<uint32_t>(this->len)},
                       0x5678);
}

bool Function::equals(const ts::Type& _that) const noexcept
{
    if (_that.kind() != Kind::function) {
        return false;
    }
    auto& that = down_cast<const Function&>(_that);
    if (this->params.length() != that.params.length()) {
        return false;
    }
    for (size_t i = 0; i < this->params.length(); ++i) {
        if (!this->params[i]->equals(*that.params[i])) {
            return false;
        }
    }
    return this->returned.equals(that.returned);
}

size_t Function::hash() const noexcept
{
    lib::Array<uint32_t> hashes(1 + this->params.length());
    for (size_t i = 0; i < this->params.length(); ++i) {
        hashes[i] = this->params[i]->hash();
    }
    hashes[this->params.length()] = this->returned.hash();
    return murmurhash3(hashes, 0x9abc);
}

size_t Struct::size() const noexcept
{
    size_t offset = 0;
    for (const Type* item: this->members) {
        offset = lib::roundUp(offset, item->align()) + item->size();
    }
    return lib::roundUp(offset, this->align());
}

size_t Struct::align() const noexcept
{
    size_t max = 1;
    for (const Type* item: this->members) {
        auto align = item->align();
        if (align > max) {
            max = align;
        }
    }
    return max;
}

size_t Union::size() const noexcept
{
    size_t max = 0;
    for (const Type* item: this->members) {
        auto sz = item->size();
        if (sz > max) {
            max = sz;
        }
    }
    return max;
}

size_t Union::align() const noexcept
{
    size_t max = 1;
    for (const Type* item: this->members) {
        auto align = item->align();
        if (align > max) {
            max = align;
        }
    }
    return max;
}

bool Qualify::equals(const ts::Type& _that) const noexcept
{
    if (_that.kind() != Kind::qualify) {
        return false;
    }
    auto& that = down_cast<const Qualify&>(_that);
    return this->qualifier == that.qualifier && this->qualified.equals(that.qualified);
}

size_t Qualify::hash() const noexcept
{
    return murmurhash3({static_cast<uint32_t>(this->qualified.hash()), static_cast<uint32_t>(this->qualifier)},
                       0xdef0);
}
