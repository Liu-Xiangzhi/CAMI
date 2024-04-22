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
#include <lib/bitmap.h>
#include <lib/assert.h>

using namespace cami;
using namespace tr;

namespace {
int parseHex(char ch)
{
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    } else if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    } else if (ch >= 'A' && ch <= 'F') {
        return ch - 'A' + 10;
    } else {
        return -1;
    }
}
}

std::unique_ptr<Token> Lexer::nextToken()
{
    for (; this->cur < this->input.size(); ++this->cur) {
        char ch = this->input[this->cur];
        switch (ch) {
        case '(':
            ++this->cur;
            return std::make_unique<Token>(Token::Type::lparen, this->cur);
        case ')':
            ++this->cur;
            return std::make_unique<Token>(Token::Type::rparen, this->cur);
        case '[':
            ++this->cur;
            return std::make_unique<Token>(Token::Type::lbracket, this->cur);
        case ']':
            ++this->cur;
            return std::make_unique<Token>(Token::Type::rbracket, this->cur);
        case '{':
            ++this->cur;
            return std::make_unique<Token>(Token::Type::lbrace, this->cur);
        case '}':
            ++this->cur;
            return std::make_unique<Token>(Token::Type::rbrace, this->cur);
        case '<':
            ++this->cur;
            return std::make_unique<Token>(Token::Type::less, this->cur);
        case '>':
            ++this->cur;
            return std::make_unique<Token>(Token::Type::great, this->cur);
        case ',':
            ++this->cur;
            return std::make_unique<Token>(Token::Type::comma, this->cur);
        case ':':
            ++this->cur;
            return std::make_unique<Token>(Token::Type::colon, this->cur);
        case ';':
            ++this->cur;
            return std::make_unique<Token>(Token::Type::semicolon, this->cur);
        case '*':
            ++this->cur;
            return std::make_unique<Token>(Token::Type::star, this->cur);
        case '.':
            if (this->cur + 1 < this->input.length()) {
                char nch = this->input[this->cur + 1];
                if (nch == '.' || nch == '_' || std::isalpha(nch)) {
                    return this->extractSectionName();
                }
            }
            ++this->cur;
            return std::make_unique<Token>(Token::Type::dot, this->cur);
        case '"':
            return this->extractQuotedString();
        case '-':
            if (this->cur + 1 < this->input.length() && this->input[this->cur + 1] == '>') {
                this->cur += 2;
                return std::make_unique<Token>(Token::Type::arrow, this->cur);
            }
            [[fallthrough]];
        case '\'':
            return this->extractNumber();
        case '#':
            for (; this->cur < this->input.length(); this->cur++) {
                if (this->input[this->cur] == '\n') {
                    this->newlines.push_back(this->cur);
                    break;
                }
            }
            continue;
        default:
            break;
        }
        if (std::isspace(ch)) {
            if (ch == '\n') {
                this->newlines.push_back(this->cur);
            }
            continue;
        }
        if (std::isdigit(ch)) {
            if (this->cur + 1 < this->input.size() && this->input[this->cur + 1] == 'x'
                && this->cur + 2 < this->input.size() && this->input[this->cur + 2] == 's') {
                return this->extractHexSequence();
            }
            return this->extractNumber();
        }
        auto begin = this->cur;
        auto str = this->extractUnquotedString();
        if (str == "true" || str == "false") {
            return std::make_unique<BoolToken>(begin, str == "true");
        }
        return std::make_unique<StringToken>(Token::Type::unquote_string, begin, std::move(str));
    }
    return std::make_unique<Token>(Token::Type::end, this->cur);
}

std::unique_ptr<Token> Lexer::extractQuotedString()
{
    const auto begin = this->cur;
    ASSERT(this->input[this->cur] == '"', "precondition violation");
    ++this->cur;
    std::string result;
    for (; this->cur < this->input.length(); ++this->cur) {
        char ch = this->input[this->cur];
        if (ch == '"') {
            break;
        }
        if (ch == '\n') {
            this->newlines.push_back(this->cur);
        }
        if (ch == '\\') {
            auto escape_ch = this->extractEscapeCharacter();
            if (!escape_ch) {
                goto end;
            }
            ch = *escape_ch;
        }
        result.push_back(ch);
    }
end:
    if (this->cur == this->input.length()) {
        this->diagnostic(this->cur, "unterminated quoted string");
    }
    ++this->cur;
    return std::make_unique<StringToken>(Token::Type::quote_string, begin + 1, std::move(result));
}

std::string Lexer::extractUnquotedString()
{
    static const lib::Bitmap<256> bitmap{
            '`', '~', '!', '@', '$', '%', '^', '&', '_', '+', '=', '\'', '/', '?',
            '-', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
            'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u',
            'v', 'w', 'x', 'y', 'z',
            'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U',
            'V', 'W', 'X', 'Y', 'Z'
    };
    const auto begin = this->cur;
    for (; this->cur < this->input.length(); ++this->cur) {
        if (!bitmap.test(this->input[this->cur])) {
            break;
        }
    }
    if (this->cur == begin) {
        this->diagnostic(this->cur, lib::format("Unknown character '${}'", static_cast<int>(this->input[this->cur])));
        ++this->cur;
    }
    return {&this->input[begin], this->cur - begin};
}

