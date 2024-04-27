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
#include <lib/downcast.h>
#include <lib/format.h>

using namespace cami;
using namespace tr;

/// parse attribute & comment section

void Assembler::parseAttribute()
{
    this->has_attribute = true;
    const auto attribute_section_offset = this->peek().begin;
    this->mbc->attribute.type = static_cast<MBC::Type>(-1);
    while (true) {
        auto token = this->nextToken();
        if (token->type == Token::Type::section_name || token->type == Token::Type::end) {
            this->putBack(std::move(token));
            break;
        }
        if (!Token::isString(token)) {
            this->diagnostic(token->begin, "expect string");
            this->skipTo({Token::Type::quote_string, Token::Type::unquote_string});
            continue;
        }
        auto& str = down_cast<StringToken&>(token).value;
        if (str == "VERSION") {
            if (!Token::isString(token = this->nextToken())) {
                this->diagnostic(token->begin, "expect string");
            } else {
                this->mbc->attribute.version = std::move(down_cast<StringToken&>(token).value);
            }
        } else if (str == "OBJECT") {
            this->mbc->attribute.type = MBC::Type::object_file;
        } else if (str == "SHARED_OBJECT") {
            this->mbc->attribute.type = MBC::Type::shared_object;
        } else if (str == "EXECUTABLE") {
            this->mbc->attribute.type = MBC::Type::executable;
        } else if (str == "ENTRY" || str == "MODULE_NAME") {
            if (!Token::isString(token = this->nextToken())) {
                this->diagnostic(token->begin, "expect string");
                continue;
            }
            this->mbc->attribute.module_or_entry_name = down_cast<StringToken&>(token).value;
        } else if (str == "STATIC_LINK") {
            this->parseLinks(this->mbc->attribute.static_links);
        } else if (str == "DYNAMIC_LINK") {
            this->parseLinks(this->mbc->attribute.dynamic_links);
        } else {
            this->diagnostic(token->begin, lib::format("unknown attribute key '${}'", str));
        }
    }
    if (this->mbc->attribute.version.empty()) {
        this->diagnostic(attribute_section_offset, "missing or empty filed `version`");
    }
    if (this->mbc->attribute.type == static_cast<MBC::Type>(-1)) {
        this->diagnostic(attribute_section_offset, "missing filed `type`");
    }
}

void Assembler::parseComment()
{
    while (true) {
        auto token = this->nextToken();
        if (token->type == Token::Type::section_name || token->type == Token::Type::end) {
            this->putBack(std::move(token));
            return;
        }
        if (!Token::isString(token)) {
            this->diagnostic(token->begin, "expect string in comment section");
            continue;
        }
        this->mbc->comment.append(down_cast<StringToken&>(token).value);
    }
}

void Assembler::parseLinks(std::vector<std::string>& result)
{
    auto token = this->nextToken();
    if (token->type != Token::Type::lbracket) {
        this->diagnostic(token->begin, "expect '['");
        return;
    }
    std::vector<std::string> files;
    while ((token = this->nextToken())->type != Token::Type::rbracket) {
        if (token->type == Token::Type::section_name || token->type == Token::Type::end) [[unlikely]] {
            this->diagnostic(token->begin, "unclosed bracket");
            return;
        }
        if (!Token::isString(token)) {
            this->diagnostic(token->begin, "expect string");
            this->skipTo({Token::Type::quote_string, Token::Type::unquote_string, Token::Type::rbracket});
            continue;
        }
        result.push_back(std::move(down_cast<StringToken&>(token).value));
        if (this->peek().type == Token::Type::comma) {
            this->nextToken();
        }
    }
}
