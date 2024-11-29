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

#ifndef CAMI_AM_EXCEPTION_H
#define CAMI_AM_EXCEPTION_H

#include <stdexcept>
#include <cstdint>
#include <vector>
#include "ub.h"
namespace cami {

using std::operator ""s;
using std::operator ""sv;

// "ignorable" here means it's still possible to continue (running am) after
//   catching those exceptions, not means those exceptions are not important
class IgnorableException : public std::runtime_error
{
public:
    explicit IgnorableException(const std::string& arg) : runtime_error(arg) {}

    explicit IgnorableException(const char* string) : runtime_error(string) {}

    explicit IgnorableException(runtime_error&& error) : runtime_error(error) {}

    explicit IgnorableException(const runtime_error& error) : runtime_error(error) {}
};

class InvalidOpcodeException : public IgnorableException
{
public:
    explicit InvalidOpcodeException(uint8_t opcode)
            : IgnorableException("Invalid opcode: "s + std::to_string(opcode)) {}
};

class ObjectStorageOutOfMemoryException : public std::runtime_error
{
public:
    explicit ObjectStorageOutOfMemoryException(const std::string& obj_name)
            : runtime_error("object storage out of memory while allocating for object: "s + obj_name) {}
};

class ReferenceCountingOverflowException : public std::runtime_error
{
public:
    explicit ReferenceCountingOverflowException(std::string_view who)
            : runtime_error(lib::format("reference counting value of ${} overflow", who)) {}
};

class MemoryAccessException : public IgnorableException
{
public:
    explicit MemoryAccessException(uint64_t addr, uint64_t len, std::string_view what)
            : IgnorableException(lib::format("memory access fault (from ${x}, len ${}): ${}", addr, len, what)) {}
};

class MMIOAccessException : public IgnorableException
{
public:
    explicit MMIOAccessException(const std::string& what)
            : IgnorableException("MMIO access failed: "s + what) {}
};

class JumpOutOfBoundaryException : public IgnorableException
{
public:
    explicit JumpOutOfBoundaryException(uint64_t addr)
            : IgnorableException("jump out of boundary(current function), aimed address: "s + std::to_string(addr)) {}
};

class AMInitialFailedException : public std::runtime_error
{
public:
    explicit AMInitialFailedException(const std::string& what)
            : runtime_error("Abstract machine initialization failed, due to: "s + what) {}
};

class ConstraintViolationException : public IgnorableException
{
public:
    explicit ConstraintViolationException(const std::string& arg)
            : IgnorableException("constraint violation: "s + arg) {}
};

class UBException : public IgnorableException
{
    std::string full_description{"Undefined Behavior detected:\n\t"};
    std::vector<am::UB> ubs;
public:
    UBException(std::vector<am::UB> ubs, const std::string& error_detail)
            : IgnorableException(""), ubs(std::move(ubs))
    {
        for (char item: error_detail) {
            this->full_description.push_back(item);
            if (item == '\n') {
                this->full_description.push_back('\t');
            }
        }
        this->full_description.append("\nrelated UB(s):\n");
        for (auto item: this->ubs) {
            this->full_description.append(lib::format("\t${}\n", item));
        }
        this->full_description.pop_back(); // remove last newline character
    }

    [[nodiscard]] const char* what() const noexcept override
    {
        return this->full_description.c_str();
    }

};

} // namespace cami

#endif //CAMI_AM_EXCEPTION_H
