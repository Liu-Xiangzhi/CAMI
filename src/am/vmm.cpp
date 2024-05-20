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

#include <vmm.h>
#include <io_def.h>
#include <obj_man.h>
#include <foundation/type/mm.h>
#include <foundation/cross_platform.h>
#include <exception.h>
#include <cstring>
#include <memory>
#include <lib/format.h>
#ifdef CAMI_TARGET_INFO_UNIX_LIKE
#include <fcntl.h>
#elif defined(CAMI_TARGET_INFO_WINDOWS)
#include <shlwapi.h>
#include <fileapi.h>
#endif

using namespace cami;
using namespace am::layout;
using am::VirtualMemory;
using ts::Kind;
using ts::type_manager;

VirtualMemory::MMIO::MMIO(ObjectManager& om)
        : file_descriptor(new lib::SharedPtr<FileDescriptor>[FILE_DESCRIPTOR_MAX]{
        lib::makeShared<FileDescriptor>(STDIN_FD, MODE_READ_ONLY),
        lib::makeShared<FileDescriptor>(STDOUT_FD, MODE_WRITE_ONLY),
        lib::makeShared<FileDescriptor>(STDERR_FD, MODE_WRITE_ONLY),
})
{
    auto& u64 = type_manager.getBasicType(Kind::u64);
    om.newPermanent("<MMIO control>", u64, MMIO_BASE);
    om.newPermanent("<MMIO word0>", u64, MMIO_BASE + 8);
    om.newPermanent("<MMIO word1>", u64, MMIO_BASE + 16);
    om.newPermanent("<MMIO word2>", u64, MMIO_BASE + 24);
    om.newPermanent("<MMIO word3>", u64, MMIO_BASE + 32);
    om.newPermanent("<MMIO word4>", u64, MMIO_BASE + 40);
    om.newPermanent("<MMIO word5>", u64, MMIO_BASE + 48);
    om.newPermanent("<MMIO word6>", u64, MMIO_BASE + 56);
}

void VirtualMemory::read(uint8_t* dest, uint64_t addr, uint64_t len) const
{
    if (addr >= UINT64_MAX - len) {
        throw MemoryAccessException{addr, len, "too large length"};
    }
    if (this->inValidCodeSegment(addr, len)) {
        return this->readCode(dest, addr, len);
    }
    if (this->inValidDataSegment(addr, len)) {
        return this->readData(dest, addr, len);
    }
    if (this->inValidStackSegment(addr, len)) {
        return this->readStack(dest, addr, len);
    }
    if (VirtualMemory::inValidHeapSegment(addr, len)) {
        return this->readHeap(dest, addr, len);
    }
    if (VirtualMemory::inValidMMIOSegment(addr, len)) {
        return this->readMMIO(dest, addr, len);
    }
    throw MemoryAccessException{addr, len, "read invalid region"};
}

void VirtualMemory::write(uint64_t addr, const uint8_t* src, uint64_t len) // NOLINT
{
    if (addr >= UINT64_MAX - len) {
        throw MemoryAccessException{addr, len, "too large length"};
    }
    if (this->inValidDataSegment(addr, len)) {
        return this->writeData(addr, src, len);
    }
    if (this->inValidStackSegment(addr, len)) {
        return this->writeStack(addr, src, len);
    }
    if (VirtualMemory::inValidHeapSegment(addr, len)) {
        return this->writeHeap(addr, src, len);
    }
    if (VirtualMemory::inValidMMIOSegment(addr, len)) {
        return this->writeMMIO(addr, src, len);
    }
    throw MemoryAccessException{addr, len, "write invalid region"};
}

void VirtualMemory::zeroize(uint64_t addr, uint64_t len)
{
    if (addr >= UINT64_MAX - len) {
        throw MemoryAccessException{addr, len, "too large length"};
    }
    if (this->inValidDataSegment(addr, len)) {
        if (addr < this->string_literal_end && len != 0) {
            throw UBException{{UB::modify_string_literal},
                              lib::format("modify string literal, address = ${X}, length = ${X}", addr, len)};
        }
        std::memset(this->data.data() + (addr - DATA_BASE), 0, len);
        return;
    }
    if (this->inValidStackSegment(addr, len)) {
        std::memset(&this->stack[STACK_BOUNDARY - addr - len], 0, len);
        return;
    }
    if (VirtualMemory::inValidHeapSegment(addr, len)) {
        return this->zeroizeHeap(addr, len);
    }
    throw MemoryAccessException{addr, len, "zeroize invalid region"};
}

