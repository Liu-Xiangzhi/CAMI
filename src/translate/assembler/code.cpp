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

/// parse code & type section

#include <assembler.h>
#include <am/fetch_decode.h>
#include <foundation/type/helper.h>
#include <lib/downcast.h>
#include <lib/format.h>

using namespace cami;
using namespace tr;
using RelocateEntry = UnlinkedMBC::RelocateEntry;

void Assembler::parseTypes()
{
    const auto isStructOrUnion = [](const std::unique_ptr<Token>& token) {
        auto& str = down_cast<const StringToken&>(token).value;
        return str == "struct" || str == "union";
    };
    while (true) {
        auto token = this->nextToken();
        if (token->type == Token::Type::section_name || token->type == Token::Type::end) {
            this->putBack(std::move(token));
            return;
        }
        if (!Token::isString(token) || !isStructOrUnion(token)) {
            this->diagnostic(token->begin, "expect 'struct' or 'union'");
            this->skipTo({Token::Type::quote_string, Token::Type::unquote_string});
            continue;
        }
        const bool is_struct = down_cast<StringToken&>(token).value == "struct";
        if (!Token::isString(token = this->nextToken())) {
            this->diagnostic(token->begin, "expect string");
            this->skipTo({Token::Type::quote_string, Token::Type::unquote_string});
            continue;
        }
        std::string str = std::move(down_cast<StringToken&>(token).value);
        this->parseTypeDefine(is_struct, std::move(str));
    }
}

std::pair<std::vector<uint8_t>, std::vector<RelocateEntry>> Assembler::parseCode()
{
    std::map<std::string, uint64_t> labels;
    std::vector<uint8_t> code;
    std::vector<RelocateEntry> relocate;
    // relocate_instr_offset, symbol_name, symbol_offset in file
    std::vector<std::tuple<uint64_t, std::string, uint64_t>> relocate_label;
    while (true) {
        auto token = this->nextToken();
        if (token->type == Token::Type::section_name || token->type == Token::Type::end) {
            this->putBack(std::move(token));
            break;
        }
        if (token->type == Token::Type::dot) {
            break;
        }
        if (token->type != Token::Type::unquote_string) {
            this->diagnostic(token->begin, "expect unquoted string");
            continue;
        }
        auto next_token = this->nextToken();
        if (next_token->type == Token::Type::colon) {
            auto [itr, inserted] = labels.emplace(
                    std::piecewise_construct, std::forward_as_tuple(std::move(down_cast<StringToken&>(token).value)),
                    std::forward_as_tuple(code.size()));
            if (!inserted) {
                this->diagnostic(token->begin, lib::format("duplicated flag: '${}'", itr->first));
            }
            continue;
        }
        this->putBack(std::move(next_token));
        this->putBack(std::move(token));
        this->parseInstr(code, relocate, relocate_label);
    }
    // relocate label
    for (const auto& item: relocate_label) {
        auto itr = labels.find(std::get<1>(item));
        if (itr == labels.end()) {
            this->diagnostic(std::get<2>(item), lib::format("undefined symbol: '${}'", std::get<1>(item)));
            continue;
        }
        auto relocate_instr_offset = std::get<0>(item);
        auto source_offset = relocate_instr_offset + 4;
        auto target_offset = itr->second;
        auto jmp_offset = target_offset - source_offset;
        auto relocate_point = code.data() + relocate_instr_offset + 1;
        lib::write<3>(relocate_point, jmp_offset);
    }
    return {std::move(code), std::move(relocate)};
}

void Assembler::parseTypeDefine(bool is_struct, std::string struct_or_union_name)
{
    auto token = this->nextToken();
    if (token->type != Token::Type::lbrace) {
        this->diagnostic(token->begin, "expect '{'");
        return;
    }
    std::vector<const ts::Type*> types;
    while (true) {
        token = this->nextToken();
        if (token->type == Token::Type::section_name || token->type == Token::Type::end) {
            this->diagnostic(token->begin, "unclosed brace");
            this->putBack(std::move(token));
            return;
        }
        if (token->type == Token::Type::rbrace) {
            break;
        }
        this->putBack(std::move(token));
        if (auto type = this->parseTypeSpecifier(); type) {
            types.push_back(*type);
            if ((token = this->nextToken())->type != Token::Type::semicolon) {
                this->diagnostic(token->begin, "expect ';'");
            }
        } else {
            this->skipTo({Token::Type::semicolon});
        }
    }
    auto member_type = lib::Array<const ts::Type*>::fromVector(std::move(types));
    if (is_struct) {
        ts::type_manager.createStruct(std::move(struct_or_union_name), std::move(member_type));
    } else {
        ts::type_manager.createUnion(std::move(struct_or_union_name), std::move(member_type));
    }
}

