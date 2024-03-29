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

#ifndef CAMI_AM_RUNTIME_ENV_H
#define CAMI_AM_RUNTIME_ENV_H

#include <cstdint>
#include <stack>
#include <map>
#include <utility>
#include <lib/array.h>
#include <lib/list.h>
#include <lib/format.h>
#include <lib/compiler_guarantee.h>
#include "object.h"
#include <foundation/value.h>
#include "vmm.h"
#include "spd.h"
#include "trace.h"

namespace cami::am::state {

struct Function
{
    spd::Function* static_info;
    uint64_t return_address;
    lib::Array<Object*> automatic_objects;
    std::stack<uint64_t> blocks{};
    TraceContext& context;
    uint32_t cur_full_expr_id = 0;
    uint64_t full_expr_exec_cnt = 0;

    Function(spd::Function* static_info, uint64_t return_address, size_t max_object_num, TraceContext& context)
            : static_info(static_info), return_address(return_address), automatic_objects(max_object_num),
              context(context)
    {
        for (auto& item: this->automatic_objects) {
            item = nullptr;
        }
    }

    ~Function()
    {
        this->context.release();
    }
};

struct Global
{
    uint64_t pc = layout::CODE_BASE;
    uint64_t frame_pointer = layout::STACK_BOUNDARY;
    std::deque<Function> call_stack{};
    // all top objects and functions(used by indirectly access i.e. integer => pointer)
    std::map<uint64_t, Entity*> entities{};

    explicit Global(const Function& boot_function)
    {
        this->call_stack.push_back(boot_function);
    }

    Function& current_function() CAMI_NOEXCEPT
    {
        COMPILER_GUARANTEE(!this->call_stack.empty(), "read empty call stack");
        return this->call_stack.back();
    }
};

} // namespace cami::am::state

CAMI_DECLARE_FORMATTER(cami::am::state::Function);

CAMI_DECLARE_FORMATTER(cami::am::state::Global);

#endif //CAMI_AM_RUNTIME_ENV_H
