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

/// parse object & function section

#include <translate/assembler.h>
#include <lib/downcast.h>
#include <lib/format.h>

using namespace cami;
using namespace tr;
using am::spd::StaticObjectDescription;
using am::spd::Function;
using am::spd::Block;
using am::spd::AutomaticObjectDescription;
using am::spd::SourceCodeLocator;
using am::FullExprInfo;

template<typename T>
struct tr::ParseFunctionTrait
{
};
template<>
struct tr::ParseFunctionTrait<am::spd::Function>
{
    static constexpr auto value = &Assembler::parseFunction;
};
template<>
struct tr::ParseFunctionTrait<am::spd::StaticObjectDescription>
{
    static constexpr auto value = &Assembler::parseStaticObject;
};
template<>
struct tr::ParseFunctionTrait<am::spd::AutomaticObjectDescription>
{
    static constexpr auto value = &Assembler::parseAutoObject;
};
template<>
struct tr::ParseFunctionTrait<am::spd::Block>
{
    static constexpr auto value = &Assembler::parseBlock;
};
template<>
struct tr::ParseFunctionTrait<am::FullExprInfo>
{
    static constexpr auto value = &Assembler::parseFullExprInfo;
};
template<>
struct tr::ParseFunctionTrait<std::pair<uint64_t, uint64_t>>
{
    static constexpr auto value = &Assembler::parseFullExprSourceLocationPair;
};
template<>
struct tr::ParseFunctionTrait<am::spd::SourceCodeLocator::Item>
{
    static constexpr auto value = &Assembler::parseDebugLocInfo;
};

template<typename T>
lib::Array<T> Assembler::parseLists()
{
    auto token = this->nextToken();
    if (token->type != Token::Type::lbracket) {
        this->diagnostic(token->begin, "expect '['");
        this->skipTo({Token::Type::lbracket});
        this->nextToken();
    }
    if ((token = this->nextToken())->type == Token::Type::rbracket) {
        return {};
    }
    this->putBack(std::move(token));
    std::vector<T> sub_elements;
    while (true) {
        sub_elements.push_back((this->*ParseFunctionTrait<T>::value)());
        token = this->nextToken();
        if (token->type == Token::Type::section_name || token->type == Token::Type::end) {
            this->diagnostic(token->begin, "unclosed bracket");
            this->putBack(std::move(token));
            break;
        }
        if (token->type == Token::Type::rbracket) {
            break;
        }
        this->putBack(std::move(token));
    }
    return lib::Array<T>::fromVector(std::move(sub_elements));
}

void Assembler::parseObjectsCommon(lib::Array<StaticObjectDescription>& result)
{
    result.assign(this->parseLists<StaticObjectDescription>());
}

void Assembler::parseFunctionsCommon(lib::Array<Function>& result)
{
    result.assign(this->parseLists<Function>());
}

StaticObjectDescription Assembler::parseStaticObject()
{
    StaticObjectDescription result;
    auto token = this->nextToken();
    if (token->type != Token::Type::lbrace) {
        this->diagnostic(token->begin, "expect '{'");
        this->skipTo({Token::Type::lbrace});
        this->nextToken();
    }
    this->parseKeyValuePair("name"sv, result.name);
    this->parseKeyValuePair("address"sv, result.address);
    this->parseKeyValuePair("type"sv, result.type);
    if ((token = this->nextToken())->type != Token::Type::rbrace) {
        this->diagnostic(token->begin, "expect '}'");
        this->skipTo({Token::Type::rbrace});
        this->nextToken();
    }
    return result;
}

Function Assembler::parseFunction()
{
    auto token = this->nextToken();
    if (token->type != Token::Type::lbrace) {
        this->diagnostic(token->begin, "expect '{'");
        this->skipTo({Token::Type::lbrace});
        this->nextToken();
    }
    using ts::Type;
    std::string name;
    std::string func_file_name;
    uint64_t address{};
    uint64_t frame_size{};
    uint64_t code_size{};
    uint64_t max_object_num{};
    const Type* type{};
    this->parseKeyValuePair("name"sv, name);
    this->parseKeyValuePair("address"sv, address);
    this->parseKeyValuePair("type"sv, type);
    this->parseKeyValuePair("frame_size"sv, frame_size);
    this->parseKeyValuePair("code_size"sv, code_size);
    this->parseKeyValuePair("max_object_num"sv, max_object_num);
    this->parseKeyValuePair("file_name"sv, func_file_name);
    auto blocks = this->parseBlocks();
    auto full_expr_info = this->parseFullExprInfos();
    auto debug_loc_info = this->parseDebugLocInfos();
    for (auto& item: debug_loc_info.data) {
        item.addr += address;
    }
    if ((token = this->nextToken())->type != Token::Type::rbrace) {
        this->diagnostic(token->begin, "expect '}'");
        this->skipTo({Token::Type::rbrace});
        this->nextToken();
    }
    return {std::move(name), *type, address, std::move(func_file_name), frame_size, code_size, max_object_num,
            std::move(blocks), std::move(full_expr_info), {}};
}