void Assembler::parseInstr(std::vector<uint8_t>& code_bin,
                           std::vector<UnlinkedMBC::RelocateEntry>& relocate,
                           std::vector<std::tuple<uint64_t, std::string, uint64_t>>& relocate_label)
{
    using am::Opcode;
    using am::InstrInfo;
    static const std::map<std::string_view, Opcode> one_byte_opcode{
            {"drf"sv,   Opcode::drf},
            {"mdfi"sv,  Opcode::mdfi},
            {"zeroi"sv, Opcode::zeroi},
            {"lb"sv,    Opcode::lb},
            {"ij"sv,    Opcode::ij},
            {"ret"sv,   Opcode::ret},
            {"nop"sv,   Opcode::nop},
            {"pushu"sv, Opcode::pushu},
            {"pop"sv,   Opcode::pop},
            {"dup"sv,   Opcode::dup},
            {"halt"sv,  Opcode::halt},
            {"addr"sv,  Opcode::addr},
            {"cpl"sv,   Opcode::cpl},
            {"pos"sv,   Opcode::pos},
            {"neg"sv,   Opcode::neg},
            {"not"sv,   Opcode::not_},
            {"mul"sv,   Opcode::mul},
            {"div"sv,   Opcode::div},
            {"mod"sv,   Opcode::mod},
            {"add"sv,   Opcode::add},
            {"sub"sv,   Opcode::sub},
            {"ls"sv,    Opcode::ls},
            {"rs"sv,    Opcode::rs},
            {"sl"sv,    Opcode::sl},
            {"sle"sv,   Opcode::sle},
            {"sg"sv,    Opcode::sg},
            {"sge"sv,   Opcode::sge},
            {"seq"sv,   Opcode::seq},
            {"sne"sv,   Opcode::sne},
            {"and"sv,   Opcode::and_},
            {"or"sv,    Opcode::or_},
            {"xor"sv,   Opcode::xor_}};
    static const std::map<std::string_view, Opcode> four_bytes_opcode{
            {"dsg"sv,   Opcode::dsg},
            {"read"sv,  Opcode::read},
            {"mdf"sv,   Opcode::mdf},
            {"zero"sv,  Opcode::zero},
            {"eb"sv,    Opcode::eb},
            {"new"sv,   Opcode::new_},
            {"del"sv,   Opcode::del},
            {"fe"sv,    Opcode::fe},
            {"j"sv,     Opcode::j},
            {"jst"sv,   Opcode::jst},
            {"jnt"sv,   Opcode::jnt},
            {"call"sv,  Opcode::call},
            {"push"sv,  Opcode::push},
            {"dot"sv,   Opcode::dot},
            {"arrow"sv, Opcode::arrow},
            {"cast"sv,  Opcode::cast}};
    auto token = this->nextToken();
    ASSERT(token->type == Token::Type::unquote_string, "precondition violation");
    auto& instr = down_cast<StringToken&>(token).value;
    if (auto itr = one_byte_opcode.find(instr);itr != one_byte_opcode.end()) {
        code_bin.push_back(static_cast<uint8_t>(itr->second));
        return;
    }
    auto itr = four_bytes_opcode.find(instr);
    if (itr == four_bytes_opcode.end()) {
        this->diagnostic(token->begin, lib::format("unknown instruction '${}'", instr));
        return;
    }
    code_bin.push_back(static_cast<uint8_t>(itr->second));
    code_bin.push_back(-1);
    code_bin.push_back(-1);
    code_bin.push_back(-1);
    if (instr == "dsg") {
        token = this->nextToken();
        if (token->type == Token::Type::integer) {
            auto id = InstrInfo::IdentifierId::fromAutomaticObject(down_cast<IntegerToken&>(token).value);
            lib::write<3>(&code_bin.back() - 2, id);
            return;
        }
        if ((token)->type != Token::Type::unquote_string) {
            this->diagnostic(token->begin, "expect unquoted string or integer");
            return;
        }
        relocate.emplace_back(code_bin.size() - 4, std::move(down_cast<StringToken&>(token).value));
    } else if (instr == "new" || instr == "cast") {
        if (auto type = this->parseTypeSpecifier(); type) {
            this->parsed_types.insert(*type);
            relocate.emplace_back(code_bin.size() - 4, lib::format("#${}", **type));
        }
    } else if (instr == "push") {
        if (auto constant = this->parseConstant(); constant) {
            this->parsed_constants.insert(*constant);
            relocate.emplace_back(code_bin.size() - 4,
                                  lib::format("<${}; ${}>", *(*constant).first, (*constant).second));
        }
    } else if (instr == "j" || instr == "jst" || instr == "jnt") {
        if ((token = this->nextToken())->type != Token::Type::unquote_string) {
            this->diagnostic(token->begin, "expect unquoted string");
            return;
        }
        relocate_label.emplace_back(code_bin.size() - 4, std::move(down_cast<StringToken&>(token).value),
                                    token->begin);
    } else {
        if ((token = this->nextToken())->type != Token::Type::integer) {
            this->diagnostic(token->begin, "expect unquoted integer");
            return;
        }
        auto val = down_cast<IntegerToken&>(token).value;
        lib::write<3>(&code_bin.back() - 2, val);
    }
}

