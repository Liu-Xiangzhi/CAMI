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

#include <execute.h>
#include <lib/compiler_guarantee.h>
#include <foundation/type/helper.h>
#include <foundation/type/mm.h>
#include <foundation/value.h>

using namespace cami;
using namespace ts;
using am::Execute;

#define CHECK_ID(id_type, id, size) COMPILER_GUARANTEE(id < size, lib::format("Value(${}) of " #id_type " id out of boundary(${})", id, size))
#define CHECK_DESIGNATION_REGISTER() COMPILER_GUARANTEE(am.dsg_reg.entity != nullptr && am.dsg_reg.lvalue_type != nullptr, "entity or lvalue_type of designation register is null")
#define CHECK_TYPE(expr) COMPILER_GUARANTEE(expr, "type constraint violation")

void Execute::designate(AbstractMachine& am, InstrInfo info)
{
    auto id = info.getIdentifierID();
    if (id.isFuntion()) {
        CHECK_ID(function, id.value(), am.static_info.functions.length());
        am.dsg_reg.entity = &am.static_info.functions[id.value()];
    } else {
        auto& objects = id.isGlobal() ? am.static_info.static_objects
                                      : am.state.current_function().automatic_objects;
        CHECK_ID(object, id.value(), objects.length());
        am.dsg_reg.entity = objects[id.value()];
    }
    COMPILER_GUARANTEE(am.dsg_reg.entity != nullptr, "entity of designation register is null");
    am.dsg_reg.lvalue_type = &am.dsg_reg.entity->effective_type;
    am.dsg_reg.offset = 0;
}

void Execute::dereference(AbstractMachine& am)
{
    auto pointer = am.operand_stack.popDeterminateValue();
    if (pointer->getType().kind() == Kind::dissociative_pointer) {
        throw UBException{{UB::eva_ivd_lvalue}, lib::format(
                "dereference dissociative pointer, value: `${x}`", pointer.get<DissociativePointerValue>().address)};
    }
    CHECK_TYPE(pointer->getType().kind() == Kind::pointer);
    auto ent = pointer.get<PointerValue>().getReferenced();
    if (!ent) {
        throw UBException{{UB::deref_ivd_ptr}, "dereference nullptr"};
    }
    am.dsg_reg.entity = *ent;
    am.dsg_reg.lvalue_type = &down_cast<const Pointer&>(pointer->getType()).referenced;
    am.dsg_reg.offset = pointer.get<PointerValue>().getOffset();
    if (am.dsg_reg.entity->effective_type.kind() != Kind::function) {
        auto& obj = down_cast<Object&>(*am.dsg_reg.entity);
        if (pointer.get<PointerValue>().getOffset() >= obj.effective_type.size()) {
            throw UBException{{UB::deref_ending_ptr}, lib::format(
                    "dereference pointer which points just past the last element of certain array\n${}", pointer)};
        }
        if (obj.status == Object::Status::destroyed) {
            throw UBException{{UB::refer_del_obj, UB::use_ptr_value_which_ref_del_obj}, lib::format(
                    "object `${name}` which is referenced by pointer dereference is deleted\n${}", obj, pointer)};
        }
    }
}