lib::Array<Block> Assembler::parseBlocks()
{
    if (!this->parseKeyValueCommonCheck("blocks")) {
        return {};
    }
    return this->parseLists<Block>();
}

Block Assembler::parseBlock()
{
    return Block{this->parseLists<AutomaticObjectDescription>()};
}

AutomaticObjectDescription Assembler::parseAutoObject()
{
    using ts::Type;
    auto token = this->nextToken();
    if (token->type != Token::Type::lbrace) {
        this->diagnostic(token->begin, "expect '{'");
        this->skipTo({Token::Type::lbrace});
        this->nextToken();
    }
    std::string name{};
    uint64_t dsg_id{};
    const Type* type{};
    uint64_t offset{};
    uint64_t init_data_offset{};
    this->parseKeyValuePair("name"sv, name);
    this->parseKeyValuePair("dsg_id"sv, dsg_id);
    this->parseKeyValuePair("type"sv, type);
    this->parseKeyValuePair("offset"sv, offset);
    this->parseKeyValuePair("init_data_offset"sv, init_data_offset);
    if ((token = this->nextToken())->type != Token::Type::rbrace) {
        this->diagnostic(token->begin, "expect '}'");
        this->skipTo({Token::Type::rbrace});
        this->nextToken();
    }
    return {std::move(name), dsg_id, *type, offset, init_data_offset};
}

lib::Array<FullExprInfo> Assembler::parseFullExprInfos()
{
    if (!this->parseKeyValueCommonCheck("full_expressions")) {
        return {};
    }
    return this->parseLists<FullExprInfo>();
}

FullExprInfo Assembler::parseFullExprInfo()
{
    uint64_t trace_event_cnt{};
    auto token = this->nextToken();
    if (token->type != Token::Type::lbrace) {
        this->diagnostic(token->begin, "expect '{'");
        this->skipTo({Token::Type::lbrace});
        this->nextToken();
    }
    this->parseKeyValuePair("trace_event_cnt"sv, trace_event_cnt);
    auto locations = this->parseFullExprSourceLocation();
    auto graph = this->parseFullExprSequenceAfterGraph(trace_event_cnt);
    if ((token = this->nextToken())->type != Token::Type::rbrace) {
        this->diagnostic(token->begin, "expect '}'");
        this->skipTo({Token::Type::rbrace});
        this->nextToken();
    }
    return {trace_event_cnt, std::move(graph), std::move(locations)};
}

lib::Array<std::pair<uint64_t, uint64_t>> Assembler::parseFullExprSourceLocation()
{
    if (!this->parseKeyValueCommonCheck("source_location")) {
        return {};
    }
    return this->parseLists<std::pair<uint64_t, uint64_t>>();
}

std::pair<uint64_t, uint64_t> Assembler::parseFullExprSourceLocationPair()
{
    auto token = this->nextToken();
    if (token->type != Token::Type::lparen) {
        this->diagnostic(token->begin, "expect '('");
        this->skipTo({Token::Type::lparen});
        this->nextToken();
    }
    std::pair<uint64_t, uint64_t> result{};
    if ((token = this->nextToken())->type != Token::Type::integer) {
        this->diagnostic(token->begin, "expect integer");
    } else {
        result.first = down_cast<IntegerToken&>(token).value;
    }
    if ((token = this->nextToken())->type != Token::Type::comma) {
        this->diagnostic(token->begin, "expect ','");
    }
    if ((token = this->nextToken())->type != Token::Type::integer) {
        this->diagnostic(token->begin, "expect integer");
    } else {
        result.second = down_cast<IntegerToken&>(token).value;
    }
    if ((token = this->nextToken())->type != Token::Type::rparen) {
        this->diagnostic(token->begin, "expect ')'");
        this->skipTo({Token::Type::rparen});
        this->nextToken();
    }
    return result;
}

lib::Array<uint8_t> Assembler::parseFullExprSequenceAfterGraph(uint64_t trace_event_cnt)
{
    if (!this->parseKeyValueCommonCheck("sequence_after")) {
        return {};
    }
    auto token = this->nextToken();
    if (token->type != Token::Type::lbracket) {
        this->diagnostic(token->begin, "expect '['");
        this->skipTo({Token::Type::lbracket});
        this->nextToken();
    }
    const auto graph_offset = token->begin;
    if ((token = this->nextToken())->type == Token::Type::rbracket) {
        return {};
    }
    this->putBack(std::move(token));
    std::vector<std::vector<uint64_t>> adjacent_list_graph;
    while (true) {
        token = this->nextToken();
        if (token->type == Token::Type::section_name || token->type == Token::Type::end) {
            this->diagnostic(token->begin, "unclosed bracket");
            this->putBack(std::move(token));
            break;
        }
        if (token->type == Token::Type::rbracket) {
            break;
        }
        this->putBack(std::move(token));
        adjacent_list_graph.push_back(this->parseIntegerList(trace_event_cnt - 1));
    }
    lib::Array<uint8_t> bitmap_graph(lib::roundUpDiv(trace_event_cnt * trace_event_cnt, 8));
    std::memset(bitmap_graph.data(), 0, bitmap_graph.length());
    if (adjacent_list_graph.size() != trace_event_cnt) {
        this->diagnostic(graph_offset, "number of member of sequence after graph don't match number of trace event");
        return bitmap_graph;
    }
    for (size_t i = 0; i < adjacent_list_graph.size(); ++i) {
        for (const auto item: adjacent_list_graph[i]) {
            ASSERT(item < trace_event_cnt, "post-condition of parseIntegerList violation");
            auto idx = i * trace_event_cnt + item;
            bitmap_graph[idx / 8] |= 1 << (idx % 8);
        }
    }
    return bitmap_graph;
}

