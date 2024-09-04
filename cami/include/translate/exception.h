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

#ifndef CAMI_TRANSLATE_EXCEPTION_H
#define CAMI_TRANSLATE_EXCEPTION_H

#include <stdexcept>
#include <lib/format.h>
#include <am/fetch_decode.h>
#include <foundation/type/def.h>

namespace cami {
class AssemblyException : public std::runtime_error
{
public:
    explicit AssemblyException() : std::runtime_error("assembly aborted due to errors") {}
};

class LinkException : public std::runtime_error
{
public:
    explicit LinkException(std::string_view what)
            : std::runtime_error(std::string{"link error: "}.append(what)) {}
};

class MissingModuleOrEntryNameException : public LinkException
{
public:
    MissingModuleOrEntryNameException() : LinkException("missing module or entry name") {}
};

class ConflictModuleOrEntryException : public LinkException
{
public:
    ConflictModuleOrEntryException() : LinkException("conflict module or entry names") {}
};

class CannotFoundSymbolException : public LinkException
{
public:
    explicit CannotFoundSymbolException(std::string_view symbol)
            : LinkException(lib::format("cannot find symbol `${}`", symbol)) {}
};

class CannotRelocateException : public LinkException
{
public:
    explicit CannotRelocateException(am::Opcode op)
            : LinkException(lib::format("cannot relocate instruction symbol `${}`", op)) {}
};

class CannotMakeConstantException : public LinkException
{
public:
    explicit CannotMakeConstantException(const ts::Type& type)
            : LinkException(lib::format("cannot spawn due to invalid constant type `${}`", type)) {}
};

class DuplicatedSymbolException : public LinkException
{
public:
    explicit DuplicatedSymbolException(std::string_view symbol)
            : LinkException(lib::format("duplicated symbol `${}`", symbol)) {}
};

} // namespace cami

#endif //CAMI_TRANSLATE_EXCEPTION_H