void Execute::read(AbstractMachine& am, InstrInfo info)
{
    CHECK_DESIGNATION_REGISTER();
    auto& lvalue_type = *am.dsg_reg.lvalue_type;
    if (!isAllowed(lvalue_type, am.dsg_reg.entity->effective_type)) {
        throw UBException{{UB::incompatible_read}, lib::format(
                "entity '${}' is read by incompatible type `${}`", *am.dsg_reg.entity, lvalue_type)};
    }
    if (lvalue_type.kind() == Kind::function) {
        // lvalue conversion
        auto ptr = new PointerValue{&type_manager.getPointer(lvalue_type), am.dsg_reg.entity, 0};
        am.operand_stack.push(ValueBox{ptr});
        return;
    }
    auto& obj = down_cast<Object&>(*am.dsg_reg.entity);
    if (am.dsg_reg.offset > 0 && !isCCharacter(lvalue_type.kind())) {
        throw ConstraintViolationException{lib::format("entity '${}' is read with nonzero offset", *am.dsg_reg.entity)};
    }
    if (!checkStatusForRead(obj)) {
        throw UBException{{UB::read_ir_obj, UB::read_nvr, UB::read_before_init, UB::eva_ivd_lvalue}, lib::format(
                "Object `${name}` is read, but it's not in a well status\n${}", obj, obj)};
    }
    if (obj.effective_type.kind() == Kind::qualify) {
        auto qualifier = down_cast<const Qualify&>(obj.effective_type).qualifier;
        if (qualifier & Qualifier::volatile_) {
            if (lvalue_type.kind() != Kind::qualify
                || !(down_cast<const Qualify&>(lvalue_type).qualifier & Qualifier::volatile_)) {
                throw UBException{{UB::ivd_read_volatile_obj}, lib::format(
                        "volatile object '${name}' is read by an lvalue of non-volatile type ${}", obj, lvalue_type)};
            }
        }
    }
    if (lvalue_type.kind() == Kind::array) {
        // lvalue conversion
        const auto [qualifier, _] = peelQualify(*am.dsg_reg.lvalue_type);
        ASSERT(obj.sub_objects.length() > 0, "array object must have at least one element");
        auto& elem_t = addQualify(down_cast<const Array&>(lvalue_type).element, qualifier);
        am.operand_stack.push(ValueBox{new PointerValue{&type_manager.getPointer(elem_t), obj.sub_objects[0], 0}});
        return;
    }
    Execute::attachTag(am, obj, InnerID::newCoexisting(info.getInnerID()));
    Execute::do_read(am);
}

void Execute::modify(AbstractMachine& am, InstrInfo info)
{
    Execute::modifyCheck(am, false);
    auto& obj = down_cast<Object&>(*am.dsg_reg.entity);
    Execute::attachTag(am, obj, InnerID::newMutualExclude(info.getInnerID()));
    Execute::do_modify(am, am.operand_stack.popDeterminateValue());
    updateCommonInitialSequenceStatus(obj);
}

void Execute::zero(AbstractMachine& am, InstrInfo info)
{
    Execute::basicModifyCheck(am, false);
    auto& obj = down_cast<Object&>(*am.dsg_reg.entity);
    am.memory.zeroize(obj.address, obj.effective_type.size());
    if (auto ref = am.object_manager.getReferencedObject(&obj); ref) {
        [[maybe_unused]] auto cnt = (*ref)->referenced_by.erase(&obj);
        ASSERT(cnt == 1, "referenced object do not contains referencing object's reference");
    }
    Execute::attachTag(am, obj, InnerID::newMutualExclude(info.getInnerID()));
    applyRecursively(obj, [](Object& o) { o.status = Object::Status::well; });
}

void Execute::writeInit(AbstractMachine& am)
{
    Execute::modifyCheck(am, true);
    COMPILER_GUARANTEE(down_cast<Object*>(am.dsg_reg.entity)->status == Object::Status::uninitialized,
                       lib::format("object `${}` is double initialized", down_cast<Object&>(*am.dsg_reg.entity).name));
    Execute::do_modify(am, am.operand_stack.popDeterminateValue());
    updateCommonInitialSequenceStatus(down_cast<Object&>(*am.dsg_reg.entity));
}

void Execute::zeroInit(AbstractMachine& am)
{
    Execute::basicModifyCheck(am, true);
    auto& obj = down_cast<Object&>(*am.dsg_reg.entity);
    COMPILER_GUARANTEE(obj.status == Object::Status::uninitialized, lib::format("object `${}` is double initialized", obj.name));
    am.memory.zeroize(obj.address, obj.effective_type.size());
    if (auto ref = am.object_manager.getReferencedObject(&obj); ref) {
        [[maybe_unused]] auto cnt = (*ref)->referenced_by.erase(&obj);
        ASSERT(cnt == 1, "referenced object do not contains referencing object's reference");
    }
    applyRecursively(obj, [](Object& o) { o.status = Object::Status::well; });
}