void VirtualMemory::notifyStackPointer(uint64_t val)
{
    auto stack_size = STACK_BOUNDARY - val;
    if (stack_size > this->stack.size()) {
        this->stack.resize(stack_size);
    }
    // optimize: may call `this->stack.shrink_to_fit()` if real stack size is less than
    //   the capacity of `this->stack`, but it should be noted that `shrink_to_fit` should be
    //   delayed once, since a function may return a struct/union(the address of that will be pushed into operand stack).
}

void VirtualMemory::readCode(uint8_t* dest, uint64_t addr, uint64_t len) const
{
    std::memcpy(dest, this->code.data() + (addr - CODE_BASE), len);
}

void VirtualMemory::readData(uint8_t* dest, uint64_t addr, uint64_t len) const
{
    std::memcpy(dest, this->data.data() + (addr - DATA_BASE), len);
}

void VirtualMemory::readStack(uint8_t* dest, uint64_t addr, uint64_t len) const
{
    for (size_t i = 0; i < len; ++i) {
        dest[i] = this->stack[STACK_BOUNDARY - (addr + i) - 1];
    }
}

void VirtualMemory::readHeap(uint8_t* dest, uint64_t addr, uint64_t len) const
{
    auto roundUpAddr = lib::roundUp(addr, Heap::PAGE_SIZE);
    if (addr < roundUpAddr) {
        if (addr + len <= roundUpAddr) {
            this->readPage(dest, addr, len);
            return;
        }
        auto read_len = roundUpAddr - addr;
        this->readPage(dest, addr, read_len);
        addr = roundUpAddr;
        len -= read_len;
        dest += read_len;
    }
    for (; len >= Heap::PAGE_SIZE; len -= Heap::PAGE_SIZE) {
        this->readPage(dest, addr, Heap::PAGE_SIZE);
        addr += Heap::PAGE_SIZE;
        dest += Heap::PAGE_SIZE;
    }
    if (len > 0) {
        this->readPage(dest, addr, len);
    }
}

void VirtualMemory::readMMIO(uint8_t* dest, uint64_t addr, uint64_t len) const
{
    if (addr % 8 || len != 8) {
        throw MemoryAccessException{addr, len, "incorrect read to MMIO object"};
    }
    ASSERT((addr - MMIO_BASE) / 8 < MMIO::_num, "precondition violation");
    *reinterpret_cast<uint64_t*>(dest) = this->mmio.content[(addr - MMIO_BASE) / 8];
}

void VirtualMemory::writeData(uint64_t addr, const uint8_t* src, uint64_t len)
{
    if (addr < this->string_literal_end && len != 0) {
        throw UBException{{UB::modify_string_literal},
                          lib::format("modify string literal, address = ${x}, length = ${x}", addr, len)};
    }
    std::memcpy(this->data.data() + (addr - DATA_BASE), src, len);
}

void VirtualMemory::writeStack(uint64_t addr, const uint8_t* src, uint64_t len)
{
    for (size_t i = 0; i < len; ++i) {
        this->stack[STACK_BOUNDARY - (addr + i) - 1] = src[i];
    }
}

void VirtualMemory::writeHeap(uint64_t addr, const uint8_t* src, uint64_t len)
{
    auto roundUpAddr = lib::roundUp(addr, Heap::PAGE_SIZE);
    if (addr < roundUpAddr) {
        if (addr + len <= roundUpAddr) {
            this->writePage(addr, src, len);
            return;
        }
        auto write_len = roundUpAddr - addr;
        this->writePage(addr, src, write_len);
        addr = roundUpAddr;
        len -= write_len;
        src += write_len;
    }
    for (; len >= Heap::PAGE_SIZE; len -= Heap::PAGE_SIZE) {
        this->writePage(addr, src, Heap::PAGE_SIZE);
        addr += Heap::PAGE_SIZE;
        src += Heap::PAGE_SIZE;
    }
    if (len > 0) {
        this->writePage(addr, src, len);
    }
}

