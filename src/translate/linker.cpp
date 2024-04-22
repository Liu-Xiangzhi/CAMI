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
#include <lib/slice.h>
#include <foundation/type/mm.h>
#include <am/fetch_decode.h>
#include <am/spd.h>
#include <set>
#include <map>
#include <cstring>

using namespace cami;
using namespace tr;
using am::spd::StaticObjectDescription;
using am::spd::Function;
using am::InstrInfo;

namespace {
enum class UnlinkedMBCField
{
    type, constant, dynamic_link
};
template<UnlinkedMBCField field>
struct FieldGetter;

template<>
struct FieldGetter<UnlinkedMBCField::type>
{
    using type = const ts::Type*;

    static std::vector<type>& get(UnlinkedMBC& unlinked_mbc)
    {
        return unlinked_mbc.types;
    }
};

template<>
struct FieldGetter<UnlinkedMBCField::constant>
{
    using type = std::pair<const ts::Type*, uint64_t>;

    static std::vector<type>& get(UnlinkedMBC& unlinked_mbc)
    {
        return unlinked_mbc.constants;
    }
};

template<>
struct FieldGetter<UnlinkedMBCField::dynamic_link>
{
    using type = std::string;

    static std::vector<type>& get(UnlinkedMBC& unlinked_mbc)
    {
        return unlinked_mbc.attribute.dynamic_links;
    }
};

template<UnlinkedMBCField filed>
std::vector<typename FieldGetter<filed>::type> uniquelyMerge(lib::Slice<std::unique_ptr<UnlinkedMBC>> mbcs)
{
    std::set<typename FieldGetter<filed>::type> set{};
    for (auto& item: mbcs) {
        for (auto& i: FieldGetter<filed>::get(*item)) {
            set.emplace(std::move(i));
        }
    }
    return {std::make_move_iterator(set.begin()), std::make_move_iterator(set.end())};
}

std::vector<const ts::Type*> mergeTypes(lib::Slice<std::unique_ptr<UnlinkedMBC>> mbcs)
{
    return uniquelyMerge<UnlinkedMBCField::type>(mbcs);
}

std::vector<std::pair<const ts::Type*, uint64_t>> mergeConstants(lib::Slice<std::unique_ptr<UnlinkedMBC>> mbcs)
{
    return uniquelyMerge<UnlinkedMBCField::constant>(mbcs);
}

std::vector<std::string> mergeStaticLinks(lib::Slice<std::unique_ptr<UnlinkedMBC>>& mbcs)
{
    std::set<std::string> files{};
    for (auto& mbc: mbcs) {
        for (auto& item: mbc->attribute.static_links) {
            files.emplace(std::move(item));
        }
    }
    for (const auto& mbc: mbcs) {
        files.erase(mbc->source_name);
    }
    return {std::make_move_iterator(files.begin()), std::make_move_iterator(files.end())};
}

std::vector<std::string> mergeDynamicLinks(lib::Slice<std::unique_ptr<UnlinkedMBC>> mbcs)
{
    return uniquelyMerge<UnlinkedMBCField::dynamic_link>(mbcs);
}

MBC::Attribute linkAttribute(lib::Slice<std::unique_ptr<UnlinkedMBC>> mbcs, MBC::Type link_type)
{
    std::string module_or_entry_name;
    for (const auto& mbc: mbcs) {
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

struct ObjectLayout
{
//  string_literal_begin == 0
    uint64_t data_begin;
    uint64_t bss_begin;
    uint64_t thread_local_begin;

    [[nodiscard]] uint64_t staticObjectCnt() const noexcept
    {
        return this->thread_local_begin;
    }
};

auto mergeObjects(lib::Slice<std::unique_ptr<UnlinkedMBC>> mbcs)
-> std::pair<std::vector<std::unique_ptr<UnlinkedMBC::StaticObject>>, std::map<std::string, std::string>>
{
    std::map<std::string, std::string> string_literal_name_map;
    std::map<std::string_view, std::string> string_literal_value_to_name;
    std::vector<std::unique_ptr<UnlinkedMBC::StaticObject>> objects;
    for (auto& item: mbcs) {
        for (auto& obj: item->objects) {
            if (obj->segment == UnlinkedMBC::StaticObject::string_literal) {
                std::string_view sv{reinterpret_cast<const char*>(obj->value.data()), obj->value.size()};
                if (auto itr = string_literal_value_to_name.find(sv);itr != string_literal_value_to_name.end()) {
                    string_literal_name_map.emplace(obj->name, itr->second);
                    continue;
                } else {
                    string_literal_value_to_name.emplace(sv, obj->name);
                }
            }
            objects.push_back(std::move(obj));
        }
    }
    return {std::move(objects), std::move(string_literal_name_map)};
}

std::vector<std::unique_ptr<UnlinkedMBC::Function>> mergeFunctions(lib::Slice<std::unique_ptr<UnlinkedMBC>> mbcs)
{
    std::vector<std::unique_ptr<UnlinkedMBC::Function>> functions;
    for (auto& item: mbcs) {
        for (auto& func: item->functions) {
            functions.push_back(std::move(func));
        }
    }
    return std::move(functions);
}

void relocateStringLiteralReference(const std::map<std::string, std::string>& string_literal_name_map,
                                    std::vector<std::unique_ptr<UnlinkedMBC::StaticObject>>& objects,
                                    std::vector<std::unique_ptr<UnlinkedMBC::Function>>& functions)
{
    for (auto& item: functions) {
        for (auto& relo: item->relocate) {
            if (auto itr = string_literal_name_map.find(relo.symbol); itr != string_literal_name_map.end()) {
                relo.symbol = itr->second;
            }
        }
    }
    for (auto& item: objects) {
        if (auto itr = string_literal_name_map.find(item->relocate_symbol); itr != string_literal_name_map.end()) {
            item->relocate_symbol = itr->second;
        }
    }
}

std::unique_ptr<UnlinkedMBC> makeUnlinkedMBC(lib::Slice<std::unique_ptr<UnlinkedMBC>> mbcs, const LinkOption& option)
{
    auto attribute = linkAttribute(mbcs, option.type);
    ASSERT(option.type == MBC::Type::object_file || attribute.static_links.empty(), "missing static link files");
    auto [objects, string_literal_name_map] = mergeObjects(mbcs);
    auto functions = mergeFunctions(mbcs);
    relocateStringLiteralReference(string_literal_name_map, objects, functions);
    return std::make_unique<UnlinkedMBC>("", std::move(attribute), "linked by CAMI Linker",
                                         std::move(objects), std::move(functions), mergeTypes(mbcs),
                                         mergeConstants(mbcs));
}

void insertBootFunction(UnlinkedMBC& unlinked_mbc)
{
    using am::Opcode;
    auto func = std::make_unique<UnlinkedMBC::Function>();
    func->segment = UnlinkedMBC::Function::execute;
    func->name = "__boot__";
    func->file_name = "<no file>";
    auto& void_ = ts::type_manager.getBasicType(ts::Kind::void_);
    func->effective_type = &ts::type_manager.getFunction(void_, {});
    func->frame_size = 0;
    func->max_object_num = 0;
    func->blocks.emplace_back();
    uint64_t init_func_cnt = 0;
    func->code.push_back(static_cast<uint8_t>(Opcode::fe));
    func->code.push_back(0);
    func->code.push_back(0);
    func->code.push_back(0);
    for (const auto& item: unlinked_mbc.functions) {
        if (item->segment == UnlinkedMBC::Function::init || item->segment == UnlinkedMBC::Function::thread_local_init) {
            func->relocate.emplace_back(func->code.size(), item->name);
            func->code.push_back(static_cast<uint8_t>(Opcode::dsg));
            func->code.push_back(-1);
            func->code.push_back(-1);
            func->code.push_back(-1);
            func->code.push_back(static_cast<uint8_t>(Opcode::addr));
            func->code.push_back(static_cast<uint8_t>(Opcode::call));
            func->code.push_back(init_func_cnt & 0xff);
            func->code.push_back((init_func_cnt >> 8) & 0xff);
            func->code.push_back((init_func_cnt >> 16) & 0xff);
            init_func_cnt++;
        }
    }
    func->relocate.emplace_back(func->code.size(), unlinked_mbc.attribute.module_or_entry_name);
    func->code.push_back(static_cast<uint8_t>(Opcode::dsg));
    func->code.push_back(-1);
    func->code.push_back(-1);
    func->code.push_back(-1);
    func->code.push_back(static_cast<uint8_t>(Opcode::addr));
    func->code.push_back(static_cast<uint8_t>(Opcode::call));
    func->code.push_back(init_func_cnt & 0xff);
    func->code.push_back((init_func_cnt >> 8) & 0xff);
    func->code.push_back((init_func_cnt >> 16) & 0xff);
    func->code.push_back(static_cast<uint8_t>(Opcode::halt));
    UnlinkedMBC::FullExprInfo full_expr_info{
            .trace_event_cnt = init_func_cnt + 1,
            .sequence_after_graph = std::vector<uint8_t>(lib::roundUpDiv((init_func_cnt + 1) * (init_func_cnt + 1), 8)),
            .source_location = std::vector<std::pair<uint64_t, uint64_t>>(init_func_cnt + 1),
    };
    for (size_t i = 0; i < init_func_cnt + 1; ++i) {
        for (size_t j = 0; j < i; ++j) {
            auto idx = i * (init_func_cnt + 1) + j;
            full_expr_info.sequence_after_graph[idx / 8] |= 1 << (idx % 8);
        }
    }
    func->full_expr_infos.push_back(std::move(full_expr_info));
    unlinked_mbc.functions.push_back(std::move(func));
    std::swap(unlinked_mbc.functions.back(), unlinked_mbc.functions.front());
}

void arrangeFunction(std::vector<std::unique_ptr<UnlinkedMBC::Function>>& functions)
{
    uint64_t exec_func_cnt = 0;
    uint64_t init_func_cnt = 0;
    for (const auto& item: functions) {
        if (item->segment == UnlinkedMBC::Function::execute) {
            exec_func_cnt++;
        } else if (item->segment == UnlinkedMBC::Function::init) {
            init_func_cnt++;
        }
    }
    uint64_t cur_exec = 0;
    uint64_t cur_init = exec_func_cnt;
    uint64_t cur_thread_local_init = exec_func_cnt + init_func_cnt;
    for (auto& item: functions) {
        if (item->segment == UnlinkedMBC::Function::execute) {
            std::swap(item, functions[cur_exec++]);
        } else if (item->segment == UnlinkedMBC::Function::init) {
            std::swap(item, functions[cur_init++]);
        } else {
            std::swap(item, functions[cur_thread_local_init++]);
        }
    }
}

ObjectLayout arrangeObject(std::vector<std::unique_ptr<UnlinkedMBC::StaticObject>>& objects)
{
    uint64_t string_literal_cnt = 0;
    uint64_t data_cnt = 0;
    uint64_t bss_cnt = 0;
    for (const auto& item: objects) {
        if (item->segment == UnlinkedMBC::StaticObject::string_literal) {
            string_literal_cnt++;
        } else if (item->segment == UnlinkedMBC::StaticObject::data) {
            data_cnt++;
        } else if (item->segment == UnlinkedMBC::StaticObject::bss) {
            bss_cnt++;
        }
    }
    uint64_t cur_string_literal = 0;
    uint64_t cur_data = string_literal_cnt;
    uint64_t cur_bss = string_literal_cnt + data_cnt;
    uint64_t cur_thread_local = string_literal_cnt + data_cnt + bss_cnt;
    for (auto& item: objects) {
        if (item->segment == UnlinkedMBC::StaticObject::string_literal) {
            std::swap(item, objects[cur_string_literal++]);
        } else if (item->segment == UnlinkedMBC::StaticObject::data) {
            std::swap(item, objects[cur_data++]);
        } else if (item->segment == UnlinkedMBC::StaticObject::bss) {
            std::swap(item, objects[cur_bss++]);
        } else {
            std::swap(item, objects[cur_thread_local++]);
        }
    }
    return {string_literal_cnt, string_literal_cnt + data_cnt, string_literal_cnt + data_cnt + bss_cnt};
}

ObjectLayout arrange(UnlinkedMBC& unlinked_mbc)
{
    arrangeFunction(unlinked_mbc.functions);
    return arrangeObject(unlinked_mbc.objects);
}

void relocate(UnlinkedMBC& unlinked_mbc, const uint64_t static_object_cnt)
{
    std::map<std::string, uint64_t> sym_map;
    for (size_t i = 0; i < unlinked_mbc.types.size(); ++i) {
        sym_map.emplace(lib::format("#${}", *unlinked_mbc.types[i]), i);
    }
    for (size_t i = 0; i < unlinked_mbc.constants.size(); ++i) {
        sym_map.emplace(lib::format("<${}; ${}>", *unlinked_mbc.constants[i].first, unlinked_mbc.constants[i].second),
                        i);
    }
    for (size_t i = 0; i < unlinked_mbc.functions.size(); ++i) {
        if (!sym_map.emplace(unlinked_mbc.functions[i]->name, InstrInfo::IdentifierId::fromFunctionIndex(i)).second) {
            throw DuplicatedSymbolException{unlinked_mbc.functions[i]->name};
        }
    }
    for (size_t i = 0; i < static_object_cnt; ++i) {
        if (!sym_map.emplace(unlinked_mbc.objects[i]->name, InstrInfo::IdentifierId::fromStaticObject(i)).second) {
            throw DuplicatedSymbolException{unlinked_mbc.objects[i]->name};
        }
    }
    for (auto& func: unlinked_mbc.functions) {
        for (const auto& [offset, symbol]: func->relocate) {
            auto itr = sym_map.find(symbol);
            if (itr == sym_map.end()) {
                throw CannotFoundSymbolException{symbol};
            }
            auto id = itr->second;
            func->code[offset + 1] = static_cast<uint8_t>(id);
            func->code[offset + 2] = static_cast<uint8_t>(id >> 8);
            func->code[offset + 3] = static_cast<uint8_t>(id >> 16);
        }
        func->relocate.clear();
    }
    // relocate of data will be done by am on loading bytecode
}

lib::Array<ValueBox> makeConstants(lib::Slice<const std::pair<const ts::Type*, uint64_t>> constants)
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

lib::Array<am::spd::Block> makeBlocks(std::vector<UnlinkedMBC::Block>& blocks)
{
    using am::spd::AutomaticObjectDescription;
    lib::Array<am::spd::Block> result(blocks.size());
    for (size_t i = 0; i < result.length(); ++i) {
        lib::Array<AutomaticObjectDescription> objects(blocks[i].automatic_objects.size());
        for (size_t j = 0; j < objects.length(); ++j) {
            auto& src = blocks[i].automatic_objects[j];
            std::unique_ptr<uint8_t[]> init_data{};
            if (!src.init_data.empty()) {
                init_data = std::unique_ptr<uint8_t[]>{new uint8_t[src.init_data.size()]};
                std::memcpy(init_data.get(), src.init_data.data(), src.init_data.size());
            }
            objects.init(j, std::move(src.name), src.id, *src.type, src.offset, std::move(init_data));
        }
        result.init(i, std::move(objects));
    }
    return result;
}

lib::Array<am::FullExprInfo> makeFullExprInfos(std::vector<UnlinkedMBC::FullExprInfo>& src)
{
    lib::Array<am::FullExprInfo> result(src.size());
    for (size_t i = 0; i < result.length(); ++i) {
        result.init(i, src[i].trace_event_cnt, lib::Array<uint8_t>::fromVector(src[i].sequence_after_graph),
                    lib::Array<std::pair<uint64_t, uint64_t>>::fromVector(src[i].source_location));
    }
    return result;
}

std::pair<lib::Array<Function>, lib::Array<uint8_t>> makeFunctions(UnlinkedMBC& unlinked_mbc)
{
    using am::spd::SourceCodeLocator;
    lib::Array<Function> functions(unlinked_mbc.functions.size());
    uint64_t addr = 0;
    for (size_t i = 0; i < functions.length(); ++i) {
        auto& src = *unlinked_mbc.functions[i];
        functions.init(i, std::move(src.name), *src.effective_type, addr, std::move(src.file_name), src.frame_size,
                       src.code.size(), src.max_object_num, makeBlocks(src.blocks),
                       makeFullExprInfos(src.full_expr_infos),
                       SourceCodeLocator{lib::Array<SourceCodeLocator::Item>::fromVector(src.func_locator.data)});
        addr += src.code.size();
    }
    lib::Array<uint8_t> code(addr);
    for (int i = 0; i < functions.length(); ++i) {
        std::memcpy(&code[functions[i].address], unlinked_mbc.functions[i]->code.data(), functions[i].code_size);
    }
    return {std::move(functions), std::move(code)};
}

auto makeObjects(UnlinkedMBC& unlinked_mbc, const ObjectLayout& object_layout)
-> std::tuple<lib::Array<StaticObjectDescription>, lib::Array<uint8_t>, std::vector<LinkedMBC::RelocateEntry>, uint64_t, uint64_t>
{
    std::vector<LinkedMBC::RelocateEntry> data_relocate;
    lib::Array<StaticObjectDescription> static_objects(object_layout.staticObjectCnt());
    uint64_t addr = 0;
    uint64_t idx = 0;
    uint64_t end = object_layout.data_begin;
    uint64_t string_literal_len, non_bss_data_len;
    enum {
        string_literal, data_, bss
    } state = string_literal;
    while (true) {
        for (; idx < end; ++idx) {
            auto& src = *unlinked_mbc.objects[idx];
            addr = lib::roundUp(addr, src.type->align());
            static_objects.init(idx, std::move(src.name), src.type, addr);
            if (!src.relocate_symbol.empty()) {
                data_relocate.emplace_back(addr, std::move(src.relocate_symbol));
            }
            addr += src.type->size();
        }
        if (state == string_literal) {
            string_literal_len = addr;
            end = object_layout.bss_begin;
            state = data_;
        } else if (state == data_) {
            non_bss_data_len = addr;
            end = object_layout.staticObjectCnt();
            state = bss;
        } else {
            break;
        }
    }
    lib::Array<uint8_t> data(non_bss_data_len);
    std::memset(data.data(), 0, non_bss_data_len);
    for (uint64_t i = 0; i < object_layout.bss_begin; ++i) {
        std::memcpy(&data[static_objects[i].address], unlinked_mbc.objects[i]->value.data(),
                    static_objects[i].type->size());
    }
    return {std::move(static_objects), std::move(data), std::move(data_relocate), string_literal_len,
            addr - non_bss_data_len};
}

std::unique_ptr<LinkedMBC> makeLinkedMBC(std::unique_ptr<UnlinkedMBC> unlinked_mbc, const LinkOption& option,
                                         const ObjectLayout& object_layout)
{
    auto types = lib::Array<const ts::Type*>::fromVector(unlinked_mbc->types);
    auto constants = makeConstants(unlinked_mbc->constants);
    auto [functions, code] = makeFunctions(*unlinked_mbc);
    auto [static_objects, data, data_relocate, string_literal_len, bss_size] = makeObjects(*unlinked_mbc, object_layout);
    unlinked_mbc->attribute.type = option.type;
    if (option.type == MBC::Type::executable) {
        auto itr = std::find_if(functions.begin(), functions.end(), [&](const Function& func) {
            return func.name == unlinked_mbc->attribute.module_or_entry_name;
        });
        ASSERT(itr != functions.end(), "entry name should be checked in `relocate`");
        unlinked_mbc->attribute.entry = &*itr;
    }
    return std::make_unique<LinkedMBC>(std::move(unlinked_mbc->source_name), std::move(unlinked_mbc->attribute),
                                       std::move(unlinked_mbc->comment), std::move(code), std::move(data),
                                       string_literal_len, bss_size, std::move(static_objects), std::move(constants),
                                       std::move(types), std::move(functions), std::move(data_relocate));
}
} // anonymous namespace

std::unique_ptr<MBC> Linker::link(std::vector<std::unique_ptr<UnlinkedMBC>> mbcs, const LinkOption& option)
{
    ASSERT(!mbcs.empty(), "cannot link empty files");
    auto unlinked_mbc = makeUnlinkedMBC({mbcs.data(), mbcs.size()}, option);
    if (option.type == MBC::Type::object_file) {
        return unlinked_mbc;
    }
    if (option.type == MBC::Type::executable) {
        insertBootFunction(*unlinked_mbc);
    }
    auto arrange_result = arrange(*unlinked_mbc);
    relocate(*unlinked_mbc, arrange_result.staticObjectCnt());
    return makeLinkedMBC(std::move(unlinked_mbc), option, arrange_result);
}
