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

#ifndef CAMI_TRANSLATE_BYTECODE_H
#define CAMI_TRANSLATE_BYTECODE_H

#include <cstdint>
#include <lib/array.h>
#include <am/spd.h>
#include <foundation/value.h>
#include <string>
#include <string_view>
#include <utility>
#include <memory>

namespace cami::tr {
using std::operator ""sv;

/**
 * @syntax tbc: Text form cami ByteCode
 *
 * @code
 * begin ::= {section}
 * section ::= attributes | comment | objects | functions |
 *             types | data | code | bss
 * attributes ::= '.attribute' {attribute}
 * attribute ::= version | type | [entry] | [mod_name] |
 *               [static_link] | [dynamic_link]
 * version ::= 'VERSION' string
 * type ::= 'OBJECT' | 'SHARED_OBJECT' | 'EXECUTABLE' |
 *          'FIX_ADDRESS_EXECUTABLE'
 * entry ::= 'ENTRY' string
 * mod_name ::= 'MODULE_NAME' string
 * static_link ::= 'STATIC_LINK' files
 * dynamic_link ::= 'DYNAMIC_LINK' files
 * files ::= '[' string {',' string} [','] ']'
 * comment ::= '.comment' {string}
 * objects ::= ('.object' | '.object.string_literal' |
 *             '.object.bss' | '.object.thread_local')
 *             '[' {object} ']'
 * object ::= '{'
 *                  'name' ':' string
 *                  'address' ':' integer
 *                  'type' ':' type_specifier
 *            '}'
 * functions ::= '.function' '['{ function }']'
 * function ::= '{'
 *                  'name' ':' string
 *                  'address' ':' integer
 *                  'type' ':' type_specifier
 *                  'frame_size' ':' integer
 *                  'code_size' ':' integer
 *                  'max_object_num' ':' integer
 *                  'file_name' ':' string
 *                  'blocks' ':' '[' {block} ']'
 *                  'full_expressions' ':' '['{full_expr}']'
 *                  'debug' ':' '[' {debug_loc_info} ']'
 *              '}'
 * block ::= '[' {stack_object} ']'
 * stack_object ::= '{'
 *                      'name' ':' string
 *                      'dsg_id' ':' integer
 *                      'type' ':' type_specifier
 *                      'offset' ':' integer
 *                      'init_data_offset' ':' integer
 *                  '}'
 * full_expr ::= '{'
 *                  'trace_event_cnt' ':' integer
 *                  'source_location' ':' '[' {location} ']'
 *                  'sequence_after' ':' seq_matrix
 *               '}'
 * location ::= '(' integer ',' integer ')'
 * seq_matrix ::= '[' {'[' [integer {',' integer}] ']'} ']'
 * debug_loc_info ::= '(' integer ',' integer ','integer')'
 * types ::= '.type' type_declaration {type_declaration}
 * type_declaration ::= ('struct' | 'union') string
 *                      type_define
 * type_define ::= '{' type_specifier ';'
 *                    {type_specifier ';'} '}'
 * type_specifier ::= basic_type | 'null' |
 *                    '(' type_specifier ')' |
 *                    type_specifier '*' |
 *                    type_specifier '[' integer ']' |
 *                    '(' ')' '->' type_specifier |
 *                    type_specifier '->' type_specifier |
 *                    '(' type_specifier ',' type_specifier
 *                        {',' type_specifier}
 *                    ')' '->' type_specifier |
 *                    type_specifier qualifier
 * basic_type ::= 'i8' | 'u8' | 'i16' | 'u16' | 'i32' |
 *                'u32' | 'i64' | 'u64' |
 *                'char' | 'bool' | 'f32' | 'f64' | 'void' |
 *                ('struct' | 'union') string
 * qualifier ::= 'const' | 'volatile' |
 *               'restrict' | 'atomic'
 * data ::= ('.data' | '.data.thread_local' | '.stack_init'|
 *          '.string_literal') 'ALIGN' integer bin
 * bin ::= {hex_sequence | string}
 * code ::= ('.code' | '.code.init' |
 *           '.code.init_thread_local')
 *          {code_line}
 * code_line ::= [label] [instr [info]]
 * label ::= unquote_string ':'
 * instr ::= unquote_string
 * info ::= type_specifier | constant | integer |
 *          unquote_string
 * constant ::= '&lt;' type_specifier [';' number] '>'
 * bss ::= '.bss' 'ALIGN' integer integer
 * @endcode
 *
 * @lexical
 *
 * @code
 * boolean ::= 'true' | 'false'
 * hex_sequence ::= '0xs' hex_pair {hex_pair}
 * string ::= unquote_string | quote_string
 * integer ::= dec_integer | dec_float
 * floating ::= dec_integer '.' dec_digits
 *               ['e' ['-'] dec_integer]
 * section_name ::= '.' ('.' | '_' | alpha)
 *                  {'.' | '_' | alpha | dec_digit}
 * alpha ::= 'a' | 'b' | 'c' | 'd' | 'e' | 'f' |
 *           'g' | 'h' | 'i' | 'j' | 'k' | 'l' |
 *           'm' | 'n' | 'o' | 'p' | 'q' | 'r' |
 *           's' | 't' | 'u' | 'v' | 'w' | 'x' |
 *           'y' | 'z' | 'A' | 'B' | 'C' | 'D' |
 *           'E' | 'F' | 'G' | 'H' | 'I' | 'J' |
 *           'K' | 'L' | 'M' | 'N' | 'O' | 'P' |
 *           'Q' | 'R' | 'S' | 'T' | 'U' | 'V' |
 *           'W' | 'X' | 'Y' | 'Z'
 * dec_integer ::= ['-'] dec_digits
 * hex_integer ::= '0x' hex_digits
 * hex_pair ::= hex_digit hex_digit
 * dec_digits ::= dec_digit {dec_digit}
 * hex_digits ::= hex_digit {hex_digit}
 * dec_digit ::= '0' | '1' | '2' | '3' | '4' | '5' |
 *               '6' | '7' | '8' | '9' |
 * hex_digit ::= dec_digit | 'a' | 'b' | 'c' | 'd' | 'e' |
 *               'f' | 'A' | 'B' | 'C' | 'D' | 'E' | 'F'
 * unquote_string ::= normal_char_begin {normal_char}
 *      # unquote_char cannot be 'true' or 'false'
 * quote_string ::= '"' full_char {full_char} '"'
 * normal_char_begin ::= '`' | '~' | '!' | '@' | '$' | '%' |
 *                       '^' | '&' | '_' | '/' | '+' | '=' |
 *                       '|' | ''' | '?' | alpha
 * normal_char ::= normal_char_begin | dec_digit | '-'
 * full_char ::= normal_char | escape_char | SPACE_CHARS |
 *               ',' | ':' | '.' | '[' | '{' | ']' | '}' |
 *               ';' | '*' | '&lt;' | '>' | '#' | '(' | ')'
 * escape_char ::= '\a' | '\b' | '\f' | '\n' | '\r' |
 *                 '\t' | '\v' | '\\' | '\"' | '\'' |
 *                 '\0' | '\x' hex_digit hex_digit
 * @endcode
 */

// memory form bytecode
struct MBC
{
    enum class Type
    {
        object_file, executable, fix_address_executable, shared_object,
    };
    struct Attribute
    {
        std::string version;
        Type type;
        std::string module_or_entry_name;
        const am::spd::Function* entry;
        lib::Array<std::string> static_links;
        lib::Array<std::string> dynamic_links;
    };