void VirtualMemory::zeroizeHeap(uint64_t addr, uint64_t len)
{
    auto roundUpAddr = lib::roundUp(addr, Heap::PAGE_SIZE);
    if (addr < roundUpAddr) {
        if (addr + len <= roundUpAddr) {
            this->zeroizePage(addr, len);
            return;
        }
        auto write_len = roundUpAddr - addr;
        this->zeroizePage(addr, write_len);
        addr = roundUpAddr;
        len -= write_len;
    }
    for (; len >= Heap::PAGE_SIZE; len -= Heap::PAGE_SIZE) {
        this->zeroizePage(addr, Heap::PAGE_SIZE);
        addr += Heap::PAGE_SIZE;
    }
    if (len > 0) {
        this->zeroizePage(addr, len);
    }
}

void VirtualMemory::writeMMIO(uint64_t addr, const uint8_t* src, uint64_t len) // NOLINT
{
    if (addr % 8 || len != 8) {
        throw MemoryAccessException{addr, len, "incorrect write to MMIO object"};
    }
    ASSERT((addr - MMIO_BASE) / 8 < MMIO::_num, "precondition violation");
    auto val = *reinterpret_cast<const uint64_t*>(src);
    this->mmio.content[(addr - MMIO_BASE) / 8] = val;
    if (addr == layout::MMIO_BASE + MMIO::control * 8) {
        uint64_t ec;
        switch (val) {
        case MMIO::open:
            ec = this->do_open();
            break;
        case MMIO::close:
            ec = this->do_close();
            break;
        case MMIO::read:
            ec = this->do_read();
            break;
        case MMIO::write:
            ec = this->do_write();
            break;
        case MMIO::seek:
            ec = this->do_seek();
            break;
        case MMIO::truncate:
            ec = this->do_truncate();
            break;
        case MMIO::rename:
            ec = this->do_remove();
            break;
        case MMIO::remove:
            ec = this->do_rename();
            break;
        case MMIO::dup:
            ec = this->do_dup();
            break;
        default:
            throw MMIOAccessException{"invalid control number "s + std::to_string(val)};
        }
        this->mmio.content[MMIO::control] = ec;
    }
}

void VirtualMemory::readPage(uint8_t* dest, uint64_t addr, uint64_t len) const
{
    ASSERT(addr % Heap::PAGE_SIZE + len <= Heap::PAGE_SIZE, "precondition violation");
    auto page = this->getPage(addr);
    if (!page) {
        throw MemoryAccessException{addr, len, "read unallocated heap page"};
    }
    std::memcpy(dest, (*page)->data + addr % Heap::PAGE_SIZE, len);
}

void VirtualMemory::writePage(uint64_t addr, const uint8_t* src, uint64_t len)
{
    ASSERT(addr % Heap::PAGE_SIZE + len <= Heap::PAGE_SIZE, "precondition violation");
    auto page = [&]() {
        if (auto pg = this->getPage(addr); pg) {
            return *pg;
        }
        return this->allocPage(addr);
    }();
    std::memcpy(page->data + addr % Heap::PAGE_SIZE, src, len);
}

void VirtualMemory::zeroizePage(uint64_t addr, uint64_t len)
{
    ASSERT(addr % Heap::PAGE_SIZE + len <= Heap::PAGE_SIZE, "precondition violation");
    auto page = [&]() {
        if (auto pg = this->getPage(addr); pg) {
            return *pg;
        }
        return this->allocPage(addr);
    }();
    std::memset(page->data + addr % Heap::PAGE_SIZE, 0, len);
}

lib::Optional<VirtualMemory::Heap::Page*> VirtualMemory::getPage(uint64_t addr) const
{
    auto piece_size = (HEAP_BOUNDARY - HEAP_BASE) / Heap::PAGE_TABLE_ITEM_NUM;
    addr -= HEAP_BASE;
    auto page_table = this->heap.page_table;
    for (size_t i = 0; i < Heap::PAGE_TABLE_LEVEL - 1; ++i) {
        auto idx = addr / piece_size;
        ASSERT(idx < Heap::PAGE_TABLE_ITEM_NUM, "");
        addr %= piece_size;
        page_table = page_table->item[idx].sub_page_table;
        if (page_table == nullptr) {
            return {};
        }
    }
    if (auto* page = page_table->item[addr / piece_size].page;page != nullptr) {
        return page;
    }
    return {};
}

