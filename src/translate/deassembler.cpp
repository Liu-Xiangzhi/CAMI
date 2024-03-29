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

// 'Q' means quoted 'E' means escaped
struct QEStringView
{
    const char* ptr;
    size_t len;

    explicit QEStringView(std::string_view sv) : ptr(sv.data()), len(sv.length()) {}

    explicit QEStringView(const std::string& str) : ptr(str.data()), len(str.length()) {}

    explicit QEStringView(const lib::Array<uint8_t>& arr)
            : ptr(reinterpret_cast<const char*>(arr.data())), len(arr.length()) {}
};
namespace {
char hexToChar(uint8_t hex)
{
    return static_cast<char>(hex < 10 ? hex + '0' : hex - 10 + 'A');
}

uint64_t readI24(const uint8_t* addr)
{
#ifdef CAMI_TARGET_INFO_LITTLE_ENDIAN
    int64_t value = 0;
    std::memcpy(&value, addr, 3);
#else
    value = *addr++;
    value |= *addr++ << 8;
    value |= *addr << 16;
#endif
    return (value << 40) >> 40;
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
} // anonymous namespace

template<>
struct lib::ToString<QEStringView>
{
    static std::string invoke(const QEStringView& arg, [[maybe_unused]] std::string_view specifier)
    {
        return std::string(1, '\"').append(escapeString({arg.ptr, arg.len})).append(1, '\"');
    }
};

void DeAssembler::deassemble(const MBC& mbc, std::ostream& output)
{
    DeAssembler::dasAttribute(mbc.attribute, output);
    DeAssembler::dasComment(mbc.comment, output);
    DeAssembler::dasTypes(mbc.types, output);
    DeAssembler::dasCode(".code", mbc, mbc.code, output);
    DeAssembler::dasCode(".code.init", mbc, mbc.init_code, output);
    DeAssembler::dasCode(".code.init_thread_local", mbc, mbc.thread_local_init_code, output);
    DeAssembler::dasObject(".object", mbc.data.objects, output);
    DeAssembler::dasObject(".object.string_literal", mbc.string_literal.objects, output);
    DeAssembler::dasObject(".object.bss", mbc.bss.objects, output);
    DeAssembler::dasObject(".object.thread_local", mbc.thread_local_.objects, output);
    DeAssembler::dasFunction(mbc.code.functions, output);
    DeAssembler::dasBSS(mbc.bss, output);
    DeAssembler::dasData(".data", mbc.data, output);
    DeAssembler::dasData(".data.string_literal", mbc.string_literal, output, true);
    DeAssembler::dasData(".data.thread_local", mbc.thread_local_, output);
    DeAssembler::dasData(".stack_init", mbc.stack_init, output);
}

void DeAssembler::dasAttribute(const MBC::Attribute& attribute, std::ostream& output)
{
    output << lib::format(".attribute\n\tVERSION ${}\n", QEStringView{attribute.version});
    switch (attribute.type) {
    case MBC::Type::object_file:
        output << "\tOBJECT\n";
        break;
    case MBC::Type::executable:
        output << "\tEXECUTABLE\n";
        break;
    case MBC::Type::fix_address_executable:
        output << "\tFIX_ADDRESS_EXECUTABLE\n";
        break;
    case MBC::Type::shared_object:
        output << "\tSHARED_OBJECT\n";
        break;
    }
    if (!attribute.module_or_entry_name.empty()) {
        auto attribute_name = attribute.type == MBC::Type::executable ||
                              attribute.type == MBC::Type::fix_address_executable ? "ENTRY" : "MODULE";
        output << lib::format("\t${} ${}\n", attribute_name, QEStringView{attribute.module_or_entry_name});
    }
    if (attribute.static_links.length() != 0) {
        output << "\tSTATIC_LINK [";
        for (size_t i = 0; i < attribute.static_links.length() - 1; ++i) {
            output << lib::format("${} ,", QEStringView{attribute.static_links[i]});
        }
        output << lib::format("${}]\n", QEStringView{attribute.static_links.back()});
    }
    if (attribute.dynamic_links.length() != 0) {
        output << "\tDYNAMIC_LINK [";
        for (size_t i = 0; i < attribute.dynamic_links.length() - 1; ++i) {
            output << lib::format("${} ,", QEStringView{attribute.dynamic_links[i]});
        }
        output << lib::format("${}]\n", QEStringView{attribute.dynamic_links.back()});
    }
}

void DeAssembler::dasComment(std::string_view comment, std::ostream& output)
{
    if (comment.empty()) {
        return;
    }
    output << lib::format(".comment\n\t${}\n", QEStringView{comment});
}

void DeAssembler::dasBSS(const MBC::BSS& bss, std::ostream& output)
{
    if (bss.len == 0) {
        return;
    }
    output << lib::format(".bss ALIGN ${} ${}\n", bss.align, bss.len);
}

void DeAssembler::dasData(std::string_view section_name, const MBC::Data& data, std::ostream& output, bool to_string)
{
    if (data.bin.empty()) {
        return;
    }
    output << lib::format("${} ALIGN ${}\n", section_name, data.align);
    if (to_string) {
        output << lib::format("\t${}\n", QEStringView{data.bin});
    } else {
        output << "\t0x";
        for (auto item: data.bin) {
            output << hexToChar(item >> 4) << hexToChar(item & 0xf);
        }
    }
    output << '\n';
}

void DeAssembler::dasObject(std::string_view section_name, const lib::Array<StaticObjectDescription>& objects,
                            std::ostream& output)
{
    if (objects.empty()) {
        return;
    }
    output << section_name << '\n';
    output << "\t[\n";
    for (const auto& item: objects) {
        output << "\t\t{\n"
               << lib::format("\t\t\tname: ${}\n", QEStringView{item.name})
               << lib::format("\t\t\taddress: ${} # ${x}\n", item.address, item.address)
               << lib::format("\t\t\ttype: ${}\n", *item.type)
               << "\t\t}\n";
    }
    output << "\t]\n";
}

void DeAssembler::dasFunction(const lib::Array<Function>& functions, std::ostream& output)
{
    output << ".function\n";
    output << "\t[\n";
    for (const auto& item: functions) {
        output << "\t\t{\n"
               << lib::format("\t\t\tname: ${}\n", QEStringView{item.name})
               << lib::format("\t\t\taddress: ${} # ${x}\n", item.address, item.address)
               << lib::format("\t\t\ttype: ${}\n", item.effective_type)
               << lib::format("\t\t\tframe_size: ${}\n", item.frame_size)
               << lib::format("\t\t\tcode_size: ${}\n", item.code_size)
               << lib::format("\t\t\tmax_object_num: ${}\n", item.max_object_num)
               << lib::format("\t\t\tfile_name: ${}\n", QEStringView{item.file_name})
               << "\t\t\tblocks: [\n";
        for (const auto& block: item.blocks) {
            DeAssembler::dasBlock(block, output);
        }
        output << "\t\t\t]\n"
               << "\t\t\tfull_expressions: [\n";
        for (const auto& full_expr: item.full_expr_infos) {
            DeAssembler::dasFullExpression(full_expr, output);
        }
        output << "\t\t\t]\n"
               << "\t\t\tdebug: [\n";
        DeAssembler::dasSourceLocation(item.func_locator, output);
        output << "\t\t\t]\n"
               << "\t\t}\n";
    }
    output << "\t]\n";
}

void DeAssembler::dasBlock(const Block& block, std::ostream& output)
{
    output << "\t\t\t\t[\n";
    for (const auto& item: block.obj_desc) {
        output << "\t\t\t\t\t{\n"
               << lib::format("\t\t\t\t\t\tname: ${}\n", QEStringView{item.name})
               << lib::format("\t\t\t\t\t\tdsg_id: ${}\n", item.id)
               << lib::format("\t\t\t\t\t\ttype: ${}\n", item.type)
               << lib::format("\t\t\t\t\t\toffset: ${} # ${x}\n", item.offset, item.offset)
               << "\t\t\t\t\t\tinit_data_offset: ";
        if (item.init_offset) {
            output << *item.init_offset;
        } else {
            output << -1;
        }
        output << "\n\t\t\t\t\t}\n";
    }
    output << "\t\t\t\t]\n";
}

void DeAssembler::dasFullExpression(const FullExprInfo& full_expr, std::ostream& output)
{
    output << "\t\t\t\t{\n"
           << lib::format("\t\t\t\t\ttrace_event_cnt: ${}\n", full_expr.trace_event_cnt)
           << "\t\t\t\t\tsource_location: [\n";
    for (const auto& item: full_expr.source_location) {
        output << lib::format("\t\t\t\t\t\t(${}, ${})\n", item.first, item.second);
    }
    output << "\t\t\t\t\t]\n"
           << "\t\t\t\t\tsequence_after: [\n";
    if (full_expr.sequence_after_graph.length() <
        lib::roundUpDiv(full_expr.trace_event_cnt * full_expr.trace_event_cnt, 8)) {
        log::unbuffered.eprintln("sequence_after_graph size(${}) do not match trace_event_cnt(${})."
                                 "The former must be great equal than the square of the latter div 8",
                                 full_expr.sequence_after_graph.length(), full_expr.trace_event_cnt);
    } else {
        for (size_t i = 0; i < full_expr.trace_event_cnt; ++i) {
            output << "\t\t\t\t\t\t[";
            std::vector<uint64_t> numbers;
            for (int j = 0; j < full_expr.trace_event_cnt; ++j) {
                auto idx = i * full_expr.trace_event_cnt + j;
                if (full_expr.sequence_after_graph[idx / 8] & (1 << (idx % 8))) {
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
    output << "\t\t\t\t\t]\n"
           << "\t\t\t\t}\n";
}

void DeAssembler::dasSourceLocation(const SourceCodeLocator& locator, std::ostream& output)
{
    for (const auto& item: locator.data) {
        output << lib::format("\t\t\t\t(${}, ${}, ${})\n", item.addr, item.len, item.line);
    }
}

void DeAssembler::dasTypes(const lib::Array<const ts::Type*>& types, std::ostream& output)
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

void DeAssembler::dasCode(std::string_view section_name, const MBC& mbc, const MBC::Code& code, std::ostream& output)
{
    if (code.bin.empty()) {
        return;
    }
    using am::FetchDecode;
    using am::Formatter;
    output << section_name << '\n';
    auto label_offsets = DeAssembler::getLabelOffsets(code);
    std::map<uint64_t, std::string_view> relocate_map;
    for (const auto& item: code.relocate) {
        relocate_map.emplace(item.first, item.second);
    }
    auto cur_label = label_offsets.begin();
    for (size_t i = 0; i < code.bin.length();) {
        if (cur_label != label_offsets.end() && i == *cur_label) {
            output << "L" << *cur_label << ":\n";
            cur_label++;
        }
        auto op = static_cast<Opcode>(code.bin[i]);
        output << '\t' << Formatter::opcode(op);
        if (!FetchDecode::hasExtraInfo(op)) {
            output << '\n';
            i++;
            continue;
        }
        i += 4;
        auto val = readI24(&code.bin[i - 3]);
        if (op == Opcode::j || op == Opcode::jst || op == Opcode::jnt) {
            auto itr = label_offsets.find(i + val);
            ASSERT(itr != label_offsets.end(), "you cannot possibly not find the element that is inserted before");
            output << " L" << *itr << '\n';
            continue;
        }
        if (mbc.attribute.type != MBC::Type::object_file) {
            DeAssembler::dasLinkedFileInstr(op, val, mbc, output);
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
}

std::set<uint64_t> DeAssembler::getLabelOffsets(const MBC::Code& code)
{
    using am::FetchDecode;
    std::set<uint64_t> label_offsets{};
    for (size_t i = 0; i < code.bin.length();) {
        auto op = static_cast<Opcode>(code.bin[i]);
        if (!FetchDecode::hasExtraInfo(op)) {
            i++;
            continue;
        }
        if (op == Opcode::j || op == Opcode::jst || op == Opcode::jnt) {
            label_offsets.insert(i + 4 + readI24(&code.bin[i + 1]));
        }
        i += 4;
    }
    return label_offsets;
}

void DeAssembler::dasLinkedFileInstr(am::Opcode op, uint64_t info, const MBC& mbc, std::ostream& output)
{
    using am::InstrInfo;
    switch (op) {
    case Opcode::new_:
    case Opcode::cast:
        if (info >= mbc.types.length()) {
            goto show_unknown;
        }
        output << lib::format(" ${}\n", *mbc.types[info]);
        return;
    case Opcode::push:
        if (info >= mbc.constants.length()) {
            goto show_unknown;
        }
        DeAssembler::dasConstant(mbc.constants[info], output);
        return;
    case Opcode::dsg: {
        auto identifierId = InstrInfo::IdentifierId{static_cast<uint32_t>(info)};
        if (!identifierId.isGlobal()) {
            output << ' ' << identifierId.value() << '\n';
            return;
        }
        if (identifierId.isFuntion()) {
            if (identifierId.value() >= mbc.code.functions.length()) {
                goto show_unknown;
            }
            output << ' ' << mbc.code.functions[identifierId.value()].name << '\n';
        } else {
            if (identifierId.value() >= mbc.data.objects.length()) {
                goto show_unknown;
            }
            output << ' ' << mbc.data.objects[identifierId.value()].name << '\n';
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

void DeAssembler::dasConstant(std::pair<const ts::Type*, uint64_t> constant, std::ostream& output)
{
    using namespace ts;
    auto type = constant.first;
    output << lib::format(" <${}; ", *type);
    switch (type->kind()) {
    case Kind::f32: {
        float value = *reinterpret_cast<const float*>(constant.second);
        output << value;
        break;
    }
    case Kind::f64: {
        double value = *reinterpret_cast<const double*>(constant.second);
        output << value;
        break;
    }
    case Kind::null:
        output << "null";
        break;
    default:
        if (!isInteger(type->kind())) {
            output << "???";
        } else {
            if (isUnsigned(type->kind())) {
                output << constant.second;
            } else {
                output << *reinterpret_cast<int64_t*>(&constant.second);
            }
        }
    }
    output << ">\n";
}
