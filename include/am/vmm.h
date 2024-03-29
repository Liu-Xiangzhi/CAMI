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

#ifndef CAMI_AM_VMM_H
#define CAMI_AM_VMM_H

#include <config.h>
#include <cstddef>
#include <cstdint>
#include <deque>
#include "object.h"
#include "exception.h"
#include <foundation/cross_platform.h>
#include <lib/array.h>
#include <lib/utils.h>
#include <lib/shared_ptr.h>
#include <lib/format.h>

namespace cami::am {
class ObjectManager;
namespace layout {
constexpr std::size_t CODE_BASE = 0x0000'0000'0001'0000ULL;
constexpr std::size_t CODE_BOUNDARY = 0x1000'0000'0000'0000ULL;
constexpr std::size_t DATA_BASE = CODE_BOUNDARY;
constexpr std::size_t DATA_BOUNDARY = 0x2000'0000'0000'0000ULL;
constexpr std::size_t HEAP_BASE = DATA_BOUNDARY;
constexpr std::size_t HEAP_BOUNDARY = 0x5fff'ffff'ffff'0000ULL;
constexpr std::size_t STACK_BASE = 0x6000'0000'0000'0000ULL;
constexpr std::size_t STACK_BOUNDARY = 0x8000'0000'0000'0000ULL;
constexpr std::size_t MMIO_BASE = STACK_BOUNDARY;
constexpr std::size_t MMIO_BOUNDARY = 0xa000'0000'0000'0000ULL;
constexpr std::size_t NOT_USED_BASE = MMIO_BOUNDARY;
constexpr std::size_t NOT_USED_BOUNDARY = 0xffff'ffff'ffff'ffffULL;
} // namespace layout

using namespace lib::literals;

class VirtualMemory
{
    struct Heap
    {
        static constexpr std::size_t PAGE_SIZE = CAMI_MEMORY_HEAP_PAGE_SIZE;
        static constexpr std::size_t PAGE_TABLE_LEVEL = CAMI_MEMORY_HEAP_PAGE_TABLE_LEVEL;
        static constexpr std::size_t TOTAL_PAGE_NUM = (layout::HEAP_BOUNDARY - layout::HEAP_BASE) / PAGE_SIZE;
        static constexpr std::size_t PAGE_TABLE_ITEM_NUM = lib::roundUpNthRoot(TOTAL_PAGE_NUM, PAGE_TABLE_LEVEL);

        struct Page
        {
            uint8_t* data = new uint8_t[PAGE_SIZE]{};

            ~Page()
            {
                delete[] this->data;
            }
        };

        struct PageTable
        {
            union
            {
                PageTable* sub_page_table = nullptr;
                Page* page;
            } item[PAGE_TABLE_ITEM_NUM]{};
        };
        PageTable* page_table = new PageTable{};

        ~Heap()
        {
            deletePageTable(this->page_table, 1);
        }

        static void deletePageTable(PageTable* table, int level) // NOLINT
        {
            if (level == PAGE_TABLE_LEVEL) {
                for (auto i: table->item) {
                    delete i.page;
                }
            } else {
                for (auto i: table->item) {
                    if (i.sub_page_table != nullptr) {
                        deletePageTable(i.sub_page_table, level + 1);
                    }
                }
            }
            delete table;
        }
    };

    struct MMIO
    {
        static constexpr uint64_t FILE_DESCRIPTOR_MAX = CAMI_MEMORY_MMIO_MAX_FILE;
        static constexpr auto FS_ROOT = CAMI_FILE_SYSTEM_ROOT;
        enum
        {
            control,
            word0, word1, word2, word3, word4, word5, word6,
            _num
        };
        enum
        {
            open, close, read, write, seek, truncate, rename, remove, dup
        };
        struct FileDescriptor
        {
            FD file = IVD_FD;
            int mode = 0;
        };
        uint64_t content[_num]{};
        lib::SharedPtr<FileDescriptor>* file_descriptor;
        explicit MMIO(ObjectManager& om);

        ~MMIO()
        {
            delete[] this->file_descriptor;
        }
    };

    lib::Array<uint8_t> code;
    lib::Array<uint8_t> data;
    const uint64_t string_literal_base;
    std::deque<uint8_t> stack;
    Heap heap;
    MMIO mmio;

public:
    static constexpr std::size_t MMIO_OBJECT_NUM = MMIO::_num;
public:
    explicit VirtualMemory(lib::Array<uint8_t> code, lib::Array<uint8_t> data, uint64_t string_literal_base,
                           ObjectManager& om)
            : code(std::move(code)), data(std::move(data)), string_literal_base(string_literal_base), mmio(om)
    {
        using namespace layout;
        if (this->code.length() > CODE_BOUNDARY - CODE_BASE) {
            throw AMInitialFailedException{"too large code segment size"};
        }
        if (this->data.length() > DATA_BOUNDARY - DATA_BASE) {
            throw AMInitialFailedException{"too large data segment size"};
        }
    }

public:
    void read(uint8_t* dest, uint64_t addr, uint64_t len) const;
    void write(uint64_t addr, const uint8_t* src, uint64_t len);
    void zeroize(uint64_t addr, uint64_t len);
    void notifyStackPointer(uint64_t val);

