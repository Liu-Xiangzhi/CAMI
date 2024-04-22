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

#ifndef CAMI_TRANSLATE_ASSEMBLER_H
#define CAMI_TRANSLATE_ASSEMBLER_H

#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <set>
#include <deque>
#include <memory>
#include <foundation/type/def.h>
#include <foundation/logger.h>
#include <lib/optional.h>
#include <lib/slice.h>
#include "bytecode.h"

// TBC ==> MBC
namespace cami::tr {
namespace detail {
template<typename T>
struct ParseFunctionTrait;

struct ConstantHelper
{
    static std::pair<const ts::Type*, uint64_t> makeNull() noexcept
    {
        return {nullptr, 0};
    }

    static bool isNull(const std::pair<const ts::Type*, uint64_t>& val) noexcept
    {
        return val.first == nullptr;
    }
};

} // namespace detail

using constant_opt_t = lib::Optional<std::pair<const ts::Type*, uint64_t>, detail::ConstantHelper>;

struct Token
{
    enum class Type
    {
        unquote_string, quote_string, section_name,
        lparen, rparen, lbracket, rbracket, lbrace, rbrace, colon, semicolon,
        dot, comma, star, arrow, less, great,
        boolean, integer, floating, hex_sequence, end
    };
    Type type;
    uint64_t begin;

    Token(Type type, uint64_t begin) : type(type), begin(begin) {}

    virtual ~Token() = default;

    static bool isString(Type type) noexcept
    {
        return type <= Type::quote_string;
    }

    static bool isString(const std::unique_ptr<Token>& token) noexcept
    {
        return Token::isString(token->type);
    }
};

struct BoolToken : public Token
{
    bool value;

    BoolToken(uint64_t begin, bool value)
            : Token(Type::boolean, begin), value(value) {}
};

struct IntegerToken : public Token
{
    uint64_t value;

    IntegerToken(uint64_t begin, uint64_t value)
            : Token(Type::integer, begin), value(value) {}
};

struct FloatingToken : public Token
{
    double value;

    FloatingToken(uint64_t begin, double value)
            : Token(Type::floating, begin), value(value) {}
};

struct StringToken : public Token
{
    std::string value;

    StringToken(Type type, uint64_t begin, std::string value)
            : Token(type, begin), value(std::move(value)) {}
};

struct HexSequenceToken : public Token
{
    std::vector<uint8_t> value;

    explicit HexSequenceToken(uint64_t begin) : Token(Type::hex_sequence, begin) {}

    HexSequenceToken(uint64_t begin, std::vector<uint8_t> value)
            : Token(Type::hex_sequence, begin), value(std::move(value)) {}
};

class Lexer
{
    std::string_view input;
    std::string_view file_name;
    uint64_t cur = 0;
    std::vector<uint64_t> newlines; // offset of newline
    uint64_t starting_line;
    uint64_t starting_colum;
    bool has_error = false;
public:
    Lexer(const std::string_view& input, std::string_view file_name,
          uint64_t starting_line = 1, uint64_t starting_colum = 1)
            : input(input), file_name(file_name), starting_line(starting_line), starting_colum(starting_colum) {}

public:
    std::unique_ptr<Token> nextToken();

    void reset(std::string_view new_input, std::string_view new_file_name) noexcept
    {
        this->input = new_input;
        this->file_name = new_file_name;
        this->cur = 0;
    }

    std::pair<uint64_t, uint64_t> getLineColum(std::unique_ptr<Token>& token)
    {
        return getLineColum(token->begin);
    }

    std::pair<uint64_t, uint64_t> getLineColum(uint64_t offset)
    {
        auto itr = std::lower_bound(this->newlines.begin(), this->newlines.end(), offset);
        if (itr == this->newlines.begin()) {
            return {this->starting_line, starting_colum + offset};
        }
        itr--;
        auto offset_of_last_line = *itr;
        auto line_cnt = itr - this->newlines.begin();
        return {this->starting_line + line_cnt + 1, offset - offset_of_last_line};
    }

    [[nodiscard]] std::string_view getFileName() const noexcept
    {
        return this->file_name;
    }

    [[nodiscard]] bool hasError() const noexcept
    {
        return this->has_error;
    }

private:
    void diagnostic(uint64_t offset, std::string_view what)
    {
        this->has_error = true;
        auto [line, colum] = this->getLineColum(offset);
        log::unbuffered.eprintln("Lexical error: at file '${}', ${}:${} : ${}", this->file_name, line, colum, what);
    }

