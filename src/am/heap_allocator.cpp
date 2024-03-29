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

#include <heap_allocator.h>

using namespace cami;
using namespace am;

uint64_t SimpleAllocator::alloc(uint64_t size, uint64_t align)
{
    uint64_t addr = layout::HEAP_BASE;
    uint64_t len;
    uint64_t alloc_len;
    do {
        addr = this->findNextAvailable(addr);
        if (addr == -1) {
            return -1;
        }
        len = this->memory.read64(addr);
        alloc_len = 16 + lib::roundUp(size, 2) + lib::roundUp(addr + 8, align);
    } while (alloc_len > len);
    if (alloc_len < len - 16) {
        this->memory.write64(addr, alloc_len | 1);
        this->memory.write64(addr + alloc_len - 8, alloc_len | 1);
        this->memory.write64(addr + alloc_len, len - alloc_len);
        this->memory.write64(addr + len - 8, len - alloc_len);
    } else {
        this->memory.write64(addr, len | 1);
        this->memory.write64(addr + len - 8, len | 1);
    }
    auto alloc_addr = lib::roundUp(addr + 8, align);
    for (auto i = addr + 8; i < alloc_addr; ++i) {
        this->memory.write8(i, 0);
    }
    return alloc_addr;
}

void SimpleAllocator::dealloc(uint64_t addr, [[maybe_unused]] uint64_t size)
{
    const auto hasPrevChunk = [](uint64_t addr) {
        return addr > layout::HEAP_BASE;
    };
    const auto hasNextChunk = [](uint64_t addr, uint64_t len) {
        return addr + len < layout::HEAP_BOUNDARY;
    };
    while (this->memory.read8(addr) == 0) {
        --addr;
    }
    auto chunk_addr = addr - 7;
    auto chunk_len = this->memory.read64(chunk_addr) - 1;
    if (hasPrevChunk(chunk_addr)) {
        auto prev_chunk_len_with_flag = this->memory.read64(chunk_addr - 8);
        if ((prev_chunk_len_with_flag & 1) == 0) {
            chunk_addr -= prev_chunk_len_with_flag;
            chunk_len += prev_chunk_len_with_flag;
        }
    }
    if (hasNextChunk(chunk_addr, chunk_len)) {
        auto next_chunk_len_with_flag = this->memory.read64(chunk_addr + chunk_len);
        if ((next_chunk_len_with_flag & 1) == 0) {
            chunk_len += next_chunk_len_with_flag;
        }
    }
    this->memory.write64(chunk_addr, chunk_len);
    this->memory.write64(chunk_addr + chunk_len - 8, chunk_len);
}

uint64_t SimpleAllocator::findNextAvailable(uint64_t addr)
{
    uint64_t len_with_mark;
    while ((len_with_mark = this->memory.read64(addr)) & 1) {
        addr += len_with_mark - 1;
        if (addr >= layout::HEAP_BOUNDARY) {
            return -1;
        }
    }
    return addr;
}
