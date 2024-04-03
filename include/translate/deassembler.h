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

#ifndef CAMI_TRANSLATE_DEASSEMBLER_H
#define CAMI_TRANSLATE_DEASSEMBLER_H

#include <iostream>
#include "bytecode.h"
#include <am/fetch_decode.h>
#include <lib/slice.h>

namespace cami::tr {

// MBC ==> TBC
class DeAssembler
{
public:
    static void deassemble(const MBC& mbc, std::ostream& output);
private:
    static void attribute(const MBC::Attribute& attribute, std::ostream& output);
    static void comment(std::string_view comment, std::ostream& output);
    static void types(lib::Slice<const ts::Type* const> types, std::ostream& output);
    static void sourceCoderLocator(lib::Slice<const am::spd::SourceCodeLocator::Item> locator, std::ostream& output);
    static void accessSourceLocation(lib::Slice<const std::pair<uint64_t, uint64_t>> location, std::ostream& output);
    static void sequenceAfterGraph(uint64_t trace_event_cnt, lib::Slice<const uint8_t> graph, std::ostream& output);

    struct Unlinked
    {
        static void object(lib::Slice<const std::unique_ptr<UnlinkedMBC::StaticObject>> objects, std::ostream& output); // NOLINT
        static void function(lib::Slice<const std::unique_ptr<UnlinkedMBC::Function>> functions, std::ostream& output);
        static void code(lib::Slice<const uint8_t> code, lib::Slice<const UnlinkedMBC::RelocateEntry> relocate, std::ostream& output);
        static void block(const UnlinkedMBC::Block& block, std::ostream& output);
        static void fullExpression(const UnlinkedMBC::FullExprInfo& full_expr, std::ostream& output);
    };

    struct Linked
    {
        static void object(const LinkedMBC& linked_mbc, std::ostream& output);
        static void function(const LinkedMBC& linked_mbc, std::ostream& output);
        static void code(const LinkedMBC& linked_mbc, uint64_t addr, uint64_t len, std::ostream& output);
        static void instruction(am::Opcode op, uint64_t info, const LinkedMBC& linked_mbc, std::ostream& output);
        static void constant(const ValueBox& constant, std::ostream& output);
        static void block(const am::spd::Block& block, std::ostream& output);
        static void fullExpression(const am::FullExprInfo& full_expr, std::ostream& output);
    };
};

} // namespace cami::tr

#endif //CAMI_TRANSLATE_DEASSEMBLER_H
