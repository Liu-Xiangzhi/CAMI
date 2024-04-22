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

#ifndef CAMI_AM_AM_H
#define CAMI_AM_AM_H

#include <config.h>
#include "state.h"
#include "spd.h"
#include "evaluation.h"
#include "vmm.h"
#include "obj_man.h"
#include "heap_allocator.h"
#include "object.h"
#include <lib/format.h>
#include <translate/bytecode.h>
#include <memory>

namespace cami::am {

struct DesignationRegister
{
    Entity* entity = nullptr;
    uint64_t offset = 0;
    const ts::Type* lvalue_type = nullptr;
};

class AbstractMachine
{
    OperandStack operand_stack{};
    DesignationRegister dsg_reg{};
    state::Global state;
    ObjectManager object_manager;
    VirtualMemory memory;
    std::unique_ptr<HeapAllocator> heap_allocator;
    spd::Global static_info;

    friend class Execute;

    friend class FetchDecode;

    friend class ObjectManager;

    friend class VirtualMemory;

    friend class Trace;

    friend class Formatter;

private:
    explicit AbstractMachine(tr::LinkedMBC& bytecode)
            : state({bytecode.attribute.entry, 0, 0, TraceContext::dummy}),
              object_manager(*this, AbstractMachine::countPermanentObject(bytecode)),
              memory(std::move(bytecode.code), std::move(bytecode.data), bytecode.string_literal_len, this->object_manager),
              heap_allocator(new ::CAMI_MEMORY_HEAP_ALLOCATOR{this->memory}),
              static_info(std::move(this->initStaticInfo(bytecode))) {}

public:
    explicit AbstractMachine(std::unique_ptr<tr::LinkedMBC> bytecode)
            : AbstractMachine(AbstractMachine::preprocessBytecode(*bytecode)) {}

public:
    enum class ExitCode
    {
        halt, abort, exception
    };
    ExitCode run();
    void execute();
private:
    [[nodiscard]] bool isValidEntityAddress(uint64_t addr) const noexcept
    {
        auto func_addr = reinterpret_cast<uintptr_t>(this->static_info.functions.data());
        auto func_end = reinterpret_cast<uintptr_t>(this->static_info.functions.data() + this->static_info.functions.length());
        return addr == 0 /*nullptr*/ || this->object_manager.isValidObjectAddress(addr) ||
               (addr >= func_addr && addr < func_end && (addr - func_addr) % sizeof(spd::Function) == 0);
    };
    static void checkMetadataCnt(tr::LinkedMBC& bytecode);
    static tr::LinkedMBC& preprocessBytecode(tr::LinkedMBC& bytecode);
    spd::Global initStaticInfo(tr::LinkedMBC& bytecode);
    static uint64_t countPermanentObject(tr::LinkedMBC& bytecode);
};

} // namespace cami::am

CAMI_DECLARE_FORMATTER(cami::am::AbstractMachine);

#endif //CAMI_AM_AM_H