std::vector<uint64_t> Assembler::parseIntegerList(uint64_t max_value)
{
    std::vector<uint64_t> result;
    auto token = this->nextToken();
    if (token->type != Token::Type::lbracket) {
        this->diagnostic(token->begin, "expect '['");
        this->skipTo({Token::Type::lbracket});
        this->nextToken();
    }
    token = this->nextToken();
    if (token->type == Token::Type::rbracket) {
        return result;
    }
    this->putBack(std::move(token));
    while (true) {
        token = this->nextToken();
        if (token->type == Token::Type::section_name || token->type == Token::Type::end) {
            this->diagnostic(token->begin, "unclosed bracket");
            this->putBack(std::move(token));
            break;
        }
        if (token->type != Token::Type::integer) {
            this->diagnostic(token->begin, "expect integer");
            this->skipTo({Token::Type::integer});
            continue;
        }
        auto val = down_cast<IntegerToken&>(token).value;
        if (val > max_value) {
            this->diagnostic(token->begin,
                             lib::format("too big integer '${}'(cannot be great than ${})", val, max_value));
            val = 0;
        }
        result.push_back(val);
        if ((token = this->nextToken())->type != Token::Type::comma) {
            if (token->type == Token::Type::rbracket) {
                break;
            }
            this->diagnostic(token->begin, "expect ','");
        }
    }
    return result;
}

SourceCodeLocator Assembler::parseDebugLocInfos()
{
    if (!this->parseKeyValueCommonCheck("debug")) {
        return {};
    }
    return SourceCodeLocator{this->parseLists<SourceCodeLocator::Item>()};
}

SourceCodeLocator::Item Assembler::parseDebugLocInfo()
{
    auto token = this->nextToken();
    if (token->type != Token::Type::lparen) {
        this->diagnostic(token->begin, "expect '('");
        this->skipTo({Token::Type::lparen});
        this->nextToken();
    }
    uint64_t nums[3]{};
    for (size_t i = 0; i < 3; ++i) {
        if ((token = this->nextToken())->type != Token::Type::integer) {
            this->diagnostic(token->begin, "expect integer");
        } else {
            nums[i] = down_cast<IntegerToken&>(token).value;
        }
        if (i < 2 && (token = this->nextToken())->type != Token::Type::comma) {
            this->diagnostic(token->begin, "expect ','");
        }
    }
    if ((token = this->nextToken())->type != Token::Type::rparen) {
        this->diagnostic(token->begin, "expect ')'");
        this->skipTo({Token::Type::rparen});
        this->nextToken();
    }
    return SourceCodeLocator::Item{nums[0], nums[1], nums[2]};
}

void Assembler::parseKeyValuePair(std::string_view key, std::string& value)
{
    if (!this->parseKeyValueCommonCheck(key)) {
        return;
    }
    auto token = this->nextToken();
    if (!Token::isString(token->type)) {
        this->diagnostic(token->begin, "expect string");
        return;
    }
    value = std::move(down_cast<StringToken&>(token).value);
}

void Assembler::parseKeyValuePair(std::string_view key, uint64_t& value)
{
    if (!this->parseKeyValueCommonCheck(key)) {
        return;
    }
    auto token = this->nextToken();
    if (token->type != Token::Type::integer) {
        this->diagnostic(token->begin, "expect integer");
        return;
    }
    value = down_cast<IntegerToken&>(token).value;
}

void Assembler::parseKeyValuePair(std::string_view key, const ts::Type*& value)
{
    if (!this->parseKeyValueCommonCheck(key)) {
        return;
    }
    if (auto type = this->parseTypeSpecifier();type) {
        value = *type;
    }
}

bool Assembler::parseKeyValueCommonCheck(std::string_view key)
{
    auto token = this->nextToken();
    if (token->type != Token::Type::unquote_string) {
        this->diagnostic(token->begin, "expect unquoted string");
        this->skipTo({Token::Type::unquote_string});
        token = this->nextToken();
        if (token->type == Token::Type::section_name || token->type == Token::Type::end) {
            return false;
        }
    }
    bool pass = true;
    if (key != down_cast<StringToken&>(token).value) {
        this->diagnostic(token->begin, lib::format("expect '${}'", key));
        pass = false;
    }
    if ((token = this->nextToken())->type != Token::Type::colon) {
        this->diagnostic(token->begin, "expect ':'");
        pass = false;
    }
    return pass;
}
