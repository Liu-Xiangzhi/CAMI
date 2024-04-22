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

#include <object.h>
#include <ub.h>
#include <am.h>
#include <foundation/type/helper.h>
#include <lib/assert.h>
#include <lib/array.h>
#include <formatter.h>

using namespace cami;
using namespace am;
using namespace ts;
using lib::ToString;

std::string Formatter::formatType(const ts::Type& type) // NOLINT
{
    switch (type.kind()) {
    case Kind::bool_:
        return "bool"s;
    case Kind::char_:
        return "char"s;
    case Kind::i8:
        return "i8"s;
    case Kind::i16:
        return "i16"s;
    case Kind::i32:
        return "i32"s;
    case Kind::i64:
        return "i64"s;
    case Kind::u8:
        return "u8"s;
    case Kind::u16:
        return "u16"s;
    case Kind::u32:
        return "u32"s;
    case Kind::u64:
        return "u64"s;
    case Kind::f32:
        return "f32"s;
    case Kind::f64:
        return "f64"s;
    case Kind::void_:
        return "void"s;
    case Kind::null:
        return "null"s;
    case Kind::pointer:
        return Formatter::formatType(down_cast<const Pointer&>(type).referenced).append("*");
    case Kind::dissociative_pointer:
        return "dissociative pointer"s;
    case Kind::array: {
        auto& array = down_cast<const Array&>(type);
        return lib::format("${}[${}]", array.element, array.len);
    }
    case Kind::struct_:
        return "struct "s + down_cast<const Struct&>(type).name;
    case Kind::union_:
        return "union "s + down_cast<const Struct&>(type).name;
    case Kind::function: {
        auto& func = down_cast<const Function&>(type);
        std::string result{"("s};
        if (func.params.empty()) {
            result.append("()"s);
        } else if (func.params.length() == 1) {
            result.append(Formatter::formatType(*func.params[0]));
        } else {
            result.append("("s);
            for (auto item: func.params) {
                result.append(Formatter::formatType(*item)).append(", "s);
            }
            result.pop_back();
            result.pop_back();
            result.append(")"s);
        }
        result.append(" -> "s).append(Formatter::formatType(func.returned)).append(")"s);
        return result;
    }
    case Kind::qualify: {
        auto& qualify = down_cast<const Qualify&>(type);
        auto result = Formatter::formatType(qualify.qualified);
        if (qualify.qualifier & Qualifier::atomic) {
            result.append(" atomic"s);
        }
        if (qualify.qualifier & Qualifier::restrict) {
            result.append(" restrict"s);
        }
        if (qualify.qualifier & Qualifier::const_) {
            result.append(" const"s);
        }
        if (qualify.qualifier & Qualifier::volatile_) {
            result.append(" volatile"s);
        }
        return result;
    }
    case Kind::ivd:
        return "<invalid>"s;
    default:
        return "?type?"s;
    }
}

