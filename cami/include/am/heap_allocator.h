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

#ifndef CAMI_AM_HEAP_ALLOCATOR_H
#define CAMI_AM_HEAP_ALLOCATOR_H

#include <cstdint>
#include "vmm.h"
#include <lib/format.h>

namespace cami::am {

class HeapAllocator
{
protected:
    VirtualMemory& memory;
public:
    explicit HeapAllocator(VirtualMemory& memory) : memory(memory) {}

    virtual uint64_t alloc(uint64_t size, uint64_t align) = 0;
    virtual void dealloc(uint64_t addr, uint64_t size) = 0;
    virtual ~HeapAllocator() = default;

    [[nodiscard]] const VirtualMemory& getMemory() const noexcept
    {
        return this->memory;
    }
};

class SimpleAllocator : public HeapAllocator
{
public:
    explicit SimpleAllocator(VirtualMemory& memory) : HeapAllocator(memory)
    {
        this->memory.write64(layout::HEAP_BASE, layout::HEAP_BOUNDARY - layout::HEAP_BASE);
        this->memory.write64(layout::HEAP_BOUNDARY - 8, layout::HEAP_BOUNDARY - layout::HEAP_BASE);
    }

public:
    uint64_t alloc(uint64_t size, uint64_t align) override;
    void dealloc(uint64_t addr, uint64_t size) override;
private:
    uint64_t findNextAvailable(uint64_t addr);
};

} // namespace cami::am

CAMI_DECLARE_FORMATTER(cami::am::SimpleAllocator);

#endif //CAMI_AM_HEAP_ALLOCATOR_H