lib::Optional<const ts::Type*> Assembler::parseTypeSpecifier()
{
    auto token = this->nextToken();
    if (token->type == Token::Type::unquote_string && down_cast<StringToken&>(token).value == "null") {
        return &ts::type_manager.getBasicType(ts::Kind::null);
    }
    this->putBack(std::move(token));
    return this->do_parseTypeSpecifier();
}

lib::Optional<const ts::Type*> Assembler::do_parseTypeSpecifier() // NOLINT
{
    using namespace ts;
    const Type* type = nullptr;
    while (true) {
        auto token = this->nextToken();
        lib::Optional<const Type*> derived_type;
        switch (token->type) {
        case Token::Type::star:
            if (type == nullptr) {
                this->diagnostic(token->begin, "expect basic type");
                return {};
            }
            derived_type = &type_manager.getPointer(*type);
            break;
        case Token::Type::lbracket:
            if (type == nullptr) {
                this->diagnostic(token->begin, "expect basic type");
                return {};
            }
            derived_type = this->parseTypeArray(type);
            break;
        case Token::Type::lparen:
            derived_type = this->parseTypeFunction();
            break;
        case Token::Type::arrow:
            derived_type = this->parseTypeFunction({type});
            break;
        case Token::Type::unquote_string: {
            auto [parsed_type, terminate] = this->parseTypeBaseOrQualify(type, std::move(token));
            derived_type = parsed_type;
            if (terminate) {
                return type;
            }
            break;
        }
        default:
            this->putBack(std::move(token));
            return type;
        }
        if (!derived_type) {
            return {};
        }
        type = *derived_type;
    }
}

lib::Optional<const ts::Type*> Assembler::parseTypeArray(const ts::Type* base_type)
{
    using namespace ts;
    auto token = this->nextToken();
    if ((token)->type != Token::Type::integer) {
        this->diagnostic(token->begin, "expect integer");
        return {};
    }
    if (removeQualify(*base_type).kind() == Kind::function || removeQualify(*base_type).kind() == Kind::void_) {
        this->diagnostic(token->begin, "cannot make array of 'void' or function type");
    } else {
        base_type = &type_manager.getArray(*base_type, down_cast<IntegerToken&>(token).value);
    }
    if ((token = this->nextToken())->type != Token::Type::rbracket) {
        this->diagnostic(token->begin, "expect ']'");
    }
    return base_type;
}

lib::Optional<const ts::Type*> Assembler::parseTypeFunction() // NOLINT
{
    using namespace ts;
    auto token = this->nextToken();
    lib::Optional<const Type*> sub_type{};
    if (token->type != Token::Type::rparen) {
        this->putBack(std::move(token));
        sub_type = this->do_parseTypeSpecifier();
        if (!sub_type) {
            return {};
        }
        token = this->nextToken();
        if (token->type == Token::Type::rparen) {
            return *sub_type;
        }
    }
    std::vector<const Type*> sub_types{};
    if (sub_type) {
        sub_types.push_back(*sub_type);
    }
    while (token->type != Token::Type::rparen) {
        if (token->type == Token::Type::section_name || token->type == Token::Type::end) {
            this->diagnostic(token->begin, "incomplete type specifier");
            this->putBack(std::move(token));
            return {};
        }
        if (token->type != Token::Type::comma) {
            this->diagnostic(token->begin, "expect ','");
            return {};
        }
        sub_type = this->do_parseTypeSpecifier();
        if (!sub_type) {
            return {};
        }
        sub_types.push_back(*sub_type);
        token = this->nextToken();
    }
    if ((token = this->nextToken())->type != Token::Type::arrow) {
        this->diagnostic(token->begin, "expect '->'");
        return {};
    }
    return this->parseTypeFunction(sub_types);
}