std::string Formatter::opcode(Opcode opcode)
{
    static const std::map<Opcode, std::string_view> map{
            {Opcode::dsg,   "dsg"sv},
            {Opcode::drf,   "drf"sv},
            {Opcode::read,  "read"sv},
            {Opcode::mdf,   "mdf"sv},
            {Opcode::zero,  "zero"sv},
            {Opcode::mdfi,  "mdfi"sv},
            {Opcode::zeroi, "zeroi"sv},
            {Opcode::eb,    "eb"sv},
            {Opcode::lb,    "lb"sv},
            {Opcode::new_,  "new"sv},
            {Opcode::del,   "del"sv},
            {Opcode::fe,    "fe"sv},
            {Opcode::j,     "j"sv},
            {Opcode::jst,   "jst"sv},
            {Opcode::jnt,   "jnt"sv},
            {Opcode::call,  "call"sv},
            {Opcode::ij,    "ij"sv},
            {Opcode::ret,   "ret"sv},
            {Opcode::nop,   "nop"sv},
            {Opcode::pushu, "pushu"sv},
            {Opcode::push,  "push"sv},
            {Opcode::pop,   "pop"sv},
            {Opcode::dup,   "dup"sv},
            {Opcode::halt,  "halt"sv},
            {Opcode::dot,   "dot"sv},
            {Opcode::arrow, "arrow"sv},
            {Opcode::addr,  "addr"sv},
            {Opcode::cast,  "cast"sv},
            {Opcode::cpl,   "cpl"sv},
            {Opcode::pos,   "pos"sv},
            {Opcode::neg,   "neg"sv},
            {Opcode::not_,  "not"sv},
            {Opcode::mul,   "mul"sv},
            {Opcode::div,   "div"sv},
            {Opcode::mod,   "mod"sv},
            {Opcode::add,   "add"sv},
            {Opcode::sub,   "sub"sv},
            {Opcode::ls,    "ls"sv},
            {Opcode::rs,    "rs"sv},
            {Opcode::sl,    "sl"sv},
            {Opcode::sle,   "sle"sv},
            {Opcode::sg,    "sg"sv},
            {Opcode::sge,   "sge"sv},
            {Opcode::seq,   "seq"sv},
            {Opcode::sne,   "sne"sv},
            {Opcode::and_,  "and"sv},
            {Opcode::or_,   "or"sv},
            {Opcode::xor_,  "xor"sv},
    };
    if (auto itr = map.find(opcode); itr != map.end()) {
        return std::string{itr->second};
    }
    return lib::format("?${}?", static_cast<uint64_t>(opcode));
}

std::string Formatter::type(const ts::Type& type)
{
    auto result = Formatter::formatType(type);
    if (result.back() == ')') {
        // remove duplicated parenthesis
        return result.substr(1, result.length() - 2);
    }
    return result;
}

std::string Formatter::integerValue(const IntegerValue& value, std::string_view specifier)
{
    if (isSigned(value.getType().kind())) {
        auto tmp = value.uint64();
        auto val = *reinterpret_cast<int64_t*>(&tmp);
        return lib::format("integer{${}}", val);
    }
    return lib::DefaultToString<uint64_t>::invoke(value.uint64(), specifier);
}

std::string Formatter::f32Value(const F32Value& value)
{
    return lib::format("float{${}}", value.f32());
}

std::string Formatter::f64Value(const F64Value& value)
{
    return lib::format("double{${}}", value.f64());
}

std::string Formatter::pointerValue(const PointerValue& value)
{
    return lib::format("${}{ref: ${brief}, offset: ${}}", value.getType(), **value.getReferenced(), value.getOffset());
}

std::string Formatter::structOrUnionValue(const StructOrUnionValue& value)
{
    return lib::format("${}{object: ${brief}}", value.getType(), *value.obj);
}

std::string Formatter::nullValue()
{
    return "nullptr_t{nullptr}"s;
}

std::string Formatter::undefinedValue()
{
    return "undefined_value"s;
}

std::string Formatter::dissociativePointerValue(const DissociativePointerValue& value)
{
    return lib::format("dissociative_pointer_value{type: ${}, address: ${x}}", *value.pointer_type, value.address);
}

std::string Formatter::valueBox(const ValueBox& vb, std::string_view specifier)
{
    switch (vb->getType().kind()) {
    case Kind::f32:
        return Formatter::f32Value(vb.get<F32Value>());
    case Kind::f64:
        return Formatter::f64Value(vb.get<F64Value>());
    case Kind::pointer:
        return Formatter::pointerValue(vb.get<PointerValue>());
    case Kind::dissociative_pointer:
        return Formatter::dissociativePointerValue(vb.get<DissociativePointerValue>());
    case Kind::struct_:
    case Kind::union_:
        return Formatter::structOrUnionValue(vb.get<StructOrUnionValue>());
    case Kind::null:
        return Formatter::nullValue();
    case Kind::ivd:
        return Formatter::undefinedValue();
    default:
        ASSERT(isInteger(vb->getType().kind()), "No other type should occur");
        return Formatter::integerValue(vb.get<IntegerValue>(), specifier);
    }
}

