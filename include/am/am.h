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

public:
    explicit AbstractMachine(spd::InitializeDescription&& desc)
            : state({&desc.functions.back(), 0, 0, TraceContext::dummy}),
              object_manager(*this, AbstractMachine::countPermanentObject(desc)),
              memory(std::move(desc.code), std::move(desc.data), desc.string_literal_base, this->object_manager),
              heap_allocator(new ::CAMI_MEMORY_HEAP_ALLOCATOR{this->memory}),
              static_info(std::move(this->initStaticInfo(desc))) {}

public:
    enum class ExitCode
    {
        halt, abort, exception
    };
    ExitCode run();
    void execute();
private:
    spd::Global initStaticInfo(spd::InitializeDescription& desc);
    static uint64_t countPermanentObject(spd::InitializeDescription& desc);
};

} // namespace cami::am

CAMI_DECLARE_FORMATTER(cami::am::AbstractMachine);

#endif //CAMI_AM_AM_H
