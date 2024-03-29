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

namespace cami::tr {

// MBC ==> TBC
class DeAssembler
{
public:
    static void deassemble(const MBC& mbc, std::ostream& output);
private:
    static void dasAttribute(const MBC::Attribute& attribute, std::ostream& output);
    static void dasComment(std::string_view comment, std::ostream& output);
    static void dasTypes(const lib::Array<const ts::Type*>& types, std::ostream& output);
    static void dasCode(std::string_view section_name, const MBC& mbc, const MBC::Code& code, std::ostream& output);
    static void dasConstant(std::pair<const ts::Type*, uint64_t> constant, std::ostream& output);
    static void dasLinkedFileInstr(am::Opcode op, uint64_t info, const MBC& mbc, std::ostream& output);
    static std::set<uint64_t> getLabelOffsets(const MBC::Code& code);
    static void dasBSS(const MBC::BSS& bss, std::ostream& output);
    static void dasData(std::string_view section_name, const MBC::Data& data, std::ostream& output,
                        bool to_string = false);
    static void dasObject(std::string_view section_name, const lib::Array<am::spd::StaticObjectDescription>& objects,
                          std::ostream& output);
    static void dasFunction(const lib::Array<am::spd::Function>& functions, std::ostream& output);
    static void dasBlock(const am::spd::Block& block, std::ostream& output);
    static void dasFullExpression(const am::FullExprInfo& full_expr, std::ostream& output);
    static void dasSourceLocation(const am::spd::SourceCodeLocator& locator, std::ostream& output);
};

} // namespace cami::tr

#endif //CAMI_TRANSLATE_DEASSEMBLER_H