void Execute::do_read(am::AbstractMachine& am)
{
    auto& obj = down_cast<Object&>(*am.dsg_reg.entity);
    const auto [qualifier, _lvalue_type] = peelQualify(*am.dsg_reg.lvalue_type);
    // (For lambda)Captured structured bindings are a C++20 extension
    auto& lvalue_type = _lvalue_type;
    auto value = [&]() -> Value* {
        switch (lvalue_type.kind()) {
        case Kind::f32: {
            auto tmp = am.memory.read32(obj.address);
            float val;
            std::memcpy(&val, &tmp, 4);
            return new F32Value{val};
        }
        case Kind::f64: {
            auto tmp = am.memory.read64(obj.address);
            double val;
            std::memcpy(&val, &tmp, 8);
            return new F64Value{val};
        }
        case Kind::pointer: {
            auto addr = am.memory.read64(obj.address);
            if (am.isValidEntityAddress(addr)) {
                auto ptr = reinterpret_cast<Entity*>(addr);
                auto offset = am.memory.read64(obj.address + 8);
                return new PointerValue{&lvalue_type, ptr, offset};
            }
            return new DissociativePointerValue{&lvalue_type, addr};
        }
        case Kind::struct_:
        case Kind::union_:
            return new StructOrUnionValue{&lvalue_type, &obj};
        default:
            ASSERT(isInteger(lvalue_type.kind()), "no other type kind could occur");
            uint64_t val = 0;
            am.memory.read(reinterpret_cast<uint8_t*>(&val), obj.address + am.dsg_reg.offset, lvalue_type.size());
#ifdef CAMI_TARGET_INFO_BIG_ENDIAN
            val >>= 64 - 8 * lvalue_type.size();
#endif
            return new IntegerValue{&lvalue_type, val};
        }
    }();
    am.operand_stack.push({ValueBox{value}, {.directly_read_from = &obj, .indeterminate = false}});
}

void Execute::do_modify(am::AbstractMachine& am, ValueBox vb) // NOLINT
{
    auto& obj = down_cast<Object&>(*am.dsg_reg.entity);
    auto& write_value_type = removeQualify(vb->getType());
    switch (write_value_type.kind()) {
    case Kind::f32: {
        auto tmp = vb.get<F32Value>().f32();
        uint32_t val;
        std::memcpy(&val, &tmp, 4);
        am.memory.write32(obj.address, val);
        obj.status = Object::Status::well;
    }
        break;
    case Kind::f64: {
        auto tmp = vb.get<F64Value>().f64();
        uint64_t val;
        std::memcpy(&val, &tmp, 8);
        am.memory.write64(obj.address, val);
        obj.status = Object::Status::well;
    }
        break;
    case Kind::pointer: {
        if (auto ref = am.object_manager.getReferencedObject(&obj); ref) {
            [[maybe_unused]] auto cnt = (*ref)->referenced_by.erase(&obj);
            ASSERT(cnt == 1, "referenced object do not contains referencing object's reference");
        }
        uint64_t ptr;
        if (auto ref = vb.get<PointerValue>().getReferenced(); ref) {
            if ((*ref)->effective_type.kind() != Kind::function) {
                down_cast<Object&>(**ref).referenced_by.insert(&obj);
            }
            ptr = reinterpret_cast<uint64_t>(*ref);
        } else {
            ptr = 0;
        }
        auto offset = vb.get<PointerValue>().getOffset();
        am.memory.write64(obj.address, ptr);
        am.memory.write64(obj.address + 8, offset);
        obj.status = Object::Status::well;
    }
        break;
    case Kind::dissociative_pointer:
        am.memory.write64(obj.address, vb.get<DissociativePointerValue>().address);
        am.memory.write64(obj.address + 8, 0);
        obj.status = Object::Status::well;
        break;
    case Kind::struct_:
    case Kind::union_: {
        auto& struct_or_union_obj = *vb.get<StructOrUnionValue>().obj;
        auto size = obj.effective_type.size();
        std::unique_ptr<uint8_t[]> buf{new uint8_t[size]};
        am.memory.read(buf.get(), struct_or_union_obj.address, obj.effective_type.size());
        am.memory.write(obj.address, buf.get(), obj.effective_type.size());
        copyStatus(struct_or_union_obj, obj);
    }
        break;
    default:
        ASSERT(isInteger(write_value_type.kind()), "no other type kind could occur");
        auto val = vb.get<IntegerValue>().uint64();
#ifdef CAMI_TARGET_INFO_LITTLE_ENDIAN
        am.memory.write(obj.address + am.dsg_reg.offset, reinterpret_cast<uint8_t*>(&val), write_value_type.size());
#else
        am.memory.write(obj.address + am.dsg_reg.offset, reinterpret_cast<uint8_t*>(&val) + (8 - write_value_type.size()),
                        write_value_type.size());
#endif
        obj.status = Object::Status::well;
        if (isCCharacter(write_value_type.kind())) {
            Execute::checkObjectRepresentation(am, obj);
        }
    }
}

