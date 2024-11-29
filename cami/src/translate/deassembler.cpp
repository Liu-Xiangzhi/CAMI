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

#include <deassembler.h>
#include <lib/utils.h>
#include <lib/format.h>
#include <am/fetch_decode.h>
#include <am/formatter.h>
#include <foundation/type/helper.h>
#include <foundation/logger.h>

using namespace cami;
using namespace tr;
using am::Opcode;
using am::spd::Block;
using am::FullExprInfo;
using am::spd::SourceCodeLocator;
using am::spd::Function;
using am::spd::StaticObjectDescription;

namespace {
// 'Q' means quoted 'E' means escaped
struct QEStringView
{
    const char* ptr;
    size_t len;

    QEStringView(const char* ptr, size_t len) : ptr(ptr), len(len) {}

    explicit QEStringView(std::string_view sv) : ptr(sv.data()), len(sv.length()) {}

    explicit QEStringView(const std::string& str) : ptr(str.data()), len(str.length()) {}

    explicit QEStringView(const lib::Array<uint8_t>& arr)
            : ptr(reinterpret_cast<const char*>(arr.data())), len(arr.length()) {}
};

struct Bin
{
    const uint8_t* ptr;
    size_t len;
    bool to_string;

    Bin(const uint8_t* ptr, size_t len, bool to_string) : ptr(ptr), len(len), to_string(to_string) {}

    Bin(const std::vector<uint8_t>& vec, bool to_string) : ptr(vec.data()), len(vec.size()), to_string(to_string) {}
};

std::string toString(UnlinkedMBC::StaticObject::Segment segment)
{
    switch (segment) {
    case UnlinkedMBC::StaticObject::data:
        return "data";
    case UnlinkedMBC::StaticObject::string_literal:
        return "string_literal";
    case UnlinkedMBC::StaticObject::bss:
        return "bss";
    case UnlinkedMBC::StaticObject::thread_local_:
        return "thread_local";
    default:
        return "???";
    }
}

std::string toString(UnlinkedMBC::Function::Segment segment)
{
    switch (segment) {
    case UnlinkedMBC::Function::execute:
        return "execute";
    case UnlinkedMBC::Function::init:
        return "init";
    case UnlinkedMBC::Function::thread_local_init:
        return "thread_local_init";
    default:
        return "???";
    }
}

char hexToChar(uint8_t hex)
{
    return static_cast<char>(hex < 10 ? hex + '0' : hex - 10 + 'A');
}

std::string escapeString(std::string_view sv)
{
    std::string result;
    for (auto ch: sv) {
        if (std::isgraph(ch)) {
            if (ch == '\"') {
                result.append("\\\"");
            } else if (ch == '\\') {
                result.append("\\\\");
            } else {
                result.append(1, ch);
            }
        } else {
            switch (ch) {
            case ' ':
                result.append(1, ch);
                break;
            case '\a':
                result.append("\\a");
                break;
            case '\b':
                result.append("\\b");
                break;
            case '\f':
                result.append("\\f");
                break;
            case '\n':
                result.append("\\n");
                break;
            case '\r':
                result.append("\\r");
                break;
            case '\t':
                result.append("\\t");
                break;
            case '\v':
                result.append("\\v");
                break;
            case '\0':
                result.append("\\0");
                break;
            default:
                result.append(lib::format("\\x${x}${x}", ch >> 4, ch & 0xf));
            }
        }
    }
    return result;
}

std::set<uint64_t> getLabelOffsets(lib::Slice<const uint8_t> code)
{
    using am::FetchDecode;
    std::set<uint64_t> label_offsets{};
    for (size_t i = 0; i < code.length();) {
        auto op = static_cast<Opcode>(code[i]);
        if (!FetchDecode::hasExtraInfo(op)) {
            i++;
            continue;
        }
        if (op == Opcode::j || op == Opcode::jst || op == Opcode::jnt) {
            label_offsets.insert(i + 4 + lib::readI<3>(&code[i + 1]));
        }
        i += 4;
    }
    return label_offsets;
}
} // anonymous namespace