std::string Formatter::richValue(const OperandStack::RichValue& rv, std::string_view specifier)
{
    auto directly_read_from = !rv.attr.directly_read_from ? "<None>" :
                              Formatter::objectName(**rv.attr.directly_read_from);
    auto vb = rv.attr.indeterminate ? "<indeterminate value>" : Formatter::valueBox(rv.vb, specifier);
    return lib::format("rich_value{attribute: {indeterminate: ${}, directly_read_from: ${}}, ${}}",
                       rv.attr.indeterminate, directly_read_from, vb);
}

std::string Formatter::objectStatus(Object::Status status)
{
    switch (status) {
    case Object::Status::well:
        return "well"s;
    case Object::Status::destroyed:
        return "destroyed"s;
    case Object::Status::indeterminate:
        return "indeterminate_representation"s;
    case Object::Status::non_value_representation:
        return "non_value_representation"s;
    case Object::Status::uninitialized:
        return "uninitialized"s;
    }
    return "?status?"s;
}

std::string Formatter::object(const Object& obj, bool full) // NOLINT
{
    ASSERT(this->am != nullptr || !full, "Fully format object need info of abstract machine");
    // brief: name + address + type + status
    // standard: brief + sub-obj
    // full: standard + ref + ref_by + age + tags
    //      name of sub-obj will be ignored
    //
    auto idt = std::string(this->indent + 1, '\t');
    std::string result;
    result.append(this->indent, '\t').append("object{\n")
            .append(lib::format("${}name: ${x}\n", idt, Formatter::objectName(obj)));
    this->formatObjectStandardInfo(result, obj);
    if (full) {
        this->formatObjectDetailInfo(result, obj);
    }
    result.append(this->indent, '\t').append("}");
    return result;
}

std::string Formatter::formatSubObject(const Object& obj) // NOLINT
{
    this->indent += 2;
    auto idt = std::string(this->indent + 1, '\t');
    std::string result;
    result.append(this->indent, '\t').append("{\n");
    this->formatObjectStandardInfo(result, obj);
    if (this->am != nullptr) {
        this->formatObjectDetailInfo(result, obj);
    }
    result.append(this->indent, '\t').append("}");
    this->indent -= 2;
    return result;
}

std::string Formatter::objectName(const Object& obj)
{
    return obj.super_object ? lib::format("sub object of ${}", obj.top().name) : obj.name;
}

std::string Formatter::briefObject(const Object& obj)
{
    return lib::format("object{name: ${}, address: ${x}, type: ${}, status: ${}}",
                       Formatter::objectName(obj), obj.address, obj.effective_type, obj.status);
}

void Formatter::formatObjectStandardInfo(std::string& result, const Object& obj) // NOLINT
{
    auto idt = std::string(this->indent + 1, '\t');
    result.append(lib::format("${}address: ${x}\n", idt, obj.address))
            .append(lib::format("${}type: ${}\n", idt, obj.effective_type))
            .append(lib::format("${}status: ${}\n", idt, obj.status))
            .append(lib::format("${}sub objects: [", idt));
    for (Object* item: obj.sub_objects) {
        result.append("\n").append(this->formatSubObject(*item));
    }
    result.append(lib::format("\n${}]\n", idt));
}

void Formatter::formatObjectDetailInfo(std::string& result, const Object& obj) // NOLINT
{
    auto idt = std::string(this->indent + 1, '\t');
    result.append(lib::format("${}age: ${}\n", idt, obj.age));
    result.append(idt).append("reference: ");
    if (auto ref_obj = this->am->object_manager.getReferencedObject(&obj);ref_obj) {
        result.append(Formatter::briefObject(**ref_obj));
    } else {
        result.append("<None>");
    }
    result.append(lib::format("\n${}referenced by: [", idt));
    for (Object* item: obj.referenced_by) {
        result.append("\n").append(this->formatSubObject(*item));
    }
    result.append(lib::format("\n${}]\n", idt));
    result.append(idt).append("tags: [");
    for (const auto& item: obj.tags) {
        result.append("\n").append(this->tag(item));
    }
    result.append(lib::format("\n${}]\n", idt));
}

