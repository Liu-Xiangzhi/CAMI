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

#ifndef CAMI_TRANSLATE_LINKER_H
#define CAMI_TRANSLATE_LINKER_H

#include "bytecode.h"
#include <lib/array.h>
#include <am/spd.h>
#include <stdexcept>

namespace cami::tr {

struct LinkOption
{
    MBC::Type type;
};

class Linker
{
public:
    static std::unique_ptr<MBC> link(const lib::Array<const MBC*>& mbcs, const LinkOption& option);
    static am::spd::InitializeDescription spawn(std::unique_ptr<MBC> mbc);
};


template<typename T>
struct Traverser
{
    const MBC* const* mbcs;
    uint64_t mbcs_len;
    std::function<T&(const MBC*)> field_getter;

    Traverser(const lib::Array<const MBC*>& mbcs, const std::function<T&(const MBC*)>& field_getter)
            : mbcs(mbcs.data()), mbcs_len(mbcs.length()), field_getter(field_getter) {}

    Traverser(const MBC* const* mbcs, uint64_t mbcs_len, const std::function<T&(const MBC*)>& field_getter)
            : mbcs(mbcs), mbcs_len(mbcs_len), field_getter(field_getter) {}

    struct Iterator
    {
        const MBC* const* ptr;
        const std::function<T&(const MBC*)>& field_getter;

        Iterator(const MBC* const* ptr, const std::function<T&(const MBC*)>& field_getter)
                : ptr(ptr), field_getter(field_getter) {}

        Iterator& operator++()
        {
            this->ptr++;
            return *this;
        }

        Iterator& operator--()
        {
            this->ptr--;
            return *this;
        }

        Iterator operator++(int) // NOLINT
        {
            auto p = this->ptr;
            this->ptr++;
            return Iterator{p, this->field_getter};
        }

        Iterator operator--(int) // NOLINT
        {
            auto p = this->ptr;
            this->ptr--;
            return Iterator{p, this->field_getter};
        }

        T& operator*() const noexcept
        {
            return this->field_getter(*this->ptr);
        }

        bool operator==(const Iterator& rhs) const noexcept
        {
            return this->ptr == rhs.ptr;
        }

        bool operator!=(const Iterator& rhs) const noexcept
        {
            return this->ptr != rhs.ptr;
        }
    };

    [[nodiscard]] Iterator begin() const noexcept
    {
        return {this->mbcs, this->field_getter};
    }

    [[nodiscard]] Iterator end() const noexcept
    {
        return {this->mbcs + this->mbcs_len, this->field_getter};
    }
};

template<class T>
Traverser(const lib::Array<const MBC*>& mbcs, const T& field_getter)
-> Traverser<std::remove_reference_t<std::invoke_result_t<T, const MBC*>>>;

template<class T>
Traverser(const MBC* const* mbcs, uint64_t len, const T& field_getter)
-> Traverser<std::remove_reference_t<std::invoke_result_t<T, const MBC*>>>;

struct DataMergeResult
{
    MBC::Data data;
    lib::Array<uint64_t> bin_offsets;
    lib::Array<uint64_t> object_cnt;

    DataMergeResult(MBC::Data data, lib::Array<uint64_t> bin_offsets, lib::Array<uint64_t> object_cnt) :
            data(std::move(data)), bin_offsets(std::move(bin_offsets)), object_cnt(std::move(object_cnt)) {}
};

struct BssMergeResult
{
    MBC::BSS bss;
    lib::Array<uint64_t> bin_offsets;
    lib::Array<uint64_t> object_cnt;

    BssMergeResult(MBC::BSS bss, lib::Array<uint64_t> bin_offsets, lib::Array<uint64_t> object_cnt) :
            bss(std::move(bss)), bin_offsets(std::move(bin_offsets)), object_cnt(std::move(object_cnt)) {}
};

struct CodeMergeResult
{
    MBC::Code code;
    lib::Array<uint64_t> bin_offs;
    lib::Array<uint64_t> function_cnt;
    lib::Array<uint64_t> relocate_cnt;

    CodeMergeResult(MBC::Code code, lib::Array<uint64_t> bin_offs, lib::Array<uint64_t> function_cnt,
                    lib::Array<uint64_t> relocate_cnt) :
            code(std::move(code)), bin_offs(std::move(bin_offs)),
            function_cnt(std::move(function_cnt)), relocate_cnt(std::move(relocate_cnt)) {}
};

struct RelocateMapper
{
    std::map<const ts::Type*, uint64_t> type;
    std::map<std::pair<const ts::Type*, uint64_t>, uint64_t> constant;
    std::map<std::string_view, uint64_t> identifier;
};
} // namespace cami::tr

#endif //CAMI_TRANSLATE_LINKER_H