void Execute::checkObjectRepresentation(AbstractMachine& am, Object& obj)
{
    // no non_value_representation is defined now... so do nothing
}

void Execute::enterBlock(AbstractMachine& am, InstrInfo info)
{
    do_enterBlock(am, info.getBlockID());
}

void Execute::do_enterBlock(AbstractMachine& am, uint32_t block_id)
{
    auto& current_func = am.state.current_function();
    auto* static_info = current_func.static_info;
    current_func.blocks.push(block_id);
    CHECK_ID(block, block_id, static_info->blocks.length());
    for (const auto& item: static_info->blocks[block_id].obj_desc) {
        CHECK_ID(object, item.id, current_func.automatic_objects.length());
        auto obj = am.object_manager.new_(item.name, item.type, am.state.frame_pointer + item.offset);
        if (item.init_data) {
            applyRecursively(*obj, [](auto& o) { o.status = Object::Status::well; });
            am.memory.write(am.state.frame_pointer + item.offset, item.init_data.get(), item.type.size());
        }
        current_func.automatic_objects[item.id] = obj;
    }
}

void Execute::leaveBlock(AbstractMachine& am)
{
    auto& current_func = am.state.current_function();
    auto* static_info = current_func.static_info;
    COMPILER_GUARANTEE(!current_func.blocks.empty(),
                       "Instruction `lb` is executed while there's no block in current function");
    auto block_id = current_func.blocks.top();
    current_func.blocks.pop();
    // leave block is implicitly treated as a full expression, because it destroys automatic object(s),
    //      which may indeterminate pointer object(s)
    current_func.full_expr_exec_cnt++;
    for (const auto& item: static_info->blocks[block_id].obj_desc) {
        CHECK_ID(object, item.id, current_func.automatic_objects.length());
        auto obj = current_func.automatic_objects[item.id];
        am.object_manager.cleanup(obj, InnerID::newMutualExclude(0));
        current_func.automatic_objects[item.id] = nullptr;
    }
}

void Execute::newObject(AbstractMachine& am, InstrInfo info)
{
    static uint64_t cnt = 0;
    auto id = info.getTypeID();
    CHECK_ID(type, id, am.static_info.types.length());
    auto type = am.static_info.types[id];
    auto val = am.operand_stack.popDeterminateValue();
    CHECK_TYPE(isInteger(val->getType().kind()));
    auto num = val.get<IntegerValue>().uint64();
    if (num == 0) {
        am.operand_stack.push(ValueBox{new PointerValue{type, nullptr, 0}});
        return;
    }
    auto obj = am.object_manager.new_("<heap>#"s + std::to_string(cnt++), type_manager.getArray(*type, num),
                                      am.heap_allocator->alloc(type->size() * num, type->align()));
    am.operand_stack.push(ValueBox{new PointerValue{&type_manager.getPointer(*type), obj->sub_objects[0], 0}});
}

