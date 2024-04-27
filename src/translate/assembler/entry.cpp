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
#include <linker.h>
#include <filesystem>

using namespace cami;
using namespace tr;
using am::spd::StaticObjectDescription;
using std::operator ""sv;

std::unique_ptr<MBC> Assembler::assemble(std::string_view tbc, std::string_view name)
{
    this->reset(tbc, name);
    auto result = std::make_unique<UnlinkedMBC>();
    this->mbc = result.get();
    for (auto token = this->nextToken(); token->type != Token::Type::end; token = this->nextToken()) {
        this->putBack(std::move(token));
        this->parseSection();
    }
    this->postprocess(name);
    if (auto type = result->attribute.type; type == MBC::Type::executable || type == MBC::Type::shared_object) {
        result->attribute.type = MBC::Type::object_file;
        std::vector<std::unique_ptr<UnlinkedMBC>> mbcs{};
        mbcs.push_back(down_cast<std::unique_ptr<UnlinkedMBC>>(std::move(result)));
        return Linker::link(std::move(mbcs), {type});
    }
    return result;
}

void Assembler::parseSection()
{
    static const std::map<std::string_view, void (Assembler::*)()> mapper{
            {".attribute"sv, &Assembler::parseAttribute},
            {".comment"sv,   &Assembler::parseComment},
            {".type"sv,      &Assembler::parseTypes},
            {".object"sv,    &Assembler::parseObjects},
            {".function"sv,  &Assembler::parseFunctions},
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

void Assembler::postprocess(std::string_view name)
{
    if (!this->has_attribute) {
        this->diagnostic(0, "missing attribute section");
    }
    if (this->has_error || this->lexer.hasError()) {
        throw AssemblyException{};
    }
    this->mbc->types.insert(this->mbc->types.end(), this->parsed_types.begin(), this->parsed_types.end());
    this->mbc->constants.insert(this->mbc->constants.end(), this->parsed_constants.begin(), this->parsed_constants.end());
    auto bc_dir = std::filesystem::absolute(name);
    this->mbc->source_name = bc_dir;
    for (auto& item: this->mbc->attribute.static_links) {
        auto link_file_path = std::filesystem::path{std::move(item)};
        if (!link_file_path.is_absolute()) {
            link_file_path = std::filesystem::weakly_canonical(bc_dir.parent_path() / link_file_path);
        }
        item = std::move(link_file_path);
    }
}