VirtualMemory::Heap::Page* VirtualMemory::allocPage(uint64_t addr) const
{
    auto piece_size = (HEAP_BOUNDARY - HEAP_BASE) / Heap::PAGE_TABLE_ITEM_NUM;
    addr -= HEAP_BASE;
    auto page_table = this->heap.page_table;
    for (size_t i = 0; i < Heap::PAGE_TABLE_LEVEL - 1; ++i) {
        auto idx = addr / piece_size;
        ASSERT(idx < Heap::PAGE_TABLE_ITEM_NUM, "");
        addr %= piece_size;
        if (page_table->item[idx].sub_page_table == nullptr) {
            page_table->item[idx].sub_page_table = new Heap::PageTable{};
        }
        page_table = page_table->item[idx].sub_page_table;
    }
    auto idx = addr / piece_size;
    auto* page = page_table->item[idx].page;
    if (page == nullptr) {
        page = page_table->item[idx].page = new Heap::Page{};
    }
    return page;
}

uint64_t VirtualMemory::do_open()
{
    auto addr = this->mmio.content[MMIO::word0];
    if (addr >= MMIO_BASE) {
        return E_INVALID_ADDRESS;
    }
    auto len = this->mmio.content[MMIO::word1];
    auto mode = this->mmio.content[MMIO::word2];
    auto fd_idx = this->mmio.content[MMIO::word3];
    if (fd_idx == -1) {
        if ((fd_idx = this->findAvailableFd()) == -1) {
            return E_FD_EXHAUSTED;
        }
    } else if (fd_idx > MMIO::FILE_DESCRIPTOR_MAX || this->mmio.file_descriptor[fd_idx]->file != IVD_FD) {
        return E_INVALID_FD;
    }
    std::unique_ptr<uint8_t[]> buf{new uint8_t[len]};
    try {
        this->read(buf.get(), addr, len);
    } catch (const MemoryAccessException& e) {
        return E_BAD_IN_BUF;
    }
    if (!(mode & MODE_WRITE_MASK) && (mode & MODE_TRUNC)) {
        return E_DENY;
    }
    auto p = reinterpret_cast<char*>(buf.get());
    return this->sys_open(std::string(MMIO::FS_ROOT).append(p, len), mode, fd_idx);
}

uint64_t VirtualMemory::do_close()
{
    auto fd_idx = this->mmio.content[MMIO::word0];
    if (fd_idx > MMIO::FILE_DESCRIPTOR_MAX) {
        return E_INVALID_FD;
    }
    auto fd = this->mmio.file_descriptor[fd_idx]->file;
    if (fd == IVD_FD) {
        return E_INVALID_FD;
    }
    return this->sys_close(fd);
}

uint64_t VirtualMemory::do_read() // NOLINT
{
    auto fd_idx = this->mmio.content[MMIO::word0];
    if (fd_idx > MMIO::FILE_DESCRIPTOR_MAX) {
        return E_INVALID_FD;
    }
    auto& mmio_fd = this->mmio.file_descriptor[fd_idx];
    if (mmio_fd->file == IVD_FD) {
        return E_INVALID_FD;
    }
    if (!(mmio_fd->mode & MODE_READ_MASK)) {
        return E_DENY;
    }
    auto addr = this->mmio.content[MMIO::word1];
    if (addr >= MMIO_BASE) {
        return E_INVALID_ADDRESS;
    }
    auto len = this->mmio.content[MMIO::word2];
    std::unique_ptr<uint8_t[]> buf{new uint8_t[len]};
    auto size = this->sys_read(mmio_fd->file, buf.get(), len);
    try {
        this->write(addr, buf.get(), len);
    } catch (const MemoryAccessException& e) {
        return E_BAD_OUT_BUF;
    }
    return size;
}

