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
#include <lib/downcast.h>
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
 * section ::= attributes | comment | types |
 *             objects | functions
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
 * objects ::= '.object' '[' {object} ']'
 * object ::= '{'
 *                  'segment' : object_segment
 *                  'name' ':' string
 *                  'type' ':' type_specifier
 *                  ['value' ':' bin]
 *                  ['relocate' ':' string]
 *            '}'
 * functions ::= '.function' '['{ function }']'
 * function ::= '{'
 *                  'segment' ':' function_segment
 *                  'name' ':' string
 *                  'type' ':' type_specifier
 *                  'file_name' ':' string
 *                  'frame_size' ':' integer
 *                  'max_object_num' ':' integer
 *                  'blocks' ':' '[' {block} ']'
 *                  'full_expressions' ':' '['{full_expr}']'
 *                  'debug' ':' '[' {debug_loc_info} ']'
 *                  'code' ':' {code_line} '.'
 *              '}'
 * data_segment ::= 'string_literal' | 'data' |
 *                  'bss' | 'thread_local'
 * function_segment ::= 'execute' | 'init' |
 *                      'thread_local_init'
 * block ::= '[' {automatic_object} ']'
 * automatic_object ::= '{'
 *                      'name' ':' string
 *                      'dsg_id' ':' integer
 *                      'type' ':' type_specifier
 *                      'offset' ':' integer
 *                      ['init_data' ':' bin]
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
 * bin ::= {hex_sequence | string} '.'
 * code_line ::= [label] [instr [info]]
 * label ::= unquote_string ':'
 * instr ::= unquote_string
 * info ::= type_specifier | constant | integer |
 *          unquote_string
 * constant ::= '&lt;' type_specifier ';' constant_value '>'
 * constant_value ::= number | 'nan' | 'inf' | '-inf' |
 *                    'null'
 * @endcode
 *
 * @lexical
 *
 * @code
 * boolean ::= 'true' | 'false'
 * hex_sequence ::= '0xs' hex_pair {hex_pair}
 * string ::= unquote_string | quote_string
 * integer ::= dec_integer | hex_integer | char
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
 * char ::= '\'' full_char_without_newline '\''
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
 * full_char_without_newline ::=
 *               normal_char | escape_char | SPACE_CHARS |
 *               ',' | ':' | '.' | '[' | '{' | ']' | '}' |
 *               ';' | '*' | '&lt;' | '>' | '#' | '(' | ')'
 * full_char ::= full_char_without_newline | NEWLINE
 * escape_char ::= '\a' | '\b' | '\f' | '\n' | '\r' |
 *                 '\t' | '\v' | '\\' | '\"' | '\'' |
 *                 '\0' | '\x' hex_digit hex_digit
 * @endcode
 */

// text form bytecode
struct TBC
{
    std::string text;
    std::string_view name;
};

// memory form bytecode
struct MBC
{
    enum class Type
    {
        object_file, executable, shared_object,
    };
    struct Attribute
    {
        std::string version;
        Type type;
        std::string module_or_entry_name;
        const am::spd::Function* entry;
        std::vector<std::string> static_links;
        std::vector<std::string> dynamic_links;
    };
    std::string source_name;
    Attribute attribute;
    std::string comment;
    MBC() = default;

    MBC(std::string source_name, Attribute attribute, std::string comment) :
            source_name(std::move(source_name)), attribute(std::move(attribute)), comment(std::move(comment)) {}

    DEBUG_VIRTUAL ~MBC() = default;
};

struct UnlinkedMBC : public MBC
{
    struct StaticObject
    {
        enum Segment
        {
            string_literal, data, bss, thread_local_
        } segment;
        std::string name;
        const ts::Type* type;
        std::vector<uint8_t> value;
        // for address constant only, the address of entity referenced by `relocate_symbol` will be added into `value`
        std::string relocate_symbol;

        StaticObject(Segment segment, std::string name, const ts::Type* type, std::vector<uint8_t> value, std::string relocate_symbol)
                : segment(segment), name(std::move(name)), type(type), value(std::move(value)),
                  relocate_symbol(std::move(relocate_symbol)) {}
    };

    struct AutomaticObject
    {
        std::string name;
        size_t id;
        const ts::Type* type;
        uint64_t offset;
        std::vector<uint8_t> init_data;
    };