template<>
struct lib::ToString<QEStringView>
{
    static std::string invoke(const QEStringView& arg, [[maybe_unused]] std::string_view specifier)
    {
        return std::string(1, '\"').append(escapeString({arg.ptr, arg.len})).append(1, '\"');
    }
};

template<>
struct lib::ToString<Bin>
{
    static std::string invoke(const Bin& bin, [[maybe_unused]] std::string_view specifier)
    {
        if (bin.to_string) {
            return lib::ToString<QEStringView>::invoke({reinterpret_cast<const char*>(bin.ptr), bin.len}, specifier);
        }
        std::string result{"0x"};
        for (size_t i = 0; i < bin.len; ++i) {
            result.push_back(hexToChar(bin.ptr[i] >> 4));
            result.push_back(hexToChar(bin.ptr[i] & 0xf));
        }
        result.push_back('.');
        return result;
    }
};

void DeAssembler::deassemble(const MBC& mbc, std::ostream& output)
{
    DeAssembler::attribute(mbc.attribute, output);
    DeAssembler::comment(mbc.comment, output);
    if (mbc.attribute.type == MBC::Type::object_file) {
        auto& unlinked_mbc = down_cast<const UnlinkedMBC&>(mbc);
        DeAssembler::types(unlinked_mbc.types, output);
        DeAssembler::Unlinked::object(unlinked_mbc.objects, output);
        DeAssembler::Unlinked::function(unlinked_mbc.functions, output);
    } else {
        auto& linked_mbc = down_cast<const LinkedMBC&>(mbc);
        DeAssembler::types(linked_mbc.types, output);
        DeAssembler::Linked::object(linked_mbc, output);
        DeAssembler::Linked::function(linked_mbc, output);
    }
}

void DeAssembler::attribute(const MBC::Attribute& attribute, std::ostream& output)
{
    output << lib::format(".attribute\n\tVERSION ${}\n", QEStringView{attribute.version});
    switch (attribute.type) {
    case MBC::Type::object_file:
        output << "\tOBJECT\n";
        break;
    case MBC::Type::executable:
        output << "\tEXECUTABLE\n";
        break;
    case MBC::Type::shared_object:
        output << "\tSHARED_OBJECT\n";
        break;
    }
    if (!attribute.module_or_entry_name.empty()) {
        auto attribute_name = attribute.type == MBC::Type::executable ? "ENTRY" : "MODULE";
        output << lib::format("\t${} ${}\n", attribute_name, QEStringView{attribute.module_or_entry_name});
    }
    if (!attribute.static_links.empty()) {
        output << "\tSTATIC_LINK [";
        for (size_t i = 0; i < attribute.static_links.size() - 1; ++i) {
            output << lib::format("${} ,", QEStringView{attribute.static_links[i]});
        }
        output << lib::format("${}]\n", QEStringView{attribute.static_links.back()});
    }
    if (!attribute.dynamic_links.empty()) {
        output << "\tDYNAMIC_LINK [";
        for (size_t i = 0; i < attribute.dynamic_links.size() - 1; ++i) {
            output << lib::format("${} ,", QEStringView{attribute.dynamic_links[i]});
        }
        output << lib::format("${}]\n", QEStringView{attribute.dynamic_links.back()});
    }
}

void DeAssembler::comment(std::string_view comment, std::ostream& output)
{
    if (comment.empty()) {
        return;
    }
    output << lib::format(".comment\n\t${}\n", QEStringView{comment});
}

void DeAssembler::types(lib::Slice<const ts::Type* const> types, std::ostream& output)
{
    using ts::Kind;
    auto cnt = std::count_if(types.begin(), types.end(), [&](const ts::Type* item) {
        return item->kind() == Kind::struct_ || item->kind() == Kind::union_;
    });
    if (cnt == 0) {
        return;
    }
    output << ".type\n";
    for (auto type: types) {
        if (type->kind() == Kind::struct_) {
            auto& struct_ = down_cast<const ts::Struct&>(*type);
            output << "\tstruct " << struct_.name << "{\n";
            for (auto item: struct_.members) {
                output << lib::format("\t\t${};\n", *item);
            }
            output << "\t}\n";
        } else if (type->kind() == Kind::union_) {
            auto& union_ = down_cast<const ts::Union&>(*type);
            output << "\tunion " << union_.name << "{\n";
            for (auto item: union_.members) {
                output << lib::format("\t\t${};\n", *item);
            }
            output << "\t}\n";
        }
    }
}