uint64_t VirtualMemory::do_write()
{
    auto fd_idx = this->mmio.content[MMIO::word0];
    if (fd_idx > MMIO::FILE_DESCRIPTOR_MAX) {
        return E_INVALID_FD;
    }
    auto& mmio_fd = this->mmio.file_descriptor[fd_idx];
    if (mmio_fd->file == IVD_FD) {
        return E_INVALID_FD;
    }
    if (!(mmio_fd->mode & MODE_WRITE_MASK)) {
        return E_DENY;
    }
    auto addr = this->mmio.content[MMIO::word1];
    if (addr >= MMIO_BASE) {
        return E_INVALID_ADDRESS;
    }
    auto len = this->mmio.content[MMIO::word2];
    std::unique_ptr<uint8_t[]> buf{new uint8_t[len]};
    try {
        this->read(buf.get(), addr, len);
    } catch (const MemoryAccessException& e) {
        return E_BAD_IN_BUF;
    }
    return this->sys_write(mmio_fd->file, buf.get(), len);
}

uint64_t VirtualMemory::do_seek()
{
    auto fd_idx = this->mmio.content[MMIO::word0];
    if (fd_idx > MMIO::FILE_DESCRIPTOR_MAX) {
        return E_INVALID_FD;
    }
    auto fd = this->mmio.file_descriptor[fd_idx]->file;
    if (fd == IVD_FD) {
        return E_INVALID_FD;
    }
    auto anchor = this->mmio.content[MMIO::word1];
    if (anchor != SEEK_HEAD && anchor != SEEK_CURRENT && anchor != SEEK_TAIL) {
        return E_INVALID_ANCHOR;
    }
    auto offset = this->mmio.content[MMIO::word2];
    return this->sys_seek(fd, anchor, offset);
}

uint64_t VirtualMemory::do_truncate()
{
    auto fd_idx = this->mmio.content[MMIO::word0];
    if (fd_idx > MMIO::FILE_DESCRIPTOR_MAX) {
        return E_INVALID_FD;
    }
    auto fd = this->mmio.file_descriptor[fd_idx]->file;
    if (fd == IVD_FD) {
        return E_INVALID_FD;
    }
    auto len = this->mmio.content[MMIO::word1];
    return this->sys_trunc(fd, len);
}

uint64_t VirtualMemory::do_rename()
{
    auto addr1 = this->mmio.content[MMIO::word0];
    if (addr1 >= MMIO_BASE) {
        return E_INVALID_ADDRESS;
    }
    auto len1 = this->mmio.content[MMIO::word1];
    auto addr2 = this->mmio.content[MMIO::word2];
    if (addr2 >= MMIO_BASE) {
        return E_INVALID_ADDRESS;
    }
    auto len2 = this->mmio.content[MMIO::word3];
    std::unique_ptr<uint8_t[]> buf1{new uint8_t[len1]};
    std::unique_ptr<uint8_t[]> buf2{new uint8_t[len2]};
    try {
        this->read(buf1.get(), addr1, len1);
        this->read(buf2.get(), addr2, len2);
    } catch (const MemoryAccessException& e) {
        return E_BAD_IN_BUF;
    }
    auto p1 = reinterpret_cast<char*>(buf1.get());
    auto p2 = reinterpret_cast<char*>(buf2.get());
    return this->sys_rename(std::string(MMIO::FS_ROOT).append(p1, len1),
                            std::string(MMIO::FS_ROOT).append(p2, len2));
}

uint64_t VirtualMemory::do_remove()
{
    auto addr = this->mmio.content[MMIO::word0];
    if (addr >= MMIO_BASE) {
        return E_INVALID_ADDRESS;
    }
    auto len = this->mmio.content[MMIO::word1];
    std::unique_ptr<uint8_t[]> buf{new uint8_t[len]};
    try {
        this->read(buf.get(), addr, len);
    } catch (const MemoryAccessException& e) {
        return E_BAD_IN_BUF;
    }
    auto p = reinterpret_cast<char*>(buf.get());
    return this->sys_remove(std::string(MMIO::FS_ROOT).append(p, len));
}