void Execute::deleteObject(AbstractMachine& am, InstrInfo info)
{
    auto pointer = am.operand_stack.popDeterminateValue();
    CHECK_TYPE(pointer->getType().kind() == Kind::pointer);
    auto ent = pointer.get<PointerValue>().getReferenced();
    if (!ent) {
        throw ConstraintViolationException{"delete nullptr"};
    }
    if ((*ent)->effective_type.kind() == Kind::function) {
        throw ConstraintViolationException{lib::format("delete function `${name}`", **ent)};
    }
    auto& obj = down_cast<Object&>(**ent).topOfSameAddress();
    if (obj.super_object) {
        throw ConstraintViolationException{lib::format("delete non-top object `${name}`\n${}", **ent, **ent)};
    }
    if (obj.status == Object::Status::destroyed) {
        throw UBException{{UB::use_ptr_value_which_ref_del_obj}, lib::format("Object `${name}` is double free", obj)};
    }
    if (!VirtualMemory::inHeapSegment(obj.address)) {
        throw ConstraintViolationException{
                lib::format("delete non-allocated storage object `${name}`\n${}", **ent, **ent)};
    }
    auto inner_id = InnerID::newMutualExclude(info.getInnerID());
    Execute::attachTag(am, obj, inner_id);
    am.object_manager.cleanup(&obj, inner_id);
    am.heap_allocator->dealloc(obj.address, obj.effective_type.size());
}

void Execute::fullExpression(AbstractMachine& am, InstrInfo info)
{
    auto& cur_func = am.state.current_function();
    cur_func.cur_full_expr_id = info.getFullExprID();
    cur_func.full_expr_exec_cnt++;
}

void Execute::jump(AbstractMachine& am, InstrInfo info)
{
    auto target_pc = am.state.pc + info.getOffset();
    checkJumpAddr(am, target_pc);
    am.state.pc = target_pc;
}

void Execute::jumpIfSet(AbstractMachine& am, InstrInfo info)
{
    auto flag = am.operand_stack.popDeterminateValue();
    CHECK_TYPE(isScalar(flag->getType().kind()));
    if (!flag.isZero()) {
        jump(am, info);
    }
}

void Execute::jumpIfNotSet(AbstractMachine& am, InstrInfo info)
{
    auto flag = am.operand_stack.popDeterminateValue();
    CHECK_TYPE(isScalar(flag->getType().kind()));
    if (flag.isZero()) {
        jump(am, info);
    }
}

void Execute::call(AbstractMachine& am, InstrInfo info)
{
    auto func_ptr = am.operand_stack.popDeterminateValue();
    if (func_ptr->getType().kind() == Kind::dissociative_pointer) {
        throw UBException{{UB::incompatible_func_call, UB::eva_ivd_lvalue}, lib::format(
                "try to call dissociative pointer, value: ${}", func_ptr.get<DissociativePointerValue>().address)};
    }
    CHECK_TYPE(func_ptr->getType().kind() == Kind::pointer);
    auto entity = func_ptr.get<PointerValue>().getReferenced();
    if (!entity) {
        throw ConstraintViolationException{"call nullptr"};
    }
    auto& ref_type = down_cast<const Pointer&>(func_ptr->getType()).referenced;
    CHECK_TYPE(ref_type.kind() == Kind::function);
    if (!isCompatible(ref_type, (*entity)->effective_type)) {
        throw UBException{{UB::incompatible_func_call}, lib::format(
                "Entity `${name}`(with type `${}`) is called by incompatible pointer type `${}`",
                **entity, (*entity)->effective_type, func_ptr->getType())};
    }
    auto& func = down_cast<spd::Function&>(**entity);
    am.state.frame_pointer -= func.frame_size;
    am.memory.notifyStackPointer(am.state.frame_pointer);
    auto& cur_func = am.state.current_function();
    auto context = new TraceContext{
            cur_func.context,
            TraceLocation{cur_func.full_expr_exec_cnt, cur_func.cur_full_expr_id,
                    // it doesn't matter whether call point inner id is coexisting or not
                          InnerID::newCoexisting(info.getInnerID())},
            static_cast<uint32_t>(&func - am.static_info.functions.data())
    };
    am.state.call_stack.emplace_back(&func, am.state.pc, func.max_object_num, *context);
    am.state.pc = func.address;
    do_enterBlock(am, 0);
}

void Execute::indirectJump(AbstractMachine& am)
{
    auto addr = am.operand_stack.popValue();
    CHECK_TYPE(isInteger(addr->getType().kind()));
    auto target_pc = addr.get<IntegerValue>().uint64();
    checkJumpAddr(am, target_pc);
    am.state.pc = target_pc;
}

