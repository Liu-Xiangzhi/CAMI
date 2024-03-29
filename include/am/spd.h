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

#ifndef CAMI_AM_SPD_H
#define CAMI_AM_SPD_H
// static program data

#include <vector>
#include <tuple>
#include <string>
#include <utility>
#include <lib/array.h>
#include <lib/optional.h>
#include <foundation/value.h>
#include "object.h"
#include "trace_data.h"

namespace cami::am::spd {

namespace detail {
struct Helper
{
    static constexpr bool isNull(uint64_t value) noexcept
    {
        return value == -1;
    }

    static constexpr uint64_t makeNull()
    {
        return -1;
    }
};
} // namespace detail
using u64_opt = lib::Optional<uint64_t, detail::Helper>;

// mapper from abstract machine bytecode address to C source code line
class SourceCodeLocator
{
public:
    struct Item {
        uint64_t addr;
        uint64_t len;
        uint64_t line;
    };
    // modified only by linker
    lib::Array<Item> data;

    SourceCodeLocator() = default;
    explicit SourceCodeLocator(lib::Array<Item> data) : data(std::move(data)) {}

public:
    u64_opt getLine(uint64_t addr)
    {
        auto itr = std::upper_bound(data.begin(), data.end(), addr, [](uint64_t a, const Item& b){
            return a > b.addr;
        });
        if (itr == this->data.begin()) {
            return {};
        }
        --itr;
        auto [bc_addr, bc_len, line] = *itr;
        if (addr >= bc_addr + bc_len) {
            return {};
        }
        return line;
    }
};

struct AutomaticObjectDescription
{
    std::string name;
    size_t id;
    const ts::Type& type;
    uint64_t offset;
    u64_opt init_offset;
};
struct StaticObjectDescription
{
    std::string name;
    const ts::Type* type;
    uint64_t address;
};
struct Block
{
    lib::Array<AutomaticObjectDescription> obj_desc;
};

struct Function : public Entity
{
    std::string file_name;
    size_t frame_size;
    size_t code_size;
    size_t max_object_num;
    lib::Array<Block> blocks;
    lib::Array<FullExprInfo> full_expr_infos; // indexed by full expr id
    SourceCodeLocator func_locator;

    Function(std::string name, const ts::Type& type, uint64_t address, std::string file_name,
             size_t frame_size, size_t code_size, size_t max_object_num, lib::Array<Block> blocks,
             lib::Array<FullExprInfo> full_expr_infos, SourceCodeLocator func_locator)
            : Entity(std::move(name), type, address), file_name(std::move(file_name)), frame_size(frame_size),
              code_size(code_size), max_object_num(max_object_num), blocks(std::move(blocks)),
              full_expr_infos(std::move(full_expr_infos)), func_locator(std::move(func_locator)) {}
};

struct Global
{
    lib::Array<Object*> static_objects; // object_manager has the ownership of item of static_objects
    lib::Array<ValueBox> constants;
    lib::Array<const ts::Type*> types; // used by cast operator
    lib::Array<Function> functions;
    lib::Array<uint8_t> stack_init_data;

    Global(lib::Array<Object*> static_objects, lib::Array<ValueBox> constants,
           lib::Array<const ts::Type*> types, lib::Array<Function> functions,
           lib::Array<uint8_t> stack_init_data)
            : static_objects(std::move(static_objects)), constants(std::move(constants)),
              types(std::move(types)), functions(std::move(functions)),
              stack_init_data(std::move(stack_init_data)) {}

    Global(Global&&) noexcept = default;
    Global& operator=(Global&&) noexcept = delete;

public:
    lib::Optional<Function*> getFunctionByAddress(uint64_t address)
    {
        auto itr = std::lower_bound(this->functions.begin(), this->functions.end(), address,
                                    [](const Function& f, uint64_t addr) { return f.address < addr; });
        if (itr == this->functions.end()) {
            return {};
        }
        if (itr->address > address) {
            if (itr == this->functions.begin()) {
                return {};
            }
            --itr;
        }
        return &*itr;
    }
};

struct InitializeDescription
{
    lib::Array<uint8_t> code;
    lib::Array<uint8_t> data;
    uint64_t string_literal_base;
    lib::Array<StaticObjectDescription> static_objects;
    lib::Array<ValueBox> constants;
    lib::Array<const ts::Type*> types;
    lib::Array<Function> functions;
    lib::Array<uint8_t> stack_init_data;
};

} // namespace cami::am::spd

#endif //CAMI_AM_SPD_H