uint64_t VirtualMemory::do_dup()
{
    auto fd1_idx = this->mmio.content[MMIO::word0];
    if (fd1_idx > MMIO::FILE_DESCRIPTOR_MAX) {
        return E_INVALID_FD;
    }
    if (this->mmio.file_descriptor[fd1_idx]->file == IVD_FD) {
        return E_INVALID_FD;
    }
    auto fd2_idx = this->mmio.content[MMIO::word1];
    if (fd2_idx > MMIO::FILE_DESCRIPTOR_MAX) {
        return E_INVALID_FD;
    }
    if (this->mmio.file_descriptor[fd2_idx]->file == IVD_FD) {
        return E_INVALID_FD;
    }
    this->mmio.file_descriptor[fd2_idx] = this->mmio.file_descriptor[fd1_idx];
    return SUCCESS;
}

uint64_t VirtualMemory::findAvailableFd() const
{
    for (size_t i = 0; i < MMIO::FILE_DESCRIPTOR_MAX; ++i) {
        if (this->mmio.file_descriptor[i]->file == IVD_FD) {
            return i;
        }
    }
    return -1;
}

uint64_t VirtualMemory::sys_open(const std::string& name, uint64_t mode, uint64_t fd_no)
{
#ifdef CAMI_TARGET_INFO_UNIX_LIKE
    const auto basic_mode = mode & MODE_BMODE_MASK;
    if (basic_mode == MODE_TEST) {
        return ::access(name.c_str(), F_OK) == 0 ? SUCCESS : E_NOT_EXIST;
    }
    static_assert(MODE_READ_ONLY - 1 == O_RDONLY);
    static_assert(MODE_WRITE_ONLY - 1 == O_WRONLY);
    static_assert(MODE_READ_WRITE - 1 == O_RDWR);
    auto md = (basic_mode - 1) | (mode & MODE_CREAT ? O_CREAT : 0) | (mode & MODE_TRUNC ? O_TRUNC : 0);
    auto ec = ::open(name.c_str(), static_cast<int>(md));
    if (ec < 0) {
        this->mmio.content[MMIO::word0] = errno;
        return E_SYSTEM;
    }
    auto& file_desc = this->mmio.file_descriptor[fd_no];
    file_desc->file = ec;
    file_desc->mode = static_cast<int>(mode);
    return SUCCESS;
#else
    const auto basic_mode = mode & MODE_BMODE_MASK;
    if (basic_mode == MODE_TEST) {
        return PathFileExistsA(name.c_str()) ? SUCCESS : E_NOT_EXIST;
    }
    auto md = (basic_mode & MODE_READ_MASK ? GENERIC_READ : 0) | (basic_mode & MODE_WRITE_MASK ? GENERIC_WRITE : 0); 
    auto creation_disposition = mode & (MODE_CREAT | MODE_TRUNC) ? CREATE_ALWAYS : (mode & MODE_CREAT ? OPEN_ALWAYS : OPEN_EXISTING);
    auto handel = CreateFileA(name.c_str(), md, 0, nullptr, creation_disposition, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handel == INVALID_HANDLE_VALUE) {
        this->mmio.content[MMIO::word0] = GetLastError();
        return E_SYSTEM;
    }
    auto& file_desc = this->mmio.file_descriptor[fd_no];
    file_desc->file = handel;
    file_desc->mode = static_cast<int>(mode);
    return SUCCESS;
#endif
}

uint64_t VirtualMemory::sys_close(FD fd)
{
#ifdef CAMI_TARGET_INFO_UNIX_LIKE
    auto err = ::close(fd);
    if (err == 0) {
        this->mmio.file_descriptor[this->mmio.content[MMIO::word0]]->file = IVD_FD;
        return SUCCESS;
    } else {
        this->mmio.content[MMIO::word0] = errno;
        return E_SYSTEM;
    }
#else
    auto err = CloseHandle(fd);
    if (err == 0) {
        this->mmio.content[MMIO::word0] = GetLastError();
        return E_SYSTEM;
    } else {
        this->mmio.file_descriptor[this->mmio.content[MMIO::word0]]->file = IVD_FD;
        return SUCCESS;
    }
#endif
}