void Execute::ret(AbstractMachine& am)
{
    auto& current_func = am.state.current_function();
    while (!current_func.blocks.empty()) {
        leaveBlock(am);
    }
    am.state.pc = am.state.current_function().return_address;
    am.state.frame_pointer += current_func.static_info->frame_size;
    am.memory.notifyStackPointer(am.state.frame_pointer);
    COMPILER_GUARANTEE(!am.state.call_stack.empty(),
                       "Instruction `ret` is executed while not in a function environment");
    am.state.call_stack.pop_back();
}

void Execute::pushUndefined(cami::am::AbstractMachine& am)
{
    am.operand_stack.push({ValueBox{new UndefinedValue{}}, {.directly_read_from = {}, .indeterminate = true}});
}

void Execute::push(AbstractMachine& am, InstrInfo info)
{
    CHECK_ID(constant, info.getConstantID(), am.static_info.constants.length());
    am.operand_stack.push(am.static_info.constants[info.getConstantID()]);
}

void Execute::pop(AbstractMachine& am)
{
    am.operand_stack.pop();
}

void Execute::duplicate(AbstractMachine& am)
{
    am.operand_stack.push(am.operand_stack.top());
}

void Execute::dot(AbstractMachine& am, InstrInfo info)
{
    CHECK_DESIGNATION_REGISTER();
    if (am.dsg_reg.offset > 0) {
        throw ConstraintViolationException{"Access member of object designated with nonzero offset"};
    }
    accessMember(am, am.dsg_reg.entity, am.dsg_reg.lvalue_type, info.getMemberID());
}

void Execute::arrow(AbstractMachine& am, InstrInfo info)
{
    auto pointer = am.operand_stack.popDeterminateValue();
    CHECK_TYPE(pointer->getType().kind() == Kind::pointer);
    auto entity = pointer.get<PointerValue>().getReferenced();
    if (!entity) {
        throw ConstraintViolationException{"Access member of nullptr"};
    }
    if (pointer.get<PointerValue>().getOffset() > 0) {
        throw ConstraintViolationException{"Access member of object pointed with nonzero offset"};
    }
    accessMember(am, *entity, &down_cast<const Pointer&>(pointer->getType()).referenced, info.getMemberID());
}

void Execute::address(AbstractMachine& am)
{
    CHECK_DESIGNATION_REGISTER();
    am.operand_stack.push(ValueBox{
            new PointerValue{&type_manager.getPointer(*am.dsg_reg.lvalue_type),
                             am.dsg_reg.entity, am.dsg_reg.offset}});
}

void Execute::accessMember(AbstractMachine& am, const Entity* entity,
                           const ts::Type* _lvalue_type, uint32_t member_id)
{
    const auto [qualifier, lvalue_type] = peelQualify(*_lvalue_type);
    CHECK_TYPE(lvalue_type.kind() == Kind::struct_ || lvalue_type.kind() == Kind::union_);
    auto& obj_type = removeQualify(entity->effective_type);
    if (obj_type.kind() != lvalue_type.kind()) {
        throw ConstraintViolationException{lib::format(
                "Member of entity `${name}`(with type `${}`) is accessed by incompatible lvalue type `${}`",
                *entity, entity->effective_type, lvalue_type)};
    }
    // `obj_type.kind() != Kind::function` since `obj_type.kind() == lvalue_type.kind()` and `lvalue_type.kind()` is struct/union
    auto& obj = down_cast<const Object&>(*entity);
    CHECK_ID(member, member_id, obj.sub_objects.length());
    const Type* member_type;
    if (obj_type.kind() == Kind::struct_) {
        if (down_cast<const Struct&>(obj_type).name != down_cast<const Struct&>(lvalue_type).name) {
            throw ConstraintViolationException{lib::format(
                    "Member of object `${name}`(with type `${}`) is accessed by incompatible lvalue type `${}`",
                    obj, obj.effective_type, lvalue_type)};
        }
        ASSERT(member_id < down_cast<const Struct&>(lvalue_type).members.length(),
               "size of sub-objects do not match size of member of struct");
        member_type = down_cast<const Struct&>(lvalue_type).members[member_id];
    } else {
        if (down_cast<const Union&>(obj_type).name != down_cast<const Union&>(lvalue_type).name) {
            throw ConstraintViolationException{
                    lib::format(
                            "Member of object `${name}`(with type `${}`) is accessed by incompatible lvalue type `${}`",
                            obj, obj.effective_type, lvalue_type)};
        }
        ASSERT(member_id < down_cast<const Union&>(lvalue_type).members.length(),
               "size of sub-objects do not match size of member of struct");
        member_type = down_cast<const Union&>(lvalue_type).members[member_id];
    }
    am.dsg_reg.entity = obj.sub_objects[member_id];
    am.dsg_reg.lvalue_type = &addQualify(*member_type, qualifier);
}