std::string Formatter::tag(const Object::Tag& tag) const
{
    ASSERT(this->am != nullptr, "format tag need am info");
    auto idt = std::string(this->indent, '\t');
    std::string result;
    auto& functions = this->am->static_info.functions;
    auto ctx = &tag.context;
    auto ctx_func = &functions[ctx->func_id];
    auto [ln, cl] = ctx_func->full_expr_infos[tag.access_point.full_expr_id].source_location[tag.access_point.inner_id.value()];
    result.append(lib::format("${}accessed at source file '${}' line ${} colum ${}. backtrace:\n",
                              idt, ctx_func->file_name, ln, cl));
    while (!ctx->caller.isDummy()) {
        ctx_func = &functions[ctx->func_id];
        auto caller_func = &functions[ctx->caller.func_id];
        auto [line, colum] = caller_func->full_expr_infos[ctx->call_point.full_expr_id].source_location[ctx->call_point.inner_id.value()];
        result.append(lib::format("${}\t'${}' called at \"${}\" line ${} colum ${}.\n",
                                  idt, ctx_func->name, ctx_func->file_name, line, colum));
        ctx = &ctx->caller;
    }
    ctx_func = &functions[ctx->func_id];
    result.append(lib::format("${}\t'${}' at \"${}\".", idt, ctx_func->name, ctx_func->file_name));
    return result;
}

std::string Formatter::operandStack(const OperandStack& operand_stack)
{
    std::string result{"operand_stack["};
    auto& stack = operand_stack.getStack();
    for (auto itr = stack.rbegin(); itr != stack.rend(); ++itr) {
        result.append(Formatter::richValue(*itr, ""sv)).append(",");
    }
    if (!stack.empty()) {
        result.pop_back();
        result.pop_back();
    }
    result.append("]");
    return result;
}

std::string Formatter::simpleAllocator(const SimpleAllocator& allocator)
{
    std::string result{"heap_usage{"};
    for (auto addr = layout::HEAP_BASE; addr < layout::HEAP_BOUNDARY;) {
        auto len_with_mark = allocator.getMemory().read64(addr);
        const bool used = len_with_mark & 1;
        const auto len = len_with_mark - 1;
        result.append(lib::format("(${x}, ${x}, ${}), ", addr, addr + len, used ? "used"sv : "free"sv));
        addr += len;
    }
    result.pop_back();
    result.pop_back();
    result.append("}");
    return result;
}

std::string Formatter::objectManager(const ObjectManager& om)
{
    auto& ed = om.getEden();
    auto& su = om.getSurvivor();
    auto& og = om.getOldGeneration();
    auto& pm = om.getPermanent();
    return lib::format("object_manager{eden{size:${}, used:${}}, survivor{size:${}, used:${}}, "
                       "old_generation{size:${}, used:${}}, permanent_size:${}, used_percent:${}}",
                       ed.max_size, ed.usage, su.max_size, su.usage, og.max_size, og.usage, pm.max_size,
                       static_cast<float>(ed.usage + su.usage + og.usage) /
                       static_cast<float>(ed.max_size + su.max_size + og.max_size));
}

std::string Formatter::staticFuncInfo(const spd::Function& func)
{
    return lib::format("function{name: ${}, address: ${x}, type: ${}, file: ${}}",
                       func.name, func.address, func.effective_type, func.file_name);
}

/* *********************************************************************************** */

std::string ToString<Opcode>::invoke(const Opcode& opcode, [[maybe_unused]] std::string_view specifier)
{
    return Formatter::opcode(opcode);
}