void DeAssembler::sourceCoderLocator(lib::Slice<const am::spd::SourceCodeLocator::Item> locator, std::ostream& output)
{
    for (const auto& item: locator) {
        output << lib::format("\t\t\t\t(${}, ${}, ${})\n", item.addr, item.len, item.line);
    }
}

void DeAssembler::accessSourceLocation(lib::Slice<const std::pair<uint64_t, uint64_t>> location, std::ostream& output)
{
    for (const auto& item: location) {
        output << lib::format("\t\t\t\t\t\t(${}, ${})\n", item.first, item.second);
    }
}

void DeAssembler::sequenceAfterGraph(uint64_t trace_event_cnt, lib::Slice<const uint8_t> sequence_after_graph, std::ostream& output)
{
    if (sequence_after_graph.size() < lib::roundUpDiv(trace_event_cnt * trace_event_cnt, 8)) {
        log::unbuffered.eprintln("sequence_after_graph size(${}) do not match trace_event_cnt(${})."
                                 "The former must be great equal than the square of the latter div 8",
                                 sequence_after_graph.size(), trace_event_cnt);
    } else {
        for (size_t i = 0; i < trace_event_cnt; ++i) {
            output << "\t\t\t\t\t\t[";
            std::vector<uint64_t> numbers;
            for (int j = 0; j < trace_event_cnt; ++j) {
                auto idx = i * trace_event_cnt + j;
                if (sequence_after_graph[idx / 8] & (1 << (idx % 8))) {
                    numbers.push_back(j);
                }
            }
            if (!numbers.empty()) {
                output << numbers[0];
                for (int j = 1; j < numbers.size(); ++j) {
                    output << ", " << numbers[j];
                }
            }
            output << "]\n";
        }
    }
}
//
//void DeAssembler::Unlinked::constant(std::pair<const ts::Type*, uint64_t> constant, std::ostream& output)
//{
//    using namespace ts;
//    auto type = constant.first;
//    output << lib::format(" <${}; ", *type);
//    switch (type->kind()) {
//    case Kind::f32: {
//        float value = *reinterpret_cast<const float*>(constant.second);
//        output << value;
//        break;
//    }
//    case Kind::f64: {
//        double value = *reinterpret_cast<const double*>(constant.second);
//        output << value;
//        break;
//    }
//    case Kind::null:
//        output << "null";
//        break;
//    default:
//        if (!isInteger(type->kind())) {
//            output << "???";
//        } else {
//            if (isUnsigned(type->kind())) {
//                output << constant.second;
//            } else {
//                output << *reinterpret_cast<int64_t*>(&constant.second);
//            }
//        }
//    }
//    output << ">\n";
//}

void DeAssembler::Unlinked::object(lib::Slice<const std::unique_ptr<UnlinkedMBC::StaticObject>> objects, std::ostream& output)
{
    if (objects.empty()) {
        return;
    }
    output << ".object\n\t[\n";
    for (const auto& item: objects) {
        output << "\t\t{\n"
               << lib::format("\t\t\tsegment: ${}\n", toString(item->segment))
               << lib::format("\t\t\tname: ${}\n", QEStringView{item->name})
               << lib::format("\t\t\ttype: ${}\n", *item->type)
               << lib::format("\t\t\tvalue: ${}\n", Bin{item->value, item->segment == UnlinkedMBC::StaticObject::string_literal})
               << lib::format("\t\t\trelocate: ${}\n", QEStringView{item->relocate_symbol})
               << "\t\t}\n";
    }
    output << "\t]\n";
}