    struct BSS
    {
        uint64_t align = 1;
        uint64_t len = 0;
        lib::Array<am::spd::StaticObjectDescription> objects;
        BSS() = default;

        BSS(uint64_t align, uint64_t len, lib::Array<am::spd::StaticObjectDescription> objects)
                : align(align), len(len), objects(std::move(objects)) {}
    };

    struct Data
    {
        uint64_t align = 1;
        lib::Array<uint8_t> bin;
        lib::Array<am::spd::StaticObjectDescription> objects;
        Data() = default;

        Data(uint64_t align, lib::Array<uint8_t> bin, lib::Array<am::spd::StaticObjectDescription> objects)
                : align(align), bin(std::move(bin)), objects(std::move(objects)) {}
    };

    struct Code
    {
        lib::Array<uint8_t> bin;
        lib::Array<am::spd::Function> functions;
        lib::Array<std::pair<uint64_t, std::string>> relocate;
        Code() = default;

        Code(lib::Array<uint8_t> bin, lib::Array<am::spd::Function> functions,
             lib::Array<std::pair<uint64_t, std::string>> relocate)
                : bin(std::move(bin)), functions(std::move(functions)), relocate(std::move(relocate)) {}

        void clear()
        {
            this->bin.clear();
            this->functions.clear();
            this->relocate.clear();
        }
    };

    std::string source_name;
    Attribute attribute;
    std::string comment;
    BSS bss;
    Data data;
    Data string_literal;
    Data stack_init;
    Data thread_local_;
    Code code;
    Code init_code;
    Code thread_local_init_code;
    lib::Array<const ts::Type*> types;
    // only contains integer/floating/nullptr_t
    lib::Array<std::pair<const ts::Type*, uint64_t>> constants;
    MBC() = default;