std::string ToString<UB>::invoke(const UB& ub, [[maybe_unused]] std::string_view specifier)
{
    auto no = static_cast<int>(ub);
    return lib::format("(${}) ${}", static_cast<int>(no), cami::am::ub_descriptions[no - 1]);
}

std::string ToString<ts::Type>::invoke(const ts::Type& type, [[maybe_unused]] std::string_view specifier)
{
    return Formatter::type(type);
}

std::string ToString<IntegerValue>::invoke(const IntegerValue& arg, std::string_view specifier)
{
    return Formatter::integerValue(arg, specifier);
}

std::string ToString<F32Value>::invoke(const F32Value& arg, [[maybe_unused]] std::string_view specifier)
{
    return Formatter::f32Value(arg);
}

std::string ToString<F64Value>::invoke(const F64Value& arg, [[maybe_unused]] std::string_view specifier)
{
    return Formatter::f64Value(arg);
}

std::string ToString<PointerValue>::invoke(const PointerValue& arg, [[maybe_unused]] std::string_view specifier)
{
    return Formatter::pointerValue(arg);
}

std::string ToString<DissociativePointerValue>::invoke(const DissociativePointerValue& value, [[maybe_unused]] std::string_view specifier)
{
    return Formatter::dissociativePointerValue(value);
}

std::string ToString<StructOrUnionValue>::invoke(const StructOrUnionValue& arg,
                                                 [[maybe_unused]] std::string_view specifier)
{
    return Formatter::structOrUnionValue(arg);
}

std::string ToString<NullValue>::invoke(const NullValue&, [[maybe_unused]] std::string_view specifier)
{
    return Formatter::nullValue();
}

std::string ToString<UndefinedValue>::invoke(const UndefinedValue&, [[maybe_unused]] std::string_view specifier)
{
    return Formatter::undefinedValue();
}

std::string ToString<ValueBox>::invoke(const ValueBox& vb, std::string_view specifier)
{
    return Formatter::valueBox(vb, specifier);
}

std::string ToString<OperandStack::RichValue>::invoke(const OperandStack::RichValue& rv, std::string_view specifier)
{
    return Formatter::richValue(rv, specifier);
}

std::string ToString<OperandStack>::invoke(const OperandStack& operand_stack,
                                           [[maybe_unused]] std::string_view specifier)
{
    return Formatter::operandStack(operand_stack);
}

std::string ToString<SimpleAllocator>::invoke(const SimpleAllocator& allocator,
                                              [[maybe_unused]] std::string_view specifier)
{
    return Formatter::simpleAllocator(allocator);
}

std::string ToString<ObjectManager>::invoke(const ObjectManager& object_manager,
                                            [[maybe_unused]] std::string_view specifier)
{
    return Formatter::objectManager(object_manager);
}

std::string ToString<Object::Status>::invoke(const Object::Status& status, [[maybe_unused]] std::string_view specifier)
{
    return Formatter::objectStatus(status);
}

std::string ToString<Entity>::invoke(const Entity& ent, std::string_view specifier)
{
    if (ent.effective_type.kind() != ts::Kind::function) {
        return ToString<Object>::invoke(down_cast<const Object&>(ent), specifier);
    }
    if (specifier == "name"sv) {
        return ent.name;
    }
    auto& func = down_cast<const spd::Function&>(ent);
    return Formatter::staticFuncInfo(func);
}

std::string ToString<Object>::invoke(const Object& obj, std::string_view specifier)
{
    if (specifier == "name"sv) {
        return Formatter::objectName(obj);
    } else if (specifier == "brief"sv) {
        return Formatter::briefObject(obj);
    } else if (specifier.empty() || specifier == "standard"sv) {
        return Formatter{0, nullptr}.object(obj, false);
    } else {
        ASSERT(specifier == "full"sv, "invalid specifier of object");
        return Formatter{0, nullptr}.object(obj, true);
    }
}