    [[nodiscard]] uint8_t read8(uint64_t addr) const
    {
        uint8_t res;
        this->read(&res, addr, 1);
        return res;
    }

    [[nodiscard]] uint16_t read16(uint64_t addr) const
    {
        if (addr % 2) {
            throw MemoryAccessException{addr, 2, "unaligned read 16bits"};
        }
        uint16_t res;
        this->read(reinterpret_cast<uint8_t*>(&res), addr, 2);
        return res;
    }

    [[nodiscard]] uint32_t read32(uint64_t addr) const
    {
        if (addr % 4) {
            throw MemoryAccessException{addr, 2, "unaligned read 32bits"};
        }
        uint32_t res;
        this->read(reinterpret_cast<uint8_t*>(&res), addr, 4);
        return res;
    }

    [[nodiscard]] uint64_t read64(uint64_t addr) const
    {
        if (addr % 8) {
            throw MemoryAccessException{addr, 2, "unaligned read 64bits"};
        }
        uint64_t res;
        this->read(reinterpret_cast<uint8_t*>(&res), addr, 8);
        return res;
    }

    void write8(uint64_t addr, uint8_t value)
    {
        this->write(addr, &value, 1);
    }

    void write16(uint64_t addr, uint16_t value)
    {
        if (addr % 2) {
            throw MemoryAccessException{addr, 2, "unaligned write 16bits"};
        }
        this->write(addr, reinterpret_cast<uint8_t*>(&value), 2);
    }

    void write32(uint64_t addr, uint32_t value)
    {
        if (addr % 4) {
            throw MemoryAccessException{addr, 2, "unaligned write 32bits"};
        }
        this->write(addr, reinterpret_cast<uint8_t*>(&value), 4);
    }

    void write64(uint64_t addr, uint64_t value)
    {
        if (addr % 8) {
            throw MemoryAccessException{addr, 2, "unaligned write 64bits"};
        }
        this->write(addr, reinterpret_cast<uint8_t*>(&value), 8);
    }

    static bool inCodeSegment(uint64_t addr) noexcept
    {
        return addr >= layout::CODE_BASE && addr < layout::CODE_BOUNDARY;
    }

    static bool inDataSegment(uint64_t addr) noexcept
    {
        return addr >= layout::DATA_BASE && addr < layout::DATA_BOUNDARY;
    }

    static bool inStackSegment(uint64_t addr) noexcept
    {
        return addr >= layout::STACK_BASE && addr < layout::STACK_BOUNDARY;
    }

    static bool inHeapSegment(uint64_t addr) noexcept
    {
        return addr >= layout::HEAP_BASE && addr < layout::HEAP_BOUNDARY;
    }

    static bool inMMIOSegment(uint64_t addr) noexcept
    {
        return addr >= layout::MMIO_BASE && addr < layout::MMIO_BOUNDARY;
    }

    static bool inCodeSegment(uint64_t addr, uint64_t len) noexcept
    {
        ASSERT(addr + len >= addr, "too large length");
        return addr >= layout::CODE_BASE && addr + len <= layout::CODE_BOUNDARY;
    }

    static bool inDataSegment(uint64_t addr, uint64_t len) noexcept
    {
        ASSERT(addr + len >= addr, "too large length");
        return addr >= layout::DATA_BASE && addr + len <= layout::DATA_BOUNDARY;
    }

    static bool inStackSegment(uint64_t addr, uint64_t len) noexcept
    {
        ASSERT(addr + len >= addr, "too large length");
        return addr >= layout::STACK_BASE && addr + len <= layout::STACK_BOUNDARY;
    }

    static bool inHeapSegment(uint64_t addr, uint64_t len) noexcept
    {
        ASSERT(addr + len >= addr, "too large length");
        return addr >= layout::HEAP_BASE && addr + len <= layout::HEAP_BOUNDARY;
    }

    static bool inMMIOSegment(uint64_t addr, uint64_t len) noexcept
    {
        ASSERT(addr + len >= addr, "too large length");
        return addr >= layout::MMIO_BASE && addr + len <= layout::MMIO_BOUNDARY;
    }