uint64_t VirtualMemory::sys_read(FD fd, void* buf, uint64_t len)
{
#ifdef CAMI_TARGET_INFO_UNIX_LIKE
    auto size = ::read(fd, buf, len);
    if (size < 0) {
        this->mmio.content[MMIO::word0] = errno;
        return E_SYSTEM;
    }
    return size;
#else
    DWORD size;
    auto err = ReadFile(fd, buf, len, &size, nullptr);
    if (!err) {
        this->mmio.content[MMIO::word0] = GetLastError();
        return E_SYSTEM;
    }
    return size;
#endif
}

uint64_t VirtualMemory::sys_write(FD fd, void* buf, uint64_t len)
{
#ifdef CAMI_TARGET_INFO_UNIX_LIKE
    auto size = ::write(fd, buf, len);
    if (size < 0) {
        this->mmio.content[MMIO::word0] = errno;
        return E_SYSTEM;
    }
    return size;
#else
    DWORD size;
    auto err = WriteFile(fd, buf, len, &size, nullptr);
    if (!err) {
        this->mmio.content[MMIO::word0] = GetLastError();
        return E_SYSTEM;
    }
    return size;
#endif
}

uint64_t VirtualMemory::sys_seek(FD fd, uint64_t anchor, uint64_t offset)
{
    ASSERT(anchor == SEEK_HEAD || anchor == SEEK_CURRENT || anchor == SEEK_TAIL, "precondition violation");
#ifdef CAMI_TARGET_INFO_UNIX_LIKE
    static_assert(SEEK_HEAD == SEEK_SET);
    static_assert(SEEK_CURRENT == SEEK_CUR);
    static_assert(SEEK_TAIL == SEEK_END);
    auto off = ::lseek(fd, static_cast<__off_t>(offset), static_cast<int>(anchor));
    if (off == -1) {
        this->mmio.content[MMIO::word0] = errno;
        return E_SYSTEM;
    }
    return off;
#else
    static_assert(SEEK_HEAD == FILE_BEGIN);
    static_assert(SEEK_CURRENT == FILE_CURRENT);
    static_assert(SEEK_TAIL == FILE_END);
    LARGE_INTEGER off;
    LARGE_INTEGER result;
    off.QuadPart = offset;
    if (!SetFilePointerEx(fd, off, &result, anchor)) {
        this->mmio.content[MMIO::word0] = GetLastError();
        return E_SYSTEM;
    }
    return result.QuadPart;
#endif
}

uint64_t VirtualMemory::sys_trunc(FD fd, uint64_t len)
{
#ifdef CAMI_TARGET_INFO_UNIX_LIKE
    auto ec = ::ftruncate(fd, static_cast<__off_t>(len));
    if (ec < 0) {
        this->mmio.content[MMIO::word0] = errno;
        return E_SYSTEM;
    }
    return SUCCESS;
#else
    LARGE_INTEGER new_size;
    new_size.QuadPart = len;
    if (!SetFilePointerEx(fd, new_size, NULL, FILE_BEGIN)) {
        this->mmio.content[MMIO::word0] = GetLastError();
        return E_SYSTEM;
    }
    if (!SetEndOfFile(fd)) {
        this->mmio.content[MMIO::word0] = GetLastError();
        return E_SYSTEM;
    }
    return SUCCESS;
#endif
}

uint64_t VirtualMemory::sys_rename(const std::string& from, const std::string& to)
{
#ifdef CAMI_TARGET_INFO_UNIX_LIKE
    auto ec = ::rename(from.c_str(), to.c_str());
    if (ec < 0) {
        this->mmio.content[MMIO::word0] = errno;
        return E_SYSTEM;
    }
    return SUCCESS;
#else
    if (!MoveFile(from.c_str(), to.c_str())) {
        this->mmio.content[MMIO::word0] = GetLastError();
        return E_SYSTEM;
    }
    return SUCCESS;
#endif
}

uint64_t VirtualMemory::sys_remove(const std::string& name)
{
#ifdef CAMI_TARGET_INFO_UNIX_LIKE
    auto ec = ::remove(name.c_str());
    if (ec < 0) {
        this->mmio.content[MMIO::word0] = errno;
        return E_SYSTEM;
    }
    return SUCCESS;
#else
    if (!DeleteFile(name.c_str())) {
        this->mmio.content[MMIO::word0] = errno;
        return E_SYSTEM;
    }
    return SUCCESS;
#endif
}
