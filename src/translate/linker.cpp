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

#include <linker.h>
#include <exception.h>
#include <lib/utils.h>
#include <lib/assert.h>
#include <foundation/type/mm.h>
#include <am/vmm.h>
#include <am/fetch_decode.h>
#include <set>
#include <map>
#include <cstring>
#include <functional>

#define FIELD_GETTER(field) [](const MBC* mbc) -> auto& { return mbc->field; } // NOLINT
// create a new traverser based on old traverser `traverser`,
//  the field_getter of new one is 'piped' after the old one
//  i.e. field_getter of old one is `mbc.xxx`, while the new one is `mbc.xxx.yyy`
#define PIPE_FIELD_GETTER(traverser, field) {traverser.mbcs, traverser.mbcs_len, [&](const MBC* mbc) -> auto& { return traverser.field_getter(mbc).field; }} // NOLINT

using namespace cami;
using namespace tr;
using Data = MBC::Data;
using BSS = MBC::BSS;
using Code = MBC::Code;
using am::spd::StaticObjectDescription;
using am::spd::Function;
using am::InstrInfo;

using u64_array_ref_t = const lib::Array<uint64_t>&;
using relo_entry_t = std::pair<uint64_t, std::string>;

namespace {

template<typename T_elem, typename T_filed_getter, typename T_set_elem = T_elem>
lib::Array<T_elem> uniquelyMerge(const lib::Array<const MBC*>& mbcs, const T_filed_getter& filed_getter)
{
    std::set<T_set_elem> set{};
    for (const MBC* item: mbcs) {
        for (const auto& i: filed_getter(item)) {
            set.insert(i);
        }
    }
    lib::Array<T_elem> result(set.size());
    uint64_t cnt = 0;
    for (const auto& item: set) {
        result[cnt++] = item;
    }
    return result;
}

lib::Array<const ts::Type*> mergeTypes(const lib::Array<const MBC*>& mbcs)
{
    constexpr auto field_getter = [](const MBC* mbc) -> auto& { return mbc->types; };
    return uniquelyMerge<const ts::Type*>(mbcs, field_getter);
}

lib::Array<std::pair<const ts::Type*, uint64_t>> mergeConstants(const lib::Array<const MBC*>& mbcs)
{
    constexpr auto field_getter = [](const MBC* mbc) -> auto& { return mbc->constants; };
    return uniquelyMerge<std::pair<const ts::Type*, uint64_t>>(mbcs, field_getter);
}

lib::Array<std::string> mergeStaticLinks(const lib::Array<const MBC*>& mbcs)
{
    std::set<std::string_view> files{};
    for (const MBC* mbc: mbcs) {
        for (const auto& item: mbc->attribute.static_links) {
            files.insert(item);
        }
    }
    for (const MBC* mbc: mbcs) {
        files.erase(mbc->source_name);
    }
    lib::Array<std::string> result(files.size());
    uint64_t cnt = 0;
    for (std::string_view item: files) {
        result.init(cnt++, item);
    }
    return result;
}

lib::Array<std::string> mergeDynamicLinks(const lib::Array<const MBC*>& mbcs)
{
    constexpr auto field_getter = [](const MBC* mbc) -> auto& { return mbc->attribute.dynamic_links; };
    return uniquelyMerge<std::string, decltype(field_getter), std::string_view>(mbcs, field_getter);
}

template<typename T>
void correctAddress(u64_array_ref_t bin_offs, u64_array_ref_t metadata_cnt, lib::Array<T>& entities)
{
    uint64_t cur = 0;
    for (size_t i = 0; i < metadata_cnt.length(); ++i) {
        for (int j = 0; j < metadata_cnt[i]; ++j) {
            entities[cur++].address += bin_offs[i];
        }
    }
}

void correctDebugInfo(am::spd::SourceCodeLocator& func_locator, uint64_t offset)
{
    for (auto& item: func_locator.data) {
        item.addr += offset;
    }
}

void correctDebugInfo(u64_array_ref_t code_offs, u64_array_ref_t func_cnt, lib::Array<Function>& functions)
{
    uint64_t cur = 0;
    for (size_t i = 0; i < func_cnt.length(); ++i) {
        for (int j = 0; j < func_cnt[i]; ++j) {
            correctDebugInfo(functions[cur++].func_locator, code_offs[i]);
        }
    }
}

void correctInitOffset(u64_array_ref_t init_data_offs, u64_array_ref_t func_cnt, lib::Array<Function>& functions)
{
    uint64_t cur = 0;
    for (size_t i = 0; i < func_cnt.length(); ++i) {
        for (int j = 0; j < func_cnt[i]; ++j) {
            for (auto& block: functions[cur].blocks) {
                for (auto& obj: block.obj_desc) {
                    if (obj.init_offset) {
                        *obj.init_offset += init_data_offs[i];
                    }
                }
            }
            cur++;
        }
    }
}

constexpr auto correctObject = correctAddress<StaticObjectDescription>;

void correctFunctions(u64_array_ref_t code_offs, u64_array_ref_t init_data_offs, u64_array_ref_t func_cnt,
                      lib::Array<Function>& functions)
{
    correctAddress(code_offs, func_cnt, functions);
    correctDebugInfo(code_offs, func_cnt, functions);
    correctInitOffset(init_data_offs, func_cnt, functions);
}

void correctRelocate(u64_array_ref_t code_offs, u64_array_ref_t relo_cnt, lib::Array<relo_entry_t>& relocates)
{
    uint64_t cur = 0;
    for (size_t i = 0; i < relo_cnt.length(); ++i) {
        for (int j = 0; j < relo_cnt[i]; ++j) {
            relocates[cur++].first += code_offs[i];
        }
    }
}

template<typename T>
auto mergeEntity(uint64_t entity_cnt, const Traverser<const lib::Array<T>>& traverser)
{
    lib::Array<T> entities(entity_cnt);
    uint64_t cur = 0;
    for (const auto& src_entities: traverser) {
        for (const auto& item: src_entities) {
            entities.init(cur++, item);
        }
    }
    return entities;
}

constexpr auto mergeObject = mergeEntity<StaticObjectDescription>;
constexpr auto mergeFunction = mergeEntity<Function>;

lib::Array<uint8_t> mergeDataBin(uint64_t bin_len, const Traverser<const Data>& traverser, u64_array_ref_t data_offs)
{
    lib::Array<uint8_t> bin(bin_len);
    uint64_t i = 0;
    uint64_t cur;
    for (const auto& src_data: traverser) {
        cur = data_offs[i];
        std::memcpy(bin.data() + cur, src_data.bin.data(), src_data.bin.length());
        cur += src_data.bin.length();
        std::memset(bin.data() + cur, 0, data_offs[i + 1] - cur);
        i++;
    }
    return bin;
}

lib::Array<uint8_t> mergeCodeBin(uint64_t code_len, const Traverser<const Code>& traverser)
{
    lib::Array<uint8_t> code(code_len);
    uint64_t cur = 0;
    for (const auto& src_data: traverser) {
        std::memcpy(code.data() + cur, src_data.bin.data(), src_data.bin.length());
        cur += src_data.bin.length();
    }
    return code;
}

auto mergeRelocate(uint64_t relocate_cnt, const Traverser<const lib::Array<relo_entry_t>>& traverser)
{
    lib::Array<relo_entry_t> relocate(relocate_cnt);
    uint64_t cur = 0;
    for (const auto& src_relocate: traverser) {
        for (const auto& item: src_relocate) {
            relocate.init(cur++, item);
        }
    }
    return relocate;
}

DataMergeResult mergeData(const Traverser<const Data>& traverser)
{
    lib::Array<uint64_t> data_offsets(traverser.mbcs_len + 1);
    lib::Array<uint64_t> object_cnt(traverser.mbcs_len);
    uint64_t total_len = 0;
    uint64_t align = 0;
    uint64_t total_object_cnt = 0;
    uint64_t i = 0;
    for (const auto& src_data: traverser) {
        data_offsets[i] = lib::roundUp(total_len, src_data.align);
        object_cnt[i] = src_data.objects.length();
        total_len = data_offsets[i] + src_data.bin.length();
        align = std::max(align, src_data.align);
        total_object_cnt += src_data.objects.length();
        i++;
    }
    data_offsets[traverser.mbcs_len] = total_len;
    auto data_bin = mergeDataBin(total_len, traverser, data_offsets);
    auto objects = mergeObject(total_object_cnt, PIPE_FIELD_GETTER(traverser, objects));
    return {{align, std::move(data_bin), std::move(objects)}, std::move(data_offsets), std::move(object_cnt)};
}

BssMergeResult mergeBss(const Traverser<const BSS>& traverser)
{
    lib::Array<uint64_t> bss_offsets(traverser.mbcs_len);
    lib::Array<uint64_t> object_cnt(traverser.mbcs_len);
    uint64_t total_len = 0;
    uint64_t align = 0;
    uint64_t total_object_cnt = 0;
    uint64_t i = 0;
    for (const auto& src_bss: traverser) {
        bss_offsets[i] = lib::roundUp(total_len, src_bss.align);
        object_cnt[i] = src_bss.objects.length();
        total_len = bss_offsets[i] + src_bss.len;
        align = std::max(align, src_bss.align);
        total_object_cnt += src_bss.objects.length();
        i++;
    }
    auto objects = mergeObject(total_object_cnt, PIPE_FIELD_GETTER(traverser, objects));
    return {{align, total_len, std::move(objects)}, std::move(bss_offsets), std::move(object_cnt)};
}

CodeMergeResult mergeCode(const Traverser<const Code>& traverser)
{
    lib::Array<uint64_t> code_offsets(traverser.mbcs_len);
    lib::Array<uint64_t> function_cnt(traverser.mbcs_len);
    lib::Array<uint64_t> relocate_cnt(traverser.mbcs_len);
    uint64_t total_len = 0;
    uint64_t total_function_cnt = 0;
    uint64_t total_relocate_cnt = 0;
    uint64_t i = 0;
    for (const auto& src_code: traverser) {
        code_offsets[i] = total_len;
        function_cnt[i] = src_code.functions.length();
        relocate_cnt[i] = src_code.relocate.length();
        total_len += src_code.bin.length();
        total_function_cnt += src_code.functions.length();
        total_relocate_cnt += src_code.relocate.length();
        i++;
    }
    auto code_bin = mergeCodeBin(total_len, traverser);
    auto functions = mergeFunction(total_function_cnt, PIPE_FIELD_GETTER(traverser, functions));
    auto relocates = mergeRelocate(total_relocate_cnt, PIPE_FIELD_GETTER(traverser, relocate));
    return {{std::move(code_bin), std::move(functions), std::move(relocates)},
            std::move(code_offsets), std::move(function_cnt), std::move(relocate_cnt)};
}

MBC::Attribute linkAttribute(const lib::Array<const MBC*>& mbcs, MBC::Type link_type)
{
    std::string module_or_entry_name;
    for (const MBC* mbc: mbcs) {
        if (!mbc->attribute.module_or_entry_name.empty()) {
            if (!module_or_entry_name.empty() && mbc->attribute.module_or_entry_name != module_or_entry_name) {
                throw ConflictModuleOrEntryException{};
            }
            module_or_entry_name = mbc->attribute.module_or_entry_name;
        }
    }
    if (link_type != MBC::Type::object_file && module_or_entry_name.empty()) {
        throw MissingModuleOrEntryNameException{};
    }
    return {
            .version = mbcs[0]->attribute.version,
            .type = link_type,
            .module_or_entry_name = std::move(module_or_entry_name),
            .entry = nullptr,
            .static_links = mergeStaticLinks(mbcs),
            .dynamic_links = mergeDynamicLinks(mbcs),
    };
}

BSS linkBss(const Traverser<const BSS>& traverser)
{
    auto bss_result = mergeBss(traverser);
    correctObject(bss_result.bin_offsets, bss_result.object_cnt, bss_result.bss.objects);
    return std::move(bss_result.bss);
}

Data linkData(const Traverser<const Data>& traverser)
{
    auto data_result = mergeData(traverser);
    correctObject(data_result.bin_offsets, data_result.object_cnt, data_result.data.objects);
    return std::move(data_result.data);
}

Code linkCode(const Traverser<const Code>& traverser, const DataMergeResult& stack_init_result)
{
    auto code_result = mergeCode(traverser);
    if (code_result.function_cnt.length() > 0) {
        correctFunctions(code_result.bin_offs, stack_init_result.bin_offsets, code_result.function_cnt,
                         code_result.code.functions);
    }
    correctRelocate(code_result.bin_offs, code_result.relocate_cnt, code_result.code.relocate);
    return std::move(code_result.code);
}

std::map<std::string, uint64_t> makeRelocateMapper(const lib::Array<const ts::Type*>& types,
                                                   const lib::Array<std::pair<const ts::Type*, uint64_t>>& constants,
                                                   const Code& code, const BSS& bss, const Data& data,
                                                   const Data& string_literal, const Data& thread_local_)
{
    std::map<std::string, uint64_t> result;
    for (size_t i = 0; i < types.length(); ++i) {
        result.emplace(lib::format("#${}", *types[i]), i);
    }
    for (size_t i = 0; i < constants.length(); ++i) {
        result.emplace(lib::format("<${}; ${}>", *constants[i].first, constants[i].second), i);
    }
    for (size_t i = 0; i < code.functions.length(); ++i) {
        if (!result.emplace(code.functions[i].name, InstrInfo::IdentifierId::fromFunctionIndex(i)).second) {
            throw DuplicatedSymbolException{code.functions[i].name};
        }
    }
    uint64_t obj_cnt = 0;
    for (const auto& item: data.objects) {
        if (!result.emplace(item.name, InstrInfo::IdentifierId::fromStaticObject(obj_cnt++)).second) {
            throw DuplicatedSymbolException{item.name};
        }
    }
    for (const auto& item: bss.objects) {
        if (!result.emplace(item.name, InstrInfo::IdentifierId::fromStaticObject(obj_cnt++)).second) {
            throw DuplicatedSymbolException{item.name};
        }
    }
    for (const auto& item: string_literal.objects) {
        if (!result.emplace(item.name, InstrInfo::IdentifierId::fromStaticObject(obj_cnt++)).second) {
            throw DuplicatedSymbolException{item.name};
        }
    }
    for (const auto& item: thread_local_.objects) {
        if (!result.emplace(item.name, InstrInfo::IdentifierId::fromStaticObject(obj_cnt++)).second) {
            throw DuplicatedSymbolException{item.name};
        }
    }
    return result;
}

void do_relocate(Code& code, const std::map<std::string, uint64_t>& relocate_mapper)
{
    for (const auto& [offset, symbol]: code.relocate) {
        auto itr = relocate_mapper.find(symbol);
        if (itr == relocate_mapper.end()) {
            throw CannotFoundSymbolException{symbol};
        }
        auto id = itr->second;
        code.bin[offset + 1] = static_cast<uint8_t>(id);
        code.bin[offset + 2] = static_cast<uint8_t>(id >> 8);
        code.bin[offset + 3] = static_cast<uint8_t>(id >> 16);
    }
    code.relocate.clear();
}

void relocate(const lib::Array<const ts::Type*>& types,
              const lib::Array<std::pair<const ts::Type*, uint64_t>>& constants,
              Code& code, Code& init_code, Code& thread_local_init_code,
              const BSS& bss, const Data& data, const Data& string_literal, const Data& thread_local_)
{
    auto relocate_mapper = makeRelocateMapper(types, constants, code, bss, data, string_literal, thread_local_);
    do_relocate(code, relocate_mapper);
    do_relocate(init_code, relocate_mapper);
    do_relocate(thread_local_init_code, relocate_mapper);
}

void mergeInitCode(Code& code, Code& init_code, Code& thread_local_init_code)
{
    lib::Array<uint8_t> bin(code.bin.length() + init_code.bin.length() + thread_local_init_code.bin.length() + 2);
    std::memcpy(bin.data(), code.bin.data(), code.bin.length());
    std::memcpy(bin.data() + code.bin.length(), init_code.bin.data(), init_code.bin.length());
    bin[code.bin.length() + init_code.bin.length()] = static_cast<uint8_t>(am::Opcode::ret);
    std::memcpy(bin.data() + code.bin.length() + init_code.bin.length() + 1, thread_local_init_code.bin.data(),
                thread_local_init_code.bin.length());
    bin[code.bin.length() + init_code.bin.length() + 1 + thread_local_init_code.bin.length()]
            = static_cast<uint8_t>(am::Opcode::ret);
    code.bin.assign(std::move(bin));
    // generate function
    lib::Array<Function> functions(code.functions.length() + 2);
    uint64_t i = 0;
    for (auto& item: code.functions) {
        functions.init(i++, std::move(item));
    }
    uint64_t address = functions[i - 1].address + functions[i - 1].code_size;
    auto& void_ = ts::type_manager.getBasicType(ts::Kind::void_);
    auto& void_f_void = ts::type_manager.getFunction(void_, {});
    new(&functions[i]) Function{"__init__", void_f_void, address, "<no file>", 0, init_code.bin.length() + 1,
                                0, {{}}, {}, {}};
    address += init_code.bin.length() + 1;
    new(&functions[i + 1]) Function{"__init_thread_local__", void_f_void, address, "<no file>", 0,
                                    thread_local_init_code.bin.length() + 1, 0, {{}}, {}, {}};
    code.functions.assign(std::move(functions));
    init_code.clear();
    thread_local_init_code.clear();
}

// merge object of data bss string_literal together, do correction
void mergeBssStringLiteralObject(Data& data, BSS& bss, Data& string_literal)
{
    lib::Array<StaticObjectDescription> static_objects(
            data.objects.length() + bss.objects.length() + string_literal.objects.length());
    uint64_t cnt = 0;
    for (const auto& item: data.objects) {
        static_objects.init(cnt++, item);
    }
    for (const auto& item: bss.objects) {
        static_objects.init(cnt++, item);
    }
    for (const auto& item: string_literal.objects) {
        static_objects.init(cnt++, item);
    }
    uint64_t bss_offset = lib::roundUp(data.bin.length(), bss.align);
    uint64_t string_literal_offset = lib::roundUp(bss_offset + bss.len, string_literal.align);
    correctObject({0, bss_offset, string_literal_offset},
                  {data.objects.length(), bss.objects.length(), string_literal.objects.length()}, static_objects);
    data.objects.assign(std::move(static_objects));
    bss.objects.clear();
    string_literal.objects.clear();
}

// merge bin of data bss string_literal together
void makeDataMemoryLayout(Data& data, const BSS& bss, const Data& string_literal)
{
    uint64_t bss_offset = lib::roundUp(data.bin.length(), bss.align);
    uint64_t string_literal_offset = lib::roundUp(bss_offset + bss.len, string_literal.align);
    lib::Array<uint8_t> bin(string_literal_offset + string_literal.bin.length());
    auto dest = bin.data();
    std::memcpy(dest, data.bin.data(), data.bin.length());
    dest += data.bin.length();
    std::memset(dest, 0, string_literal_offset - data.bin.length());
    dest = bin.data() + string_literal_offset;
    std::memcpy(dest, string_literal.bin.data(), string_literal.bin.length());
    data.bin.assign(std::move(bin));
}

void insertBootCode(Code& code, uint64_t entry_id)
{
    ASSERT(code.functions[code.functions.length() - 2].name == "__init__" &&
           code.functions[code.functions.length() - 1].name == "__init_thread_local__", "assumption violation");
    using am::Opcode;
    using am::InstrInfo;
    constexpr uint64_t BOOT_CODE_SIZE = 40;
    auto init_id = InstrInfo::IdentifierId::fromFunctionIndex(code.functions.length() - 2);
    auto thread_local_init_id = InstrInfo::IdentifierId::fromFunctionIndex(code.functions.length() - 1);
    lib::Array<uint8_t> bin(code.bin.length() + BOOT_CODE_SIZE);
    // fe 0
    // dsg __init__
    // addr
    // call 0
    // fe 1
    // dsg __init_thread_local__
    // addr
    // call 0
    // fe 2
    // dsg <entry>
    // addr
    // call 0
    // halt
    bin[0] = static_cast<uint8_t>(Opcode::fe);
    bin[1] = 0;
    bin[2] = 0;
    bin[3] = 0;
    bin[4] = static_cast<uint8_t>(Opcode::dsg);
    bin[5] = static_cast<uint8_t>(init_id);
    bin[6] = static_cast<uint8_t>(init_id >> 8);
    bin[7] = static_cast<uint8_t>(init_id >> 16);
    bin[8] = static_cast<uint8_t>(Opcode::addr);
    bin[9] = static_cast<uint8_t>(Opcode::call);
    bin[10] = 0;
    bin[11] = 0;
    bin[12] = 0;
    bin[13] = static_cast<uint8_t>(Opcode::fe);
    bin[14] = 1;
    bin[15] = 0;
    bin[16] = 0;
    bin[17] = static_cast<uint8_t>(Opcode::dsg);
    bin[18] = static_cast<uint8_t>(thread_local_init_id);
    bin[19] = static_cast<uint8_t>(thread_local_init_id >> 8);
    bin[20] = static_cast<uint8_t>(thread_local_init_id >> 16);
    bin[21] = static_cast<uint8_t>(Opcode::addr);
    bin[22] = static_cast<uint8_t>(Opcode::call);
    bin[23] = 0;
    bin[24] = 0;
    bin[25] = 0;
    bin[26] = static_cast<uint8_t>(Opcode::fe);
    bin[27] = 2;
    bin[28] = 0;
    bin[29] = 0;
    bin[30] = static_cast<uint8_t>(Opcode::dsg);
    bin[31] = static_cast<uint8_t>(entry_id);
    bin[32] = static_cast<uint8_t>(entry_id >> 8);
    bin[33] = static_cast<uint8_t>(entry_id >> 16);
    bin[34] = static_cast<uint8_t>(Opcode::addr);
    bin[35] = static_cast<uint8_t>(Opcode::call);
    bin[36] = 0;
    bin[37] = 0;
    bin[38] = 0;
    bin[39] = static_cast<uint8_t>(Opcode::halt);
    std::memcpy(bin.data() + BOOT_CODE_SIZE, code.bin.data(), code.bin.length());
    code.bin.assign(std::move(bin));
    lib::Array<Function> functions(code.functions.length() + 1);
    auto& void_ = ts::type_manager.getBasicType(ts::Kind::void_);
    auto& void_f_void = ts::type_manager.getFunction(void_, {});
    new(&functions.back()) Function{"__boot__", void_f_void, 0, "<no file>", 0, BOOT_CODE_SIZE,
                                                     0, {{}}, {}, {}};
    for (size_t i = 0; i < code.functions.length(); ++i) {
        functions.init(i, std::move(code.functions[i]));
    }
    correctFunctions({BOOT_CODE_SIZE}, {0}, {code.functions.length()}, functions);
    code.functions.assign(std::move(functions));
}

void fixAddress(Data& data, Code& code)
{
    correctObject({am::layout::DATA_BASE}, {data.objects.length()}, data.objects);
    correctFunctions({am::layout::CODE_BASE}, {0}, {code.functions.length()}, code.functions);
}

std::pair<uint64_t, const Function*> findEntry(const Code& code, std::string_view entry_name)
{
    auto itr = std::find_if(code.functions.begin(), code.functions.end(), [&](const Function& item) {
        return item.name == entry_name;
    });
    if (itr == code.functions.end()) {
        throw CannotFoundSymbolException{entry_name};
    }
    auto func = &*itr;
    return {InstrInfo::IdentifierId::fromFunctionIndex(func - code.functions.data()), func};
}

lib::Array<ValueBox> makeConstants(const lib::Array<std::pair<const ts::Type*, uint64_t>>& constants)
{
    using ts::Kind;
    lib::Array<ValueBox> result(constants.length());
    for (size_t i = 0; i < constants.length(); ++i) {
        auto [type, value] = constants[i];
        switch (type->kind()) {
        case Kind::f32: {
            float val = 0;
            std::memcpy(&val, &value, 4);
            result.init(i, new F32Value{val});
            break;
        }
        case Kind::f64: {
            double val = 0;
            std::memcpy(&val, &value, 8);
            result.init(i, new F64Value{val});
            break;
        }
        case Kind::null:
            result.init(i, new NullValue{});
            break;
        default:
            if (!isInteger(type->kind())) {
                throw CannotMakeConstantException{*type};
            }
            result.init(i, new IntegerValue{type, value});
        }
    }
    return result;
}

} // anonymous namespace