    [[nodiscard]] bool inValidCodeSegment(uint64_t addr) const noexcept
    {
        return addr >= layout::CODE_BASE && addr < this->code.length() + layout::CODE_BASE;
    }

    [[nodiscard]] bool inValidDataSegment(uint64_t addr) const noexcept
    {
        return addr >= layout::DATA_BASE && addr < this->data.length() + layout::DATA_BASE;
    }

    [[nodiscard]] bool inValidStackSegment(uint64_t addr) const noexcept
    {
        return addr < layout::STACK_BOUNDARY && addr >= layout::STACK_BOUNDARY - this->stack.size();
    }

    static bool inValidHeapSegment(uint64_t addr) noexcept
    {
        return inHeapSegment(addr);
    }

    static bool inValidMMIOSegment(uint64_t addr) noexcept
    {
        return addr >= layout::MMIO_BASE && addr < layout::MMIO_BASE + MMIO::_num * 8;
    }

    [[nodiscard]] bool inValidCodeSegment(uint64_t addr, uint64_t len) const noexcept
    {
        ASSERT(addr + len >= addr, "too large length");
        return addr >= layout::CODE_BASE && addr + len <= this->code.length() + layout::CODE_BASE;
    }

    [[nodiscard]] bool inValidDataSegment(uint64_t addr, uint64_t len) const noexcept
    {
        ASSERT(addr + len >= addr, "too large length");
        return addr >= layout::DATA_BASE && addr + len <= this->data.length() + layout::DATA_BASE;
    }

    [[nodiscard]] bool inValidStackSegment(uint64_t addr, uint64_t len) const noexcept
    {
        ASSERT(addr + len >= addr, "too large length");
        return addr + len <= layout::STACK_BOUNDARY && addr >= layout::STACK_BOUNDARY - this->stack.size();
    }

    static bool inValidHeapSegment(uint64_t addr, uint64_t len) noexcept
    {
        ASSERT(addr + len >= addr, "too large length");
        return addr >= layout::HEAP_BASE && addr + len <= layout::HEAP_BOUNDARY;
    }

    static bool inValidMMIOSegment(uint64_t addr, uint64_t len) noexcept
    {
        ASSERT(addr + len >= addr, "too large length");
        return addr >= layout::MMIO_BASE && addr + len < layout::MMIO_BASE + MMIO::_num * 8;
    }

private:
    void readCode(uint8_t* dest, uint64_t addr, uint64_t len) const;
    void readData(uint8_t* dest, uint64_t addr, uint64_t len) const;
    void readStack(uint8_t* dest, uint64_t addr, uint64_t len) const;
    void readHeap(uint8_t* dest, uint64_t addr, uint64_t len) const;
    void readMMIO(uint8_t* dest, uint64_t addr, uint64_t len) const;
    void writeData(uint64_t addr, const uint8_t* src, uint64_t len);
    void writeStack(uint64_t addr, const uint8_t* src, uint64_t len);
    void writeHeap(uint64_t addr, const uint8_t* src, uint64_t len);
    void writeMMIO(uint64_t addr, const uint8_t* src, uint64_t len);
    void zeroizeHeap(uint64_t addr, uint64_t len);
    void readPage(uint8_t* dest, uint64_t addr, uint64_t len) const;
    void writePage(uint64_t addr, const uint8_t* src, uint64_t len);
    void zeroizePage(uint64_t addr, uint64_t len);
    [[nodiscard]] lib::Optional<Heap::Page*> getPage(uint64_t addr) const;
    [[nodiscard]] Heap::Page* allocPage(uint64_t addr) const;
    [[nodiscard]] uint64_t findAvailableFd() const;
    uint64_t do_open();
    uint64_t do_close();
    uint64_t do_read();
    uint64_t do_write();
    uint64_t do_seek();
    uint64_t do_truncate();
    uint64_t do_rename();
    uint64_t do_remove();
    uint64_t do_dup();
    uint64_t sys_open(const std::string& name, uint64_t mode, uint64_t fd_idx);
    uint64_t sys_close(FD fd);
    uint64_t sys_read(FD fd, void* buf, uint64_t len);
    uint64_t sys_write(FD fd, void* buf, uint64_t len);
    uint64_t sys_seek(FD fd, uint64_t anchor, uint64_t offset);
    uint64_t sys_trunc(FD fd, uint64_t len);
    uint64_t sys_rename(const std::string& from, const std::string& to);
    uint64_t sys_remove(const std::string& name);
};

} // namespace cami::am

#include <foundation/undef_cross_platform.h>

CAMI_DECLARE_FORMATTER(cami::am::VirtualMemory);

#endif //CAMI_AM_VMM_H
