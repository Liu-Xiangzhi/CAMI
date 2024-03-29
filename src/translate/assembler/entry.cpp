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

#include <assembler.h>
#include <exception.h>
#include <lib/downcast.h>
#include <lib/format.h>

using namespace cami;
using namespace tr;
using am::spd::StaticObjectDescription;
using std::operator ""sv;

std::unique_ptr<MBC> Assembler::assemble(std::string_view tbc, std::string_view name)
{
    this->reset(tbc, name);
    auto result = std::make_unique<MBC>();
    this->mbc = result.get();
    for (auto token = this->nextToken(); token->type != Token::Type::end; token = this->nextToken()) {
        this->putBack(std::move(token));
        this->parseSection();
    }
    if (this->has_error || this->lexer.hasError()) {
        throw AssemblyException{};
    }
    this->mbc->types.assign(lib::Array<const ts::Type*>::fromSet(this->parsed_types));
    this->mbc->constants.assign(lib::Array<std::pair<const ts::Type*, uint64_t>>::fromSet(this->parsed_constants));
    return result;
}

void Assembler::parseSection()
{
    static const std::map<std::string_view, void (Assembler::*)()> mapper{
            {".attribute"sv,              &Assembler::parseAttribute},
            {".comment"sv,                &Assembler::parseComment},
            {".types"sv,                  &Assembler::parseTypes},
            {".code"sv,                   &Assembler::parseCode},
            {".code.init"sv,              &Assembler::parseInitCode},
            {".code.thread_local_init"sv, &Assembler::parseThreadLocalInitCode},
            {".object"sv,                 &Assembler::parseObjects},
            {".object.bss"sv,             &Assembler::parseBSSObjects},
            {".object.string_literal"sv,  &Assembler::parseStringLiteralObjects},
            {".object.thread_local"sv,    &Assembler::parseThreadLocalObjects},
            {".function"sv,               &Assembler::parseFunctions},
            {".bss"sv,                    &Assembler::parseBSS},
            {".data"sv,                   &Assembler::parseData},
            {".data.string_literal"sv,    &Assembler::parseStringLiteralData},
            {".data.thread_local"sv,      &Assembler::parseThreadLocalData},
            {".stack_init"sv,             &Assembler::parseStackInitData},
    };
    auto token = this->nextToken();
    if (token->type != Token::Type::section_name) {
        this->diagnostic(token->begin, "expect section name");
        this->skipTo({});
        return;
    }
    auto itr = mapper.find(down_cast<StringToken&>(token).value);
    if (itr == mapper.end()) {
        this->diagnostic(token->begin, lib::format("unknown section name '${}', current section ignored",
                                                   down_cast<StringToken&>(token).value));
        this->skipTo({});
        return;
    }
    (this->*itr->second)();
}