std::unique_ptr<Token> Lexer::extractHexSequence()
{
    ASSERT(std::string_view(&this->input[this->cur], 3) == "0xs", "precondition violation");
    const auto begin = this->cur;
    this->cur += 3;
    std::vector<uint8_t> seq;
    uint8_t byte;
    enum class State
    {
        start, half
    } state = State::start;
    for (; this->cur < this->input.length(); ++this->cur) {
        int half_byte = parseHex(this->input[this->cur]);
        if (half_byte == -1) {
            break;
        }
        if (state == State::start) {
            byte = half_byte << 4;
            state = State::half;
        } else {
            byte |= half_byte;
            seq.push_back(byte);
            state = State::start;
        }
    }
    if (state != State::start || seq.empty()) {
        this->diagnostic(this->cur, "expect hex digit");
    }
    return std::make_unique<HexSequenceToken>(begin, std::move(seq));
}

std::unique_ptr<Token> Lexer::extractNumber()
{
    if (this->input[this->cur] == '\'') {
        return this->extractCharacter();
    }
    bool hex = false;
    bool floating = false;
    const auto begin = this->cur;
    if (this->input[this->cur] == '-') {
        ++this->cur;
    } else if (this->input[this->cur] == '0') {
        if (this->cur + 1 < this->input.length() && this->input[this->cur + 1] == 'x') {
            hex = true;
            this->cur += 2;
        }
    }
    for (; this->cur < this->input.length(); ++this->cur) {
        char ch = this->input[this->cur];
        if (hex ? !std::isxdigit(ch) : !std::isdigit(ch)) {
            break;
        }
    }
    if (this->cur < this->input.length() && this->input[this->cur] == '.') {
        floating = true;
        ++this->cur;
        for (; this->cur < this->input.length(); ++this->cur) {
            if (!std::isdigit(this->input[this->cur])) {
                break;
            }
        }
        if (this->cur < this->input.length() && this->input[this->cur] == 'e') {
            ++this->cur;
            if (this->cur < this->input.length() && this->input[this->cur] == '-') {
                ++this->cur;
            }
            for (; this->cur < this->input.length(); ++this->cur) {
                if (!std::isdigit(this->input[this->cur])) {
                    break;
                }
            }
        }
    }
    std::string number_str{&this->input[begin], this->cur - begin};
    if (floating) {
        double value;
        try {
            value = std::stod(number_str);
        } catch (const std::out_of_range&) {
            this->diagnostic(begin, "floating value out of range");
            value = 0;
        } catch (const std::exception&) {
            this->diagnostic(begin, lib::format("cannot parse floating value: `${}`", number_str));
            value = 0;
        }
        return std::make_unique<FloatingToken>(begin, value);
    } else {
        uint64_t value;
        try {
            value = std::stoull(number_str, nullptr, hex ? 16 : 10);
        } catch (const std::out_of_range&) {
            this->diagnostic(begin, "integer value out of range");
            value = 0;
        } catch (const std::exception&) {
            this->diagnostic(begin, lib::format("cannot parse integer value: `${}`", number_str));
            value = 0;
        }
        return std::make_unique<IntegerToken>(begin, value);
    }
}

std::unique_ptr<Token> Lexer::extractSectionName()
{
    ASSERT(this->input[this->cur] == '.', "precondition violation");
    const auto begin = this->cur;
    ++this->cur;
    for (; this->cur < this->input.length(); ++this->cur) {
        char ch = this->input[this->cur];
        if (ch != '_' && ch != '.' && !std::isalnum(ch)) {
            break;
        }
    }
    return std::make_unique<StringToken>(Token::Type::section_name, begin,
                                         std::string{&this->input[begin], this->cur - begin});
}

std::optional<char> Lexer::extractEscapeCharacter()
{
    if (++this->cur >= this->input.length()) {
        this->diagnostic(this->cur, "missing escaped character");
        return {};
    }
    auto ch = this->input[this->cur];
    switch (ch) {
    case 'a':
        return '\a';
    case 'b':
        return '\b';
    case 'f':
        return '\f';
    case 'n':
        return '\n';
    case 'r':
        return '\r';
    case 't':
        return '\t';
    case 'v':
        return '\v';
    case '\\':
        return '\\';
    case '\"':
        return '\"';
    case '\'':
        return '\'';
    case '0':
        return '\0';
    case 'x': {
        if (this->cur + 2 >= this->input.length()) {
            this->diagnostic(this->input.length(), "missing hex digit");
            return {};
        }
        auto high_4 = parseHex(this->input[++this->cur]);
        if (high_4 == -1) {
            this->diagnostic(this->cur, "expect hex digit");
        }
        auto low_4 = parseHex(this->input[++this->cur]);
        if (low_4 == -1) {
            this->diagnostic(this->cur, "expect hex digit");
        }
        return static_cast<char>((high_4 << 4) | low_4);
    }
    default:
        this->diagnostic(this->cur, "unknown escape character");
        return {};
    }
}

std::unique_ptr<Token> Lexer::extractCharacter()
{
    const auto begin = this->cur;
    ASSERT(this->input[this->cur] == '\'', "precondition violation");
    char ch = this->input[++this->cur];
    if (ch == '\'') {
        this->diagnostic(this->cur, "empty character");
        goto error;
    }
    if (ch == '\n') {
        this->diagnostic(this->cur, "newline character is not allow. try '\\n'?");
        goto error;
    }
    char result;
    if (ch == '\\') {
        auto escaped_ch = this->extractEscapeCharacter();
        if (!escaped_ch) {
            goto error;
        }
        result = *escaped_ch;
    } else {
        result = ch;
    }
    if (++this->cur >= this->input.length() || this->input[this->cur] != '\'') {
        this->diagnostic(this->cur, "expect '\\''");
        goto error;
    }
    ++this->cur;
    return std::make_unique<IntegerToken>(begin + 1, result);
error:
    for (; this->cur < this->input.length(); ++this->cur) {
        if (this->input[this->cur] == '\'') {
            ++this->cur;
            break;
        }
    }
    return std::make_unique<IntegerToken>(begin + 1, -1);
}