    MBC(std::string source_name, Attribute attribute, std::string comment, BSS bss,
        Data data, Data string_literal, Data stack_init, Data thread_local_, Code code,
        Code init_code, Code thread_local_init_code, lib::Array<const ts::Type*> types,
        lib::Array<std::pair<const ts::Type*, uint64_t>> constants)
            : source_name(std::move(source_name)), attribute(std::move(attribute)), comment(std::move(comment)),
              bss(std::move(bss)), data(std::move(data)), string_literal(std::move(string_literal)),
              stack_init(std::move(stack_init)), thread_local_(std::move(thread_local_)),
              code(std::move(code)), init_code(std::move(init_code)),
              thread_local_init_code(std::move(thread_local_init_code)), types(std::move(types)),
              constants(std::move(constants)) {}
};
/// binary form bytecode
/// @note out of date
//struct BBC
//{
//    // xxx_end_idx field of struct S means that:
//    //   S point to an array of xxx, more precisely, a range of xxx section
//    //   and the range is [i == 0 ? 0 : S_section[i-1].xxx_end_idx, xxx_end_idx),
//    //   where i is the idx of current instance of S(S_section is an array)
//    template<typename T>
//    using array_t = std::unique_ptr<T[]>;
//    static constexpr auto MAGIC_NUM = "CAMI BYTECODE \xf0\xa5"sv;
//    enum class SectionType
//    {
//        attribute, comment,
//        bss, data, string_literal_data, stack_init_data, thread_local_data,
//        code, init_code, thread_local_init_code,
//        object, bss_object, string_literal_object, thread_local_object,
//        type, constant, function, string_pool,
//        block, stack_object, evaluation_graph, graph_data, id_source_locator, file_source_locator
//    };
//    struct IndexEntry
//    {
//        SectionType section_type;
//        union
//        {
//            struct
//            {
//                uint64_t file_offset;
//                uint64_t length;
//            };
//            struct
//            {
//                uint64_t value1;
//                uint64_t value2;
//            };
//        };
//    };
//    struct Attribute
//    {
//        uint8_t version[3];
//        bool fix_address;
//        bool linked;
//        uint64_t entry;
//        lib::Array<uint64_t> static_links;
//        lib::Array<uint64_t> dynamic_links;
//    };
//    struct Object
//    {
//        uint64_t name;
//        uint64_t type;
//        uint64_t address;
//    };
//    struct AutomaticObjectDescription
//    {
//        uint64_t name;
//        uint64_t id;
//        uint64_t type;
//        uint64_t offset;
//        uint64_t init_offset;
//    };
//    struct FileSourceLocator
//    {
//        uint64_t addr;
//        uint64_t len;
//        uint64_t line;
//    };
//    struct IdentifierSourceLocator
//    {
//        uint64_t len;
//        uint64_t colum;
//    };
//    struct EvaluationSequenceGraph
//    {
//        uint64_t vertex_size;
//        uint64_t graph_data_end_idx;
//        uint64_t identifier_source_locator_idx;
//    };
//    struct Function
//    {
//        uint64_t name;
//        uint64_t type;
//        uint64_t address;
//        uint64_t file_name;
//        uint64_t frame_size;
//        uint64_t code_size;
//        uint64_t max_object_num;
//        uint64_t block_end_idx;
//        uint64_t evaluation_sequence_graph_end_idx;
//        uint64_t file_source_locator_end_idx;
//    };
//    std::string source_name;
//    char magic_number[16];
//    uint64_t idx_cnt;
//    array_t<IndexEntry> indexes;
//    Attribute attribute;
//    array_t<uint8_t> string_pool;
//    array_t<uint8_t> comment;
//    // first 8 byte of data/stack_init_data/thread_local_data is alignment
//    array_t<uint8_t> data;
//    array_t<uint8_t> stack_init_data;
//    array_t<uint8_t> thread_local_data;
//    array_t<uint8_t> code;
//    array_t<uint8_t> init_code;
//    array_t<uint8_t> thread_local_init_code;
//    array_t<uint64_t> types;
//    array_t<std::pair<uint64_t, uint64_t>> constants;
//    array_t<Object> objects;
//    array_t<Object> bss_objects;
//    array_t<Object> string_literal_objects;
//    array_t<Object> thread_local_objects;
//    array_t<Function> functions;
//    array_t<uint64_t> blocks;
//    array_t<AutomaticObjectDescription> stack_objects;
//    array_t<EvaluationSequenceGraph> evaluation_sequence_graph;
//    array_t<uint64_t> evaluation_sequence_graph_data;
//    array_t<IdentifierSourceLocator> identifier_source_locator;
//    array_t<FileSourceLocator> file_source_locator;
//};
}

#endif //CAMI_TRANSLATE_BYTECODE_H