lib::Optional<const ts::Type*> Assembler::parseTypeFunction(const std::vector<const ts::Type*>& params) // NOLINT
{
    using namespace ts;
    auto ret_type = this->do_parseTypeSpecifier();
    if (!ret_type) {
        return {};
    }
    return &type_manager.getFunction(**ret_type, lib::Array<const Type*>::fromVector(params));
}

std::pair<lib::Optional<const ts::Type*>, bool> Assembler::parseTypeBaseOrQualify(const ts::Type* base_type, std::unique_ptr<Token> token)
{
    using namespace ts;
    static const std::map<std::string_view, Kind> basics{
            {"bool", Kind::bool_},
            {"char", Kind::char_},
            {"i8",   Kind::i8},
            {"i16",  Kind::i16},
            {"i32",  Kind::i32},
            {"i64",  Kind::i64},
            {"u8",   Kind::u8},
            {"u16",  Kind::u16},
            {"u32",  Kind::u32},
            {"u64",  Kind::u64},
            {"f32",  Kind::f32},
            {"f64",  Kind::f64},
            {"void", Kind::void_},
    };
    static const std::map<std::string_view, Qualifier> qualifiers{
            {"const",    Qualifier::const_},
            {"volatile", Qualifier::volatile_},
            {"restrict", Qualifier::restrict},
            {"atomic",   Qualifier::atomic},
    };
    auto& str = down_cast<StringToken&>(token).value;
    if (auto itr = basics.find(str);itr != basics.end()) {
        if (base_type != nullptr) {
            this->diagnostic(token->begin, "expect type derivation");
            return {{}, false};
        }
        return {&type_manager.getBasicType(itr->second), false};
    }
    if (auto itr = qualifiers.find(str);itr != qualifiers.end()) {
        if (base_type == nullptr) {
            this->diagnostic(token->begin, "expect basic type");
            return {};
        }
        auto qualifier = itr->second;
        while ((token = this->nextToken())->type == Token::Type::unquote_string &&
               (itr = qualifiers.find(down_cast<StringToken&>(token).value)) != qualifiers.end()) {
            qualifier = qualifier | itr->second;
        }
        this->putBack(std::move(token));
        return {&type_manager.getQualifiedType(*base_type, qualifier), false};
    }
    if (str == "struct" || str == "union") {
        if ((token = this->nextToken())->type != Token::Type::unquote_string) {
            this->diagnostic(token->begin, "expect unquoted string");
            return {};
        }
        auto& name = down_cast<StringToken&>(token).value;
        if (str == "struct") {
            return {&type_manager.declareStruct(std::move(name)), false};
        }
        return {&type_manager.declareUnion(std::move(name)), false};
    }
    this->putBack(std::move(token));
    return {{}, true};
}

constant_opt_t Assembler::parseConstant()
{
    auto token = this->nextToken();
    const auto constant_begin = token->begin;
    if (token->type != Token::Type::less) {
        this->diagnostic(token->begin, "expect '<'");
        this->skipTo({Token::Type::great});
        return {};
    }
    auto type = this->parseTypeSpecifier();
    if (!type) {
        this->skipTo({Token::Type::great});
        return {};
    }
    if ((token = this->nextToken())->type != Token::Type::semicolon) {
        this->diagnostic(token->begin, "expect ';'");
        this->skipTo({Token::Type::great});
        return {};
    }
    auto val = [&]() -> uint64_t {
        using namespace ts;
        auto token = this->nextToken();
        switch ((*type)->kind()) {
        case Kind::f32: {
            if (token->type != Token::Type::floating) {
                this->diagnostic(token->begin, "expect floating type");
                return 0;
            }
            uint64_t res = 0;
            auto token_val = static_cast<float>(down_cast<FloatingToken&>(token).value);
            std::memcpy(&res, &token_val, 4);
            return res;
        }
        case Kind::f64: {
            if (token->type != Token::Type::floating) {
                this->diagnostic(token->begin, "expect floating type");
                return 0;
            }
            uint64_t res;
            auto token_val = down_cast<FloatingToken&>(token).value;
            std::memcpy(&res, &token_val, 8);
            return res;
        }
        case Kind::null:
            return 0;
        default:
            if (!isInteger((*type)->kind())) {
                this->diagnostic(constant_begin, lib::format("invalid constant type `${}`", **type));
                return 0;
            }
            return down_cast<IntegerToken&>(token).value;
        }
    }();
    if ((token = this->nextToken())->type != Token::Type::great) {
        this->diagnostic(token->begin, "expect '>'");
        return {};
    }
    return std::pair{*type, val};
}