    std::unique_ptr<Token> extractSectionName();
    std::unique_ptr<Token> extractQuotedString();
    std::optional<char> extractEscapeCharacter();
    std::string extractUnquotedString();
    std::unique_ptr<Token> extractHexSequence();
    std::unique_ptr<Token> extractCharacter();
    std::unique_ptr<Token> extractNumber();
};

class Assembler
{
    Lexer lexer{"", ""};
    UnlinkedMBC* mbc = nullptr;
    std::deque<std::unique_ptr<Token>> token_buffer;
    bool has_error = false;
    bool has_attribute = false;
    std::set<const ts::Type*> parsed_types;
    std::set<std::pair<const ts::Type*, uint64_t>> parsed_constants;
public:
    // entry
    std::unique_ptr<UnlinkedMBC> assemble(std::string_view tbc, std::string_view name);
private:
    // entry
    void parseSection();
    // attribute
    void parseAttribute();
    void parseComment();
    void parseLinks(std::vector<std::string>& result);
    // code
    void parseTypes();
    void parseTypeDefine(bool is_struct, std::string struct_or_union_name);
    auto parseTypeSpecifier() -> lib::Optional<const ts::Type*>;
    auto do_parseTypeSpecifier() -> lib::Optional<const ts::Type*>;
    auto parseTypeArray(const ts::Type* base_type) -> lib::Optional<const ts::Type*>;
    auto parseTypeFunction() -> lib::Optional<const ts::Type*>;
    auto parseTypeFunction(const std::vector<const ts::Type*>& params) -> lib::Optional<const ts::Type*>;
    auto parseTypeBaseOrQualify(const ts::Type* base_type, std::unique_ptr<Token> token) -> std::pair<lib::Optional<const ts::Type*>, bool>;
    auto parseCode() -> std::pair<std::vector<uint8_t>, std::vector<UnlinkedMBC::RelocateEntry>>;
    auto parseConstant() -> constant_opt_t;
    void parseInstr(std::vector<uint8_t>& code_bin,
                    std::vector<UnlinkedMBC::RelocateEntry>& relocate,
                    std::vector<std::tuple<uint64_t, std::string, uint64_t>>& relocate_label);
    // entity(object & function)
    void parseObjects();
    void parseFunctions();
    auto parseStaticObject() -> std::unique_ptr<UnlinkedMBC::StaticObject>;
    auto parseBin() -> std::vector<uint8_t>;
    auto parseFunction() -> std::unique_ptr<UnlinkedMBC::Function>;
    auto parseBlocks() -> std::vector<UnlinkedMBC::Block>;
    auto parseFullExprInfos() -> std::vector<UnlinkedMBC::FullExprInfo>;
    auto parseFullExprInfo() -> UnlinkedMBC::FullExprInfo;
    auto parseBlock() -> UnlinkedMBC::Block;
    auto parseAutoObject() -> UnlinkedMBC::AutomaticObject;
    auto parseFullExprSourceLocation(uint64_t trace_event_cnt) -> std::vector<std::pair<uint64_t, uint64_t>>;
    auto parseFullExprSourceLocationPair() -> std::pair<uint64_t, uint64_t>;
    auto parseFullExprSequenceAfterGraph(uint64_t trace_event_cnt) -> std::vector<uint8_t>;
    auto parseIntegerList(uint64_t max_value) -> std::vector<uint64_t>;
    auto parseDebugLocInfos() -> std::vector<am::spd::SourceCodeLocator::Item>;
    auto parseDebugLocInfo() -> am::spd::SourceCodeLocator::Item;
    template<typename T>
    std::vector<T> parseLists();
    template<typename T, typename P = T>
    P parseKV(std::string_view key);
    bool parseKeyValueCommonCheck(std::string_view key);
private:
    // helper functions
    void reset(std::string_view tbc, std::string_view name)
    {
        this->lexer.reset(tbc, name);
        this->token_buffer.clear();
        this->has_error = false;
        this->has_attribute = false;
        this->parsed_types.clear();
        this->parsed_constants.clear();
    }

    std::unique_ptr<Token> nextToken()
    {
        if (!this->token_buffer.empty()) {
            auto token = std::move(this->token_buffer.back());
            this->token_buffer.pop_back();
            return token;
        }
        return this->lexer.nextToken();
    }

    Token& peek()
    {
        if (this->token_buffer.empty()) {
            auto token = this->lexer.nextToken();
            this->putBack(std::move(token));
        }
        return *this->token_buffer.back();
    }

    void putBack(std::unique_ptr<Token> token)
    {
        ASSERT(token.get() != nullptr, "should not put a null token");
        this->token_buffer.push_back(std::move(token));
    }

    void diagnostic(uint64_t offset, std::string_view what)
    {
        this->has_error = true;
        auto [line, colum] = this->lexer.getLineColum(offset);
        log::unbuffered.eprintln("Syntax error: at file '${}', ${}:${} : ${}", this->lexer.getFileName(), line, colum,
                                 what);
    }

    void skipTo(const std::set<Token::Type>& types)
    {
        auto token = this->nextToken();
        while (types.find(token->type) == types.end() && token->type != Token::Type::section_name &&
               token->type != Token::Type::end) {
            token = this->nextToken();
        }
        this->putBack(std::move(token));
    }

    // helper struct for `parseLists`
    template<typename T>
    friend struct detail::ParseFunctionTrait;
};

} // namespace cami::tr

#endif //CAMI_TRANSLATE_ASSEMBLER_H