std::unique_ptr<MBC> Linker::link(const lib::Array<const MBC*>& mbcs, const LinkOption& option)
{
    ASSERT(mbcs.length() > 0, "cannot link empty files");
    ASSERT(std::all_of(mbcs.begin(), mbcs.end(),
                       [](const MBC* mbc) { return mbc->attribute.type == MBC::Type::object_file; }),
           "only object file can be linked");
    auto attribute = linkAttribute(mbcs, option.type);
    ASSERT(option.type == MBC::Type::object_file || attribute.static_links.empty(), "missing static link files");
    std::string file_name = "<no file: linker generated>";
    std::string comment = "linked by CAMI";
    auto types = mergeTypes(mbcs);
    auto constants = mergeConstants(mbcs);
    auto bss = linkBss({mbcs, FIELD_GETTER(bss)});
    auto data = linkData({mbcs, FIELD_GETTER(data)});
    auto string_literal = linkData({mbcs, FIELD_GETTER(string_literal)});
    auto thread_local_ = linkData({mbcs, FIELD_GETTER(thread_local_)});
    auto stack_init_result = mergeData({mbcs, FIELD_GETTER(stack_init)});
    auto code = linkCode({mbcs, FIELD_GETTER(code)}, stack_init_result);
    auto init_code = linkCode({mbcs, FIELD_GETTER(init_code)}, stack_init_result);
    auto thread_local_init_code = linkCode({mbcs, FIELD_GETTER(thread_local_init_code)}, stack_init_result);
    if (types.length() > InstrInfo::ID_MAX) {
        throw IDExceedException{"type"};
    }
    if (constants.length() > InstrInfo::ID_MAX) {
        throw IDExceedException{"constants"};
    }
    if (code.functions.length() + 2 > InstrInfo::FUNCTION_ID_MAX) {
        throw IDExceedException{"functions"};
    }
    if (bss.objects.length() + data.objects.length() + string_literal.objects.length() >
        InstrInfo::STATIC_OBJECT_ID_MAX) {
        throw IDExceedException{"objects"};
    }
    if (option.type != MBC::Type::object_file) {
        relocate(types, constants, code, init_code, thread_local_init_code, bss, data, string_literal, thread_local_);
        mergeInitCode(code, init_code, thread_local_init_code);
        mergeBssStringLiteralObject(data, bss, string_literal);
        if (option.type == MBC::Type::executable || option.type == MBC::Type::fix_address_executable) {
            auto [entry_id, entry_func] = findEntry(code, attribute.module_or_entry_name);
            attribute.entry = entry_func;
            insertBootCode(code, entry_id);
            if (option.type == MBC::Type::fix_address_executable) {
                fixAddress(data, code);
            }
        }
    }
    return std::make_unique<MBC>(std::move(file_name), std::move(attribute), std::move(comment), std::move(bss),
                                 std::move(data),
                                 std::move(string_literal), std::move(stack_init_result.data), std::move(thread_local_),
                                 std::move(code),
                                 std::move(init_code), std::move(thread_local_init_code), std::move(types),
                                 std::move(constants));
}

am::spd::InitializeDescription Linker::spawn(std::unique_ptr<MBC> _mbc)
{
    using ts::Type;
    auto& mbc = *_mbc;
    ASSERT(mbc.attribute.type != MBC::Type::object_file, "cannot spawn from object file");
    ASSERT(mbc.attribute.type != MBC::Type::shared_object, "not supported now");
    if (mbc.attribute.type != MBC::Type::fix_address_executable) {
        fixAddress(mbc.data, mbc.code);
    }
    auto bss_offset = lib::roundUp(mbc.data.bin.length(), mbc.bss.align);
    uint64_t string_literal_begin =
            am::layout::DATA_BASE + lib::roundUp(bss_offset + mbc.bss.len, mbc.string_literal.align);
    makeDataMemoryLayout(mbc.data, mbc.bss, mbc.string_literal);
    lib::Array<ValueBox> constants = makeConstants(mbc.constants);
    return {std::move(mbc.code.bin), std::move(mbc.data.bin), string_literal_begin, std::move(mbc.data.objects),
            std::move(constants), std::move(mbc.types), std::move(mbc.code.functions), std::move(mbc.stack_init.bin)};
}