void DeAssembler::Unlinked::function(lib::Slice<const std::unique_ptr<UnlinkedMBC::Function>> functions, std::ostream& output)
{
    output << ".function\n";
    output << "\t[\n";
    for (const auto& item: functions) {
        output << "\t\t{\n"
               << lib::format("\t\t\tsegment: ${}\n", toString(item->segment))
               << lib::format("\t\t\tname: ${}\n", QEStringView{item->name})
               << lib::format("\t\t\ttype: ${}\n", *item->effective_type)
               << lib::format("\t\t\tfile_name: ${}\n", QEStringView{item->file_name})
               << lib::format("\t\t\tframe_size: ${}\n", item->frame_size)
               << lib::format("\t\t\tmax_object_num: ${}\n", item->max_object_num)
               << "\t\t\tblocks: [\n";
        for (const auto& block: item->blocks) {
            DeAssembler::Unlinked::block(block, output);
        }
        output << "\t\t\t]\n"
               << "\t\t\tfull_expressions: [\n";
        for (const auto& full_expr: item->full_expr_infos) {
            DeAssembler::Unlinked::fullExpression(full_expr, output);
        }
        output << "\t\t\t]\n"
               << "\t\t\tdebug: [\n";
        DeAssembler::sourceCoderLocator(item->func_locator.data, output);
        output << "\t\t\t]\n"
               << "\t\t\tcode:\n";
        DeAssembler::Unlinked::code(item->code, item->relocate, output);
        output << "\t\t}\n";
    }
    output << "\t]\n";
}

void DeAssembler::Unlinked::block(const UnlinkedMBC::Block& block, std::ostream& output)
{
    output << "\t\t\t\t[\n";
    for (const auto& item: block.automatic_objects) {
        output << "\t\t\t\t\t{\n"
               << lib::format("\t\t\t\t\t\tname: ${}\n", QEStringView{item.name})
               << lib::format("\t\t\t\t\t\tdsg_id: ${}\n", item.id)
               << lib::format("\t\t\t\t\t\ttype: ${}\n", *item.type)
               << lib::format("\t\t\t\t\t\toffset: ${} # ${x}\n", item.offset, item.offset)
               << "";
        if (!item.init_data.empty()) {
            output << lib::format("\t\t\t\t\t\tinit_data: ${}\n", Bin{item.init_data, false});
        }
        output << "\t\t\t\t\t}\n";
    }
    output << "\t\t\t\t]\n";
}

void DeAssembler::Unlinked::fullExpression(const UnlinkedMBC::FullExprInfo& full_expr, std::ostream& output)
{
    output << "\t\t\t\t{\n"
           << lib::format("\t\t\t\t\ttrace_event_cnt: ${}\n", full_expr.trace_event_cnt)
           << "\t\t\t\t\tsource_location: [\n";
    DeAssembler::accessSourceLocation(full_expr.source_location, output);
    output << "\t\t\t\t\t]\n"
           << "\t\t\t\t\tsequence_after: [\n";
    DeAssembler::sequenceAfterGraph(full_expr.trace_event_cnt, full_expr.sequence_after_graph, output);
    output << "\t\t\t\t\t]\n"
           << "\t\t\t\t}\n";
}

void DeAssembler::Unlinked::code(lib::Slice<const uint8_t> code, lib::Slice<const UnlinkedMBC::RelocateEntry> relocate,
                                 std::ostream& output)
{
    using am::FetchDecode;
    using am::Formatter;
    auto label_offsets = getLabelOffsets(code);
    std::map<uint64_t, std::string_view> relocate_map;
    for (const auto& item: relocate) {
        relocate_map.emplace(item.instr_offset, item.symbol);
    }
    auto cur_label = label_offsets.begin();
    for (size_t i = 0; i < code.length();) {
        if (cur_label != label_offsets.end() && i == *cur_label) {
            output << "\t\t\t\tL" << *cur_label << ":\n";
            cur_label++;
        }
        auto op = static_cast<Opcode>(code[i]);
        output << "\t\t\t\t\t" << Formatter::opcode(op);
        if (!FetchDecode::hasExtraInfo(op)) {
            output << '\n';
            i++;
            continue;
        }
        i += 4;
        auto val = lib::readI<3>(&code[i - 3]);
        if (op == Opcode::j || op == Opcode::jst || op == Opcode::jnt) {
            auto itr = label_offsets.find(i + val);
            ASSERT(itr != label_offsets.end(), "you cannot possibly not find the element that is inserted before");
            output << " L" << *itr << '\n';
            continue;
        }
        if (op == Opcode::new_ || op == Opcode::cast || op == Opcode::push || op == Opcode::dsg) {
            auto itr = relocate_map.find(i - 4);
            if (itr == relocate_map.end()) {
                if (op == Opcode::dsg) {
                    goto output_number;
                }
                output << " ???\n";
                continue;
            }
            auto info = op == Opcode::cast || op == Opcode::new_ ? itr->second.substr(1) : itr->second;
            output << ' ' << info << '\n';
            continue;
        }
output_number:
        output << ' ' << val << '\n';
    }
    output << "\t\t\t\t.\n";
}

