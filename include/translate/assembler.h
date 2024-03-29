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
#include "bytecode.h"

// TBC ==> MBC
namespace cami::tr {
template<typename T>
struct ParseFunctionTrait;
namespace detail {
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
    std::string extractUnquotedString();
    std::unique_ptr<Token> extractHexSequence();
    std::unique_ptr<Token> extractNumber();
};

class Assembler
{
    Lexer lexer{"", ""};
    MBC* mbc = nullptr;
    std::deque<std::unique_ptr<Token>> token_buffer;
    bool has_error = false;
    std::set<const ts::Type*> parsed_types;
    std::set<std::pair<const ts::Type*,uint64_t>> parsed_constants;
public:
    // entry
    std::unique_ptr<MBC> assemble(std::string_view tbc, std::string_view name);
private:
    // entry
    void parseSection();
    // attribute
    void parseAttribute();
    void parseComment();
    void parseLinks(lib::Array<std::string>& result);
    // code
    void parseTypes();
    void parseTypeDefine(bool is_struct, std::string struct_or_union_name);
    lib::Optional<const ts::Type*> parseTypeSpecifier();
    lib::Optional<const ts::Type*> do_parseTypeSpecifier();
    lib::Optional<const ts::Type*> parseTypeArray(const ts::Type* base_type);
    lib::Optional<const ts::Type*> parseTypeFunction();
    lib::Optional<const ts::Type*> parseTypeFunction(const std::vector<const ts::Type*>& params);
    std::pair<lib::Optional<const ts::Type*>, bool> parseTypeBasicOrQualifier(
            const ts::Type* base_type, std::unique_ptr<Token> token);
    void parseCodeCommon(MBC::Code& code);
    constant_opt_t parseConstant();
    void parseInstr(std::vector<uint8_t>& code_bin,
                    std::vector<std::pair<uint64_t, std::string>>& relocate,
                    std::vector<std::tuple<uint64_t, std::string, uint64_t>>& relocate_label);
    // data
    void parseBSS();
    void parseDataCommon(MBC::Data& data);
    void parseBin(lib::Array<uint8_t>& result);
    // entity
    template<typename T>
    lib::Array<T> parseLists();
    void parseObjectsCommon(lib::Array<am::spd::StaticObjectDescription>& objects);
    am::spd::StaticObjectDescription parseStaticObject();
    void parseFunctionsCommon(lib::Array<am::spd::Function>& functions);
    am::spd::Function parseFunction();
    lib::Array<am::spd::Block> parseBlocks();
    lib::Array<am::FullExprInfo> parseFullExprInfos();
    am::FullExprInfo parseFullExprInfo();
    am::spd::Block parseBlock();
    am::spd::AutomaticObjectDescription parseAutoObject();
    lib::Array<std::pair<uint64_t, uint64_t>> parseFullExprSourceLocation();
    std::pair<uint64_t, uint64_t> parseFullExprSourceLocationPair();
    lib::Array<uint8_t> parseFullExprSequenceAfterGraph(uint64_t trace_event_cnt);
    std::vector<uint64_t> parseIntegerList(uint64_t max_value);
    am::spd::SourceCodeLocator parseDebugLocInfos();
    am::spd::SourceCodeLocator::Item parseDebugLocInfo();
    void parseKeyValuePair(std::string_view key, std::string& value);
    void parseKeyValuePair(std::string_view key, uint64_t& value);
    void parseKeyValuePair(std::string_view key, const ts::Type*& value);
    bool parseKeyValueCommonCheck(std::string_view key);

    void parseData()
    {
        this->parseDataCommon(this->mbc->data);
    }

    void parseStringLiteralData()
    {
        this->parseDataCommon(this->mbc->string_literal);
    }

    void parseThreadLocalData()
    {
        this->parseDataCommon(this->mbc->thread_local_);
    }

    void parseStackInitData()
    {
        this->parseDataCommon(this->mbc->stack_init);
    }

    void parseCode()
    {
        this->parseCodeCommon(this->mbc->code);
    };

    void parseInitCode()
    {
        this->parseCodeCommon(this->mbc->init_code);
    };

    void parseThreadLocalInitCode()
    {
        this->parseCodeCommon(this->mbc->thread_local_init_code);
    };

    void parseObjects()
    {
        this->parseObjectsCommon(this->mbc->data.objects);
    }

    void parseBSSObjects()
    {
        this->parseObjectsCommon(this->mbc->bss.objects);
    }

    void parseStringLiteralObjects()
    {
        this->parseObjectsCommon(this->mbc->string_literal.objects);
    }

    void parseThreadLocalObjects()
    {
        this->parseObjectsCommon(this->mbc->thread_local_.objects);
    }

    void parseFunctions()
    {
        this->parseFunctionsCommon(this->mbc->code.functions);
    }

private:
    // helper functions
    void reset(std::string_view tbc, std::string_view name)
    {
        this->lexer.reset(tbc, name);
        this->token_buffer.clear();
        this->has_error = false;
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
    friend
    struct ParseFunctionTrait;

};

} // namespace cami::tr

#endif //CAMI_TRANSLATE_ASSEMBLER_H
