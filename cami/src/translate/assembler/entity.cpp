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
using am::spd::SourceCodeLocator;
using StaticObject = UnlinkedMBC::StaticObject;
using Function = UnlinkedMBC::Function;
using Block = UnlinkedMBC::Block;
using AutomaticObject = UnlinkedMBC::AutomaticObject;
using FullExprInfo = UnlinkedMBC::FullExprInfo;

namespace {
// just a tag
struct Bin
{
};
}
template<typename T>
struct tr::detail::ParseFunctionTrait
{
};
template<>
struct tr::detail::ParseFunctionTrait<std::unique_ptr<Function>>
{
    static constexpr auto value = &Assembler::parseFunction;
};
template<>
struct tr::detail::ParseFunctionTrait<std::unique_ptr<StaticObject>>
{
    static constexpr auto value = &Assembler::parseStaticObject;
};
template<>
struct tr::detail::ParseFunctionTrait<AutomaticObject>
{
    static constexpr auto value = &Assembler::parseAutoObject;
};
template<>
struct tr::detail::ParseFunctionTrait<Block>
{
    static constexpr auto value = &Assembler::parseBlock;
};
template<>
struct tr::detail::ParseFunctionTrait<FullExprInfo>
{
    static constexpr auto value = &Assembler::parseFullExprInfo;
};
template<>
struct tr::detail::ParseFunctionTrait<std::pair<uint64_t, uint64_t>>
{
    static constexpr auto value = &Assembler::parseFullExprSourceLocationPair;
};
template<>
struct tr::detail::ParseFunctionTrait<am::spd::SourceCodeLocator::Item>
{
    static constexpr auto value = &Assembler::parseDebugLocInfo;
};

template<>
std::string Assembler::parseKV<std::string>(std::string_view key)
{
    if (!this->parseKeyValueCommonCheck(key)) {
        return {};
    }
    auto token = this->nextToken();
    if (!Token::isString(token->type)) {
        this->diagnostic(token->begin, "expect string");
        return {};
    }
    return std::move(down_cast<StringToken&>(token).value);
}

template<>
uint64_t Assembler::parseKV<uint64_t>(std::string_view key)
{
    if (!this->parseKeyValueCommonCheck(key)) {
        return 0;
    }
    auto token = this->nextToken();
    if (token->type != Token::Type::integer) {
        this->diagnostic(token->begin, "expect integer");
        return 0;
    }
    return down_cast<IntegerToken&>(token).value;
}

template<>
const ts::Type* Assembler::parseKV<const ts::Type*>(std::string_view key)
{
    if (!this->parseKeyValueCommonCheck(key)) {
        return nullptr;
    }
    if (auto type = this->parseTypeSpecifier();type) {
        return *type;
    }
    return {};
}

template<>
StaticObject::Segment Assembler::parseKV<StaticObject::Segment>(std::string_view key)
{
    if (!this->parseKeyValueCommonCheck(key)) {
        return StaticObject::data;
    }
    auto token = this->nextToken();
    if (!Token::isString(token->type)) {
        this->diagnostic(token->begin, "expect string");
        return {};
    }
    auto& value = down_cast<StringToken&>(token).value;
    if (value == "string_literal"sv) {
        return StaticObject::string_literal;
    }
    if (value == "data"sv) {
        return StaticObject::data;
    }
    if (value == "bss"sv) {
        return StaticObject::bss;
    }
    if (value == "thread_local"sv) {
        return StaticObject::thread_local_;
    }
    this->diagnostic(token->begin, "invalid object segment");
    return {};
}

template<>
Function::Segment Assembler::parseKV<Function::Segment>(std::string_view key)
{
    if (!this->parseKeyValueCommonCheck(key)) {
        return Function::execute;
    }
    auto token = this->nextToken();
    if (!Token::isString(token->type)) {
        this->diagnostic(token->begin, "expect string");
        return {};
    }
    auto& value = down_cast<StringToken&>(token).value;
    if (value == "execute"sv) {
        return Function::execute;
    }
    if (value == "init"sv) {
        return Function::init;
    }
    if (value == "thread_local_init"sv) {
        return Function::thread_local_init;
    }
    this->diagnostic(token->begin, "invalid function segment");
    return {};
}