void DeAssembler::Linked::object(const LinkedMBC& linked_mbc, std::ostream& output)
{
    auto& objects = linked_mbc.static_objects;
    auto& data = linked_mbc.data;
    if (objects.empty()) {
        return;
    }
    output << ".object\n\t[\n";
    for (const auto& item: objects) {
        output << "\t\t{\n"
               << lib::format("\t\t\tname: ${}\n", QEStringView{item.name})
               << lib::format("\t\t\ttype: ${}\n", *item.type)
               << lib::format("\t\t\taddress: ${}\n", item.address);
        if (item.address < data.size()) {
            output << lib::format("\t\t\tvalue: ${}\n",
                                  Bin{&data[item.address], item.type->size(), item.address < linked_mbc.string_literal_len});
        } else {
            output << "\t\t\tvalue: <bss>\n";
        }
        output << "\t\t}\n";
    }
    output << "\t]\n";
}

void DeAssembler::Linked::function(const LinkedMBC& linked_mbc, std::ostream& output)
{
    auto& functions = linked_mbc.functions;
    output << ".function\n";
    output << "\t[\n";
    for (const auto& item: functions) {
        output << "\t\t{\n"
               << lib::format("\t\t\tname: ${}\n", QEStringView{item.name})
               << lib::format("\t\t\ttype: ${}\n", item.effective_type)
               << lib::format("\t\t\taddress: ${}\n", item.address)
               << lib::format("\t\t\tfile_name: ${}\n", QEStringView{item.file_name})
               << lib::format("\t\t\tframe_size: ${}\n", item.frame_size)
               << lib::format("\t\t\tcode_size: ${}\n", item.code_size)
               << lib::format("\t\t\tmax_object_num: ${}\n", item.max_object_num)
               << "\t\t\tblocks: [\n";
        for (const auto& block: item.blocks) {
            DeAssembler::Linked::block(block, output);
        }
        output << "\t\t\t]\n"
               << "\t\t\tfull_expressions: [\n";
        for (const auto& full_expr: item.full_expr_infos) {
            DeAssembler::Linked::fullExpression(full_expr, output);
        }
        output << "\t\t\t]\n"
               << "\t\t\tdebug: [\n";
        DeAssembler::sourceCoderLocator(item.func_locator.data, output);
        output << "\t\t\t]\n"
               << "\t\t\tcode:\n";
        DeAssembler::Linked::code(linked_mbc, item.address, item.code_size, output);
        output << "\t\t}\n";
    }
    output << "\t]\n";
}

void DeAssembler::Linked::code(const LinkedMBC& linked_mbc, uint64_t addr, uint64_t len, std::ostream& output)
{
    using am::FetchDecode;
    using am::Formatter;
    auto& code = linked_mbc.code;
    auto label_offsets = getLabelOffsets(code);
    auto cur_label = label_offsets.begin();
    for (size_t i = addr; i < addr + len;) {
        if (cur_label != label_offsets.end() && i == *cur_label) {
            output << "\t\t\t\tL" << *cur_label << ":\n";
            cur_label++;
        }
        auto op = static_cast<Opcode>(code[i]);
        output << "\t\t\t\t\t" << Formatter::opcode(op);
        if (!FetchDecode::hasExtraInfo(op)) {
            output << '\n';
            i++;
            continue;
        }
        i += 4;
        auto val = lib::readI<3>(&code[i - 3]);
        if (op == Opcode::j || op == Opcode::jst || op == Opcode::jnt) {
            auto itr = label_offsets.find(i + val);
            ASSERT(itr != label_offsets.end(), "you cannot possibly not find the element that is inserted before");
            output << " L" << *itr << '\n';
            continue;
        }
        DeAssembler::Linked::instruction(op, val, linked_mbc, output);
    }
    output << "\t\t\t\t.\n";
}