void Execute::basicModifyCheck(cami::am::AbstractMachine& am, bool ignore_const)
{
    CHECK_DESIGNATION_REGISTER();
    if (am.dsg_reg.entity->effective_type.kind() == Kind::function) {
        throw ConstraintViolationException{lib::format("Modify function `${name}`", *am.dsg_reg.entity)};
    }
    auto& obj = down_cast<Object&>(*am.dsg_reg.entity);
    if (obj.status == Object::Status::destroyed) {
        throw UBException{{UB::refer_del_obj, UB::eva_ivd_lvalue}, lib::format(
                "Object `${name}` is written after free\n${}", obj, obj)};
    }
    if (obj.effective_type.kind() == Kind::qualify && !ignore_const) {
        applyRecursively(obj, [&](Object& o) {
            if (auto qualifier = down_cast<const Qualify&>(o.effective_type).qualifier;
                    qualifier & Qualifier::const_) {
                throw UBException{{UB::modify_const_obj}, lib::format("Modify const object `${name}`\n${}", obj, obj)};
            }
        });
    }
}

void Execute::modifyCheck(am::AbstractMachine& am, bool ignore_const)
{
    const auto inexactlyOverlap = [](Object& a, Object& b) {
        auto a_start = a.address;
        auto a_end = a.address + a.effective_type.size();
        auto b_start = b.address;
        auto b_end = b.address + b.effective_type.size();
        if (a_start == b_start && a_end == b_end) {
            return false;
        }
        return a_start < b_start ? a_end > b_start : a_start < b_end;
    };
    Execute::basicModifyCheck(am, ignore_const);
    auto& obj = down_cast<Object&>(*am.dsg_reg.entity);
    auto& rv = am.operand_stack.topDeterminate();
    if (auto directly_read_from = rv.attr.directly_read_from; directly_read_from) {
        if (inexactlyOverlap(obj, **directly_read_from)) {
            throw UBException{{UB::overlap_obj_assign}, lib::format(
                    "Value assign to object `${name}` is directly read from `${name}` which is incompletely overlap the former\n${}\n${}",
                    obj, **directly_read_from, obj, **directly_read_from)};
        }
    }
    if (removeQualify(obj.effective_type).kind() == Kind::array) {
        throw ConstraintViolationException{lib::format("Modify array object `${name}`", obj)};
    }
    auto& write_value_type =
            rv.vb->getType().kind() == Kind::dissociative_pointer ? *rv.vb.get<DissociativePointerValue>().pointer_type : rv.vb->getType();
    if (!isAllowed(write_value_type, obj.effective_type)) {
        throw ConstraintViolationException{lib::format(
                "object '${}' is modified by incompatible type `${}`", obj, write_value_type)};
    }
    if (am.dsg_reg.offset > 0 && !isCCharacter(write_value_type.kind())) {
        throw ConstraintViolationException{
                lib::format("entity '${}' is write with non-character lvalue type and nonzero offset", *am.dsg_reg.entity)};
    }
}

void Execute::checkJumpAddr(AbstractMachine& am, uint64_t target_pc)
{
    auto& current_func_info = am.state.current_function().static_info;
    if (target_pc < current_func_info->address
        || target_pc >= current_func_info->address + current_func_info->code_size) {
        throw JumpOutOfBoundaryException{target_pc};
    }
}