    struct RelocateEntry
    {
        uint64_t instr_offset;
        std::string symbol;

        RelocateEntry(uint64_t instr_offset, std::string symbol)
                : instr_offset(instr_offset), symbol(std::move(symbol)) {}
    };

    struct Block
    {
        std::vector<AutomaticObject> automatic_objects;
    };
    struct FullExprInfo
    {
        uint64_t trace_event_cnt;
        std::vector<uint8_t> sequence_after_graph;
        std::vector<std::pair<uint64_t, uint64_t>> source_location;
    };
    struct SourceLocator
    {
        std::vector<am::spd::SourceCodeLocator::Item> data;
    };

    struct Function
    {
        enum Segment
        {
            execute, init, thread_local_init,
        } segment{};
        std::string name;
        const ts::Type* effective_type{};
        std::string file_name;
        size_t frame_size{};
        size_t max_object_num{};
        std::vector<Block> blocks;
        std::vector<FullExprInfo> full_expr_infos;
        SourceLocator func_locator;
        std::vector<uint8_t> code;
        std::vector<RelocateEntry> relocate;
        Function() = default;

        Function(Segment segment, std::string name, const ts::Type* effective_type, std::string file_name, size_t frame_size,
                 size_t max_object_num, std::vector<Block> blocks, std::vector<FullExprInfo> full_expr_infos,
                 SourceLocator func_locator, std::vector<uint8_t> code, std::vector<RelocateEntry> relocate)
                : segment(segment), name(std::move(name)), effective_type(effective_type),
                  file_name(std::move(file_name)), frame_size(frame_size), max_object_num(max_object_num),
                  blocks(std::move(blocks)), full_expr_infos(std::move(full_expr_infos)), func_locator(std::move(func_locator)),
                  code(std::move(code)), relocate(std::move(relocate)) {}
    };

    std::vector<std::unique_ptr<StaticObject>> objects;
    std::vector<std::unique_ptr<Function>> functions;
    std::vector<const ts::Type*> types;
    // only contains integer/floating/nullptr_t
    std::vector<std::pair<const ts::Type*, uint64_t>> constants;
    UnlinkedMBC() = default;

    UnlinkedMBC(std::string source_name, Attribute attribute, std::string comment,
                std::vector<std::unique_ptr<StaticObject>> objects,
                std::vector<std::unique_ptr<Function>> functions, std::vector<const ts::Type*> types,
                std::vector<std::pair<const ts::Type*, uint64_t>> constants)
            : MBC(std::move(source_name), std::move(attribute), std::move(comment)), objects(std::move(objects)),
              functions(std::move(functions)), types(std::move(types)), constants(std::move(constants)) {}
};

struct LinkedMBC : public MBC
{
    lib::Array<uint8_t> code;
    lib::Array<uint8_t> data;
    uint64_t string_literal_len;
    uint64_t bss_size;
    lib::Array<am::spd::StaticObjectDescription> static_objects;
    lib::Array<ValueBox> constants;
    lib::Array<const ts::Type*> types;
    lib::Array<am::spd::Function> functions;

    struct RelocateEntry
    {
        uint64_t offset;
        std::string symbol;

        RelocateEntry(uint64_t offset, std::string symbol) : offset(offset), symbol(std::move(symbol)) {}
    };

    std::vector<RelocateEntry> data_relocate;

    LinkedMBC(std::string source_name, Attribute attribute, std::string comment,
              lib::Array<uint8_t> code, lib::Array<uint8_t> data, uint64_t string_literal_len, uint64_t bss_size,
              lib::Array<am::spd::StaticObjectDescription> static_objects, lib::Array<ValueBox> constants,
              lib::Array<const ts::Type*> types, lib::Array<am::spd::Function> functions,
              std::vector<RelocateEntry> data_relocate) :
            MBC(std::move(source_name), std::move(attribute), std::move(comment)), code(std::move(code)),
            data(std::move(data)), string_literal_len(string_literal_len), bss_size(bss_size),
            static_objects(std::move(static_objects)), constants(std::move(constants)),
            types(std::move(types)), functions(std::move(functions)),
            data_relocate(std::move(data_relocate)) {}
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