template<>
std::vector<uint8_t> Assembler::parseKV<Bin>(std::string_view key)
{
    if (!this->parseKeyValueCommonCheck(key)) {
        return {};
    }
    return this->parseBin();
}

template<typename T, typename P>
P Assembler::parseKV(std::string_view)
{
    static_assert(!std::is_same_v<T, T>, "invalid value type");
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

template<typename T>
std::vector<T> Assembler::parseLists()
{
    auto token = this->nextToken();
    if (token->type != Token::Type::lbracket) {
        this->diagnostic(token->begin, "expect '['");
        this->skipTo({Token::Type::lbracket});
        this->nextToken();
    }
    if (this->peek().type == Token::Type::rbracket) {
        this->nextToken();
        return {};
    }
    std::vector<T> sub_elements;
    while (true) {
        sub_elements.push_back((this->*detail::ParseFunctionTrait<T>::value)());
        if (auto& next = this->peek();next.type == Token::Type::section_name || next.type == Token::Type::end) [[unlikely]] {
            this->diagnostic(next.begin, "unclosed bracket");
            break;
        }
        if (this->peek().type == Token::Type::rbracket) {
            this->nextToken();
            break;
        }
    }
    return sub_elements;
}

void Assembler::parseObjects()
{
    this->mbc->objects = this->parseLists<std::unique_ptr<StaticObject>>();
}

void Assembler::parseFunctions()
{
    this->mbc->functions = this->parseLists<std::unique_ptr<Function>>();
}

std::unique_ptr<StaticObject> Assembler::parseStaticObject()
{
    auto token = this->nextToken();
    if (token->type != Token::Type::lbrace) {
        this->diagnostic(token->begin, "expect '{'");
        this->skipTo({Token::Type::lbrace});
        this->nextToken();
    }
    auto segment = this->parseKV<StaticObject::Segment>("segment"sv);
    auto name = this->parseKV<std::string>("name"sv);
    auto type = this->parseKV<const ts::Type*>("type"sv);
    std::vector<uint8_t> value;
    if (segment != StaticObject::bss) {
        value = this->parseKV<Bin, std::vector<uint8_t>>("value"sv);
    }
    std::string relocate;
    if (this->peek().type != Token::Type::rbrace) {
        relocate = this->parseKV<std::string>("relocate"sv);
    }
    if ((token = this->nextToken())->type != Token::Type::rbrace) {
        this->diagnostic(token->begin, "expect '}'");
        this->skipTo({Token::Type::rbrace});
        this->nextToken();
    }
    return std::make_unique<StaticObject>(segment, std::move(name), type, std::move(value), std::move(relocate));
}

std::unique_ptr<Function> Assembler::parseFunction()
{
    auto token = this->nextToken();
    if (token->type != Token::Type::lbrace) {
        this->diagnostic(token->begin, "expect '{'");
        this->skipTo({Token::Type::lbrace});
        this->nextToken();
    }
    using ts::Type;
    auto segment = this->parseKV<Function::Segment>("segment"sv);
    auto name = this->parseKV<std::string>("name"sv);
    auto type = this->parseKV<const ts::Type*>("type"sv);
    auto func_file_name = this->parseKV<std::string>("file_name"sv);
    auto frame_size = this->parseKV<uint64_t>("frame_size"sv);
    auto max_object_num = this->parseKV<uint64_t>("max_object_num");
    auto blocks = this->parseBlocks();
    auto full_expr_info = this->parseFullExprInfos();
    auto debug_loc_info = this->parseDebugLocInfos();
    auto [code, relocate] = this->parseCode();
    if ((token = this->nextToken())->type != Token::Type::rbrace) {
        this->diagnostic(token->begin, "expect '}'");
        this->skipTo({Token::Type::rbrace});
        this->nextToken();
    }
    return std::make_unique<Function>(segment, std::move(name), type, std::move(func_file_name), frame_size, max_object_num,
                                      std::move(blocks), std::move(full_expr_info), UnlinkedMBC::SourceLocator{std::move(debug_loc_info)},
                                      std::move(code), std::move(relocate));
}

std::vector<Block> Assembler::parseBlocks()
{
    if (!this->parseKeyValueCommonCheck("blocks")) {
        return {};
    }
    return this->parseLists<Block>();
}

Block Assembler::parseBlock()
{
    return Block{this->parseLists<AutomaticObject>()};
}

AutomaticObject Assembler::parseAutoObject()
{
    using ts::Type;
    auto token = this->nextToken();
    if (token->type != Token::Type::lbrace) {
        this->diagnostic(token->begin, "expect '{'");
        this->skipTo({Token::Type::lbrace});
        this->nextToken();
    }
    auto name = this->parseKV<std::string>("name"sv);
    auto dsg_id = this->parseKV<uint64_t>("dsg_id"sv);
    auto type = this->parseKV<const ts::Type*>("type"sv);
    auto offset = this->parseKV<uint64_t>("offset");
    std::vector<uint8_t> init_data;
    if (this->peek().type != Token::Type::rbrace) {
        init_data = this->parseKV<Bin, std::vector<uint8_t>>("init_data"sv);
    }
    if ((token = this->nextToken())->type != Token::Type::rbrace) {
        this->diagnostic(token->begin, "expect '}'");
        this->skipTo({Token::Type::rbrace});
        this->nextToken();
    }
    return {std::move(name), dsg_id, type, offset, std::move(init_data)};
}

std::vector<FullExprInfo> Assembler::parseFullExprInfos()
{
    if (!this->parseKeyValueCommonCheck("full_expressions")) {
        return {};
    }
    return this->parseLists<FullExprInfo>();
}

FullExprInfo Assembler::parseFullExprInfo()
{
    auto token = this->nextToken();
    if (token->type != Token::Type::lbrace) {
        this->diagnostic(token->begin, "expect '{'");
        this->skipTo({Token::Type::lbrace});
        this->nextToken();
    }
    auto trace_event_cnt = this->parseKV<uint64_t>("trace_event_cnt"sv);
    auto locations = this->parseFullExprSourceLocation(trace_event_cnt);
    auto graph = this->parseFullExprSequenceAfterGraph(trace_event_cnt);
    if ((token = this->nextToken())->type != Token::Type::rbrace) {
        this->diagnostic(token->begin, "expect '}'");
        this->skipTo({Token::Type::rbrace});
        this->nextToken();
    }
    return {trace_event_cnt, std::move(graph), std::move(locations)};
}

std::vector<std::pair<uint64_t, uint64_t>> Assembler::parseFullExprSourceLocation(uint64_t trace_event_cnt)
{
    auto source_location_offset = this->peek().begin;
    if (!this->parseKeyValueCommonCheck("source_location")) {
        return {};
    }
    auto result = this->parseLists<std::pair<uint64_t, uint64_t>>();
    if (result.size() != trace_event_cnt) {
        this->diagnostic(source_location_offset, "number of source location don't match number of trace event");
    }
    return result;
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

std::vector<uint8_t> Assembler::parseFullExprSequenceAfterGraph(uint64_t trace_event_cnt)
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
    std::vector<uint8_t> bitmap_graph(lib::roundUpDiv(trace_event_cnt * trace_event_cnt, 8));
    std::memset(bitmap_graph.data(), 0, bitmap_graph.size());
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

std::vector<SourceCodeLocator::Item> Assembler::parseDebugLocInfos()
{
    if (!this->parseKeyValueCommonCheck("debug")) {
        return {};
    }
    return this->parseLists<SourceCodeLocator::Item>();
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

std::vector<uint8_t> Assembler::parseBin()
{
    std::vector<uint8_t> hex_sequence;
    while (true) {
        auto token = this->nextToken();
        if (token->type == Token::Type::section_name || token->type == Token::Type::end) {
            this->putBack(std::move(token));
            break;
        }
        if (token->type == Token::Type::dot) {
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
    return hex_sequence;
}
