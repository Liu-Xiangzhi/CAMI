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

/// parse data & bss section

#include <assembler.h>
#include <lib/downcast.h>

using namespace cami;
using namespace tr;

void Assembler::parseBSS()
{
    auto token = this->nextToken();
    if (!Token::isString(token) && down_cast<StringToken&>(token).value != "ALIGN") {
        this->diagnostic(token->begin, "expect 'ALIGN'");
        this->skipTo({});
        return;
    }
    uint64_t vals[2];
    for (auto& val: vals) {
        token = this->nextToken();
        if (token->type != Token::Type::integer) {
            this->diagnostic(token->begin, "expect integer");
            this->skipTo({});
            return;
        }
        val = down_cast<IntegerToken&>(token).value;
    }
    this->mbc->bss.align = vals[0];
    this->mbc->bss.len = vals[1];
}

void Assembler::parseDataCommon(MBC::Data& data)
{
    auto token = this->nextToken();
    if (!Token::isString(token) && down_cast<StringToken&>(token).value != "ALIGN") {
        this->diagnostic(token->begin, "expect 'ALIGN'");
        this->skipTo({Token::Type::section_name});
        return;
    }
    token = this->nextToken();
    if (token->type != Token::Type::integer) {
        this->diagnostic(token->begin, "expect integer");
        this->skipTo({});
        return;
    }
    data.align = down_cast<IntegerToken&>(token).value;
    this->parseBin(data.bin);
}

void Assembler::parseBin(lib::Array<uint8_t>& result)
{
    std::vector<uint8_t> hex_sequence;
    while (true) {
        auto token = this->nextToken();
        if (token->type == Token::Type::section_name || token->type == Token::Type::end) {
            this->putBack(std::move(token));
            break;
        }
        if (token->type == Token::Type::hex_sequence) {
            auto& seq = down_cast<HexSequenceToken&>(token).value;
            hex_sequence.insert(hex_sequence.end(), seq.begin(), seq.end());
        } else if (Token::isString(token)) {
            auto& str = down_cast<StringToken&>(token).value;
            hex_sequence.insert(hex_sequence.end(), str.begin(), str.end());
        } else {
            this->diagnostic(token->begin, "expect hex_sequence or string");
        }
    }
    result.assign(lib::Array<uint8_t>::fromVector(std::move(hex_sequence)));
}