void DeAssembler::Linked::instruction(am::Opcode op, uint64_t info, const LinkedMBC& linked_mbc, std::ostream& output)
{
    using am::InstrInfo;
    switch (op) {
    case Opcode::new_:
    case Opcode::cast:
        if (info >= linked_mbc.types.length()) {
            goto show_unknown;
        }
        output << lib::format(" ${}\n", *linked_mbc.types[info]);
        return;
    case Opcode::push:
        if (info >= linked_mbc.constants.length()) {
            goto show_unknown;
        }
        DeAssembler::Linked::constant(linked_mbc.constants[info], output);
        return;
    case Opcode::dsg: {
        auto identifierId = InstrInfo::IdentifierId{static_cast<uint32_t>(info)};
        if (!identifierId.isGlobal()) {
            output << ' ' << identifierId.value() << '\n';
            return;
        }
        if (identifierId.isFuntion()) {
            if (identifierId.value() >= linked_mbc.functions.length()) {
                goto show_unknown;
            }
            output << ' ' << linked_mbc.functions[identifierId.value()].name << '\n';
        } else {
            if (identifierId.value() >= linked_mbc.static_objects.length()) {
                goto show_unknown;
            }
            output << ' ' << linked_mbc.static_objects[identifierId.value()].name << '\n';
        }
        return;
    }
    default:
        output << ' ' << info << '\n';
        return;
    }
show_unknown:
    output << " ???\n";
}

void DeAssembler::Linked::block(const Block& block, std::ostream& output)
{
    output << "\t\t\t\t[\n";
    for (const auto& item: block.obj_desc) {
        output << "\t\t\t\t\t{\n"
               << lib::format("\t\t\t\t\t\tname: ${}\n", QEStringView{item.name})
               << lib::format("\t\t\t\t\t\tdsg_id: ${}\n", item.id)
               << lib::format("\t\t\t\t\t\ttype: ${}\n", item.type)
               << lib::format("\t\t\t\t\t\toffset: ${} # ${x}\n", item.offset, item.offset)
               << "\t\t\t\t\t\tinit_data: ";
        if (item.init_data) {
            output << lib::format("\t\t\t\t\t\tinit_data: ${}\n", Bin{item.init_data.get(), item.type.size(), false});
        }
        output << "\t\t\t\t\t}\n";
    }
    output << "\t\t\t\t]\n";
}

void DeAssembler::Linked::constant(const ValueBox& constant, std::ostream& output)
{
    using namespace ts;
    auto& type = constant->getType();
    output << lib::format(" <${}; ", type);
    switch (type.kind()) {
    case Kind::f32: {
        output << constant.get<F32Value>().f32();
        break;
    }
    case Kind::f64: {
        output << constant.get<F64Value>().f64();
        break;
    }
    case Kind::null:
        output << "null";
        break;
    default:
        if (!isInteger(type.kind())) {
            output << "???";
        } else {
            auto val = constant.get<IntegerValue>().uint64();
            if (isUnsigned(type.kind())) {
                output << val;
            } else {
                output << *reinterpret_cast<int64_t*>(&val);
            }
        }
    }
    output << ">\n";
}

void DeAssembler::Linked::fullExpression(const FullExprInfo& full_expr, std::ostream& output)
{
    output << "\t\t\t\t{\n"
           << lib::format("\t\t\t\t\ttrace_event_cnt: ${}\n", full_expr.trace_event_cnt)
           << "\t\t\t\t\tsource_location: [\n";
    DeAssembler::accessSourceLocation(full_expr.source_location, output);
    output << "\t\t\t\t\t]\n"
           << "\t\t\t\t\tsequence_after: [\n";
    DeAssembler::sequenceAfterGraph(full_expr.trace_event_cnt, full_expr.sequence_after_graph, output);
    output << "\t\t\t\t\t]\n"
           << "\t\t\t\t}\n";
}
