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
#include <exception.h>
#include <foundation/type/helper.h>
#include <ub.h>

using namespace cami;
using namespace ts;
using am::Execute;
using am::InstrInfo;
using am::Entity;
using am::Object;
using am::UB;

#ifndef NDEBUG
#define ALL_CASE_LISTED                                                                            \
    default:                                                                                       \
    ASSERT(false, "all cases has listed in switch statement, no other case should have happened"); \
    break;
#else
#define ALL_CASE_LISTED
#endif

namespace {
// check whether
//  - ptr is nullptr, or
//  - type of the entity referenced by ptr is function, or
//  - reference type of ptr is function, or
//  - ptr type matches referenced entity type, or
//    * if so, return false
//    * otherwise, return true(which means using `char*` to observe the object representation).
bool checkPointer(PointerValue& ptr)
{
    auto entity = ptr.getReferenced();
    if (!entity.has_value()) {
        throw UBException{{UB::use_ptr_value_which_ref_del_obj}, "nullptr is used in evaluation of pointer"};
    }
    auto& entity_type = removeQualify((*entity)->effective_type);
    auto& ref_type = down_cast<const Pointer&>(ptr.getType()).referenced;
    if (entity_type.kind() == Kind::function || ref_type.kind() == Kind::function) {
        throw ConstraintViolationException{"Pointer Arithmetic is performed on pointer which references function"};
    }
    if (!isLoosestCompatible(ref_type, entity_type)) {
        if (!isCCharacter(ref_type.kind())) {
            throw ConstraintViolationException{
                    "Pointer Arithmetic is performed on pointer which is neither character type nor"
                    " compatible with the type of referenced object"};
        }
        return true;
    }
    return false;
}

void pointerAdd(PointerValue& ptr, uint64_t offset_in_element)
{
    bool is_char = checkPointer(ptr);
    auto& obj = down_cast<Object&>(**ptr.getReferenced());
    auto& obj_type = removeQualify(obj.effective_type);
    if (is_char) {
        auto off = ptr.getOffset() + offset_in_element;
        if (off > obj_type.size()) {
            throw UBException{{UB::ptr_addition_oob, UB::idx_oob}, lib::format(
                    "Character pointer addition out of boundary\n${}\noffset: ${}", ptr, offset_in_element)};
        }
        ptr.set(off);
        return;
    }
    auto obj_size = obj_type.size();
    auto super_obj = obj.super_object;
    if (!super_obj || removeQualify((*super_obj)->effective_type).kind() != Kind::array) {
        // treat as an array of len 1
        auto off_in_byte = ptr.getOffset();
        ASSERT(off_in_byte == 0 || off_in_byte == obj_size, "invalid offset");
        size_t off_in_element = off_in_byte == obj_size; // equivalent to `off_in_byte / obj_size`
        off_in_element += offset_in_element;
        if (off_in_element > 1) {
            throw UBException{{UB::ptr_addition_oob, UB::idx_oob}, lib::format(
                    "Pointer addition on top object out of boundary\n${}\noffset: ${}", ptr, offset_in_element)};
        }
        ptr.set(off_in_element * obj_size);
        return;
    }
    auto& super_obj_type = removeQualify((*super_obj)->effective_type);
    auto idx = (obj.address + ptr.getOffset() - (*super_obj)->address) / obj_size;
    idx += offset_in_element;
    auto array_len = down_cast<const Array&>(super_obj_type).len;
    if (idx > array_len) {
        throw UBException{{UB::ptr_addition_oob, UB::idx_oob}, lib::format(
                "Pointer addition out of boundary\narray length = ${} pointed index = ${}", array_len, idx)};
    }
    if (idx == array_len) {
        ptr.set((*super_obj)->sub_objects[idx - 1], obj_size);
    } else {
        ptr.set((*super_obj)->sub_objects[idx], 0);
    }
}

ValueBox pointerDiff(PointerValue& lhs, PointerValue& rhs)
{
    auto lhs_is_char = checkPointer(lhs);
    [[maybe_unused]] auto rhs_is_char = checkPointer(rhs);
    ASSERT(lhs_is_char == rhs_is_char, "precondition violation");
    if (lhs_is_char) {
        if (&down_cast<Object*>(*lhs.getReferenced())->top() != &down_cast<Object*>(*rhs.getReferenced())->top()) {
            throw UBException{{UB::ivd_ptr_subtraction}, lib::format(
                    "Two character pointers which are not reference same object are subtracted\n${}\n${}", lhs, rhs)};
        }
        return ValueBox{new IntegerValue{&type_manager.getBasicType(Kind::i64), lhs.getAddress() - rhs.getAddress()}};
    }
    auto& lhs_obj = down_cast<Object&>(**lhs.getReferenced());
    auto& rhs_obj = down_cast<Object&>(**rhs.getReferenced());
    if (lhs_obj.super_object.has_value() != rhs_obj.super_object.has_value()) {
        throw UBException{{UB::ivd_ptr_subtraction}, lib::format(
                "Two pointers that are not reference element of same array object are subtracted\n${}\n${}", lhs, rhs)};
    }
    if (!lhs_obj.super_object || removeQualify((*lhs_obj.super_object)->effective_type).kind() != Kind::array) {
        // treat as an array of len 1
        if (&lhs_obj != &rhs_obj) {
            throw UBException{{UB::ivd_ptr_subtraction}, lib::format(
                    "Two pointers on top object that are not reference same object are subtracted\n${}\n${}",
                    lhs, rhs)};
        }
        ASSERT(lhs.getOffset() == 0 || lhs.getOffset() == lhs_obj.size(), "invalid offset");
        ASSERT(rhs.getOffset() == 0 || rhs.getOffset() == lhs_obj.size(), "invalid offset");
        return ValueBox{new IntegerValue{&type_manager.getBasicType(Kind::i64),
                                         static_cast<uint64_t>(!lhs.getOffset() - !rhs.getOffset())}};
        // equivalent to (rhs.getOffset() - lhs.getOffset()) / lhs_obj_type.size()
    }
    if (*lhs_obj.super_object != *rhs_obj.super_object) {
        throw UBException{{UB::ivd_ptr_subtraction}, lib::format(
                "Two pointers that are not reference element of same array object are subtracted\n${}\n${}", lhs, rhs)};
    }
    ASSERT(&lhs_obj.effective_type == &rhs_obj.effective_type, "two elements of the same array must have same type");
    auto ptr_diff = lhs.getAddress() - rhs.getAddress();
    auto res = *reinterpret_cast<int64_t*>(&ptr_diff) / static_cast<int64_t>(lhs_obj.size());
    return ValueBox{new IntegerValue{&type_manager.getBasicType(Kind::i64), static_cast<uint64_t>(res)}};
}

int pointerCmp(PointerValue& lhs, PointerValue& rhs)
{
    const auto cmp = [](uint64_t lhs, uint64_t rhs) -> int {
        return lhs < rhs ? -1 : lhs != rhs; // equivalent to `lhs == rhs ? 0 : (lhs > rhs ? 1 : -1)`
    };
    checkPointer(lhs);
    checkPointer(rhs);
    auto& lhs_top_obj = down_cast<Object*>(*lhs.getReferenced())->top();
    auto& rhs_top_obj = down_cast<Object*>(*rhs.getReferenced())->top();
    if (&lhs_top_obj != &rhs_top_obj) {
        throw UBException{{UB::ivd_ptr_compare}, lib::format(
                "Two pointers which are not reference same top object are compared\n${}\n${}", lhs, rhs)};
    }
    return cmp(lhs.getAddress(), rhs.getAddress());
}
} // anonymous namespace

void Execute::unaryOperator(AbstractMachine& am, Opcode op)
{
    auto rich_value = am.operand_stack.pop();
    if (rich_value.attr.indeterminate) {
        am.operand_stack.push(std::move(rich_value));
        return;
    }
    auto& operand = rich_value.vb;
    switch (op) {
    case Opcode::pos:
        COMPILER_GUARANTEE(isArithmetic(operand->getType().kind()),
                           lib::format("invalid type `${}` of unary +", operand->getType()));
        operand.inplacePositive();
        break;
    case Opcode::neg:
        COMPILER_GUARANTEE(isArithmetic(operand->getType().kind()),
                           lib::format("invalid type `${}`of unary -", operand->getType()));
        operand.inplaceNegation();
        break;
    case Opcode::cpl:
        COMPILER_GUARANTEE(isInteger(operand->getType().kind()),
                           lib::format("invalid type `${}`of ~", operand->getType()));
        operand.inplaceComplement();
        break;
    case Opcode::not_:
        COMPILER_GUARANTEE(isScalar(operand->getType().kind()),
                           lib::format("invalid type `${}` of !", operand->getType()));
        if (operand->getType().kind() == Kind::null) {
            operand = ValueBox{new IntegerValue{&type_manager.getBasicType(Kind::i32), 1}};
        } else if (operand->getType().kind() == Kind::pointer) {
            operand = ValueBox{new IntegerValue{&type_manager.getBasicType(Kind::i32),
                                                !operand.get<PointerValue>().isZero()}};
        } else if (operand->getType().kind() == Kind::dissociative_pointer) {
            operand = ValueBox{new IntegerValue{&type_manager.getBasicType(Kind::i32),
                                                operand.get<DissociativePointerValue>().address != 0}};
        } else {
            operand = !operand;
        }
        break;
    ALL_CASE_LISTED
    }
    am.operand_stack.push(std::move(operand));
}

#define POINTER_CMP_COMPILER_GUARANTEE(op)\
    COMPILER_GUARANTEE(lhs->getType().kind() == Kind::pointer && rhs->getType().kind() == Kind::pointer && \
               &down_cast<const Pointer&>(lhs->getType()).referenced == &down_cast<const Pointer&>(rhs->getType()).referenced, \
               lib::format("invalid type combination `${}` `${}` of operands of " op, lhs->getType(), rhs->getType()))

void Execute::binaryOperator(AbstractMachine& am, Opcode op)
{
    auto rhs_rv = am.operand_stack.pop();
    auto lhs_rv = am.operand_stack.pop();
    if (lhs_rv.attr.indeterminate) {
        am.operand_stack.push(std::move(lhs_rv));
        return;
    }
    if (rhs_rv.attr.indeterminate) {
        am.operand_stack.push(std::move(rhs_rv));
        return;
    }
    auto& lhs = lhs_rv.vb;
    auto& rhs = rhs_rv.vb;
    switch (op) {
    case Opcode::add:
        if (lhs->getType().kind() == Kind::pointer) {
            COMPILER_GUARANTEE(isInteger(rhs->getType().kind()), lib::format(
                    "invalid type combination `${}` `${}` of operands of binary +", lhs->getType(), rhs->getType()));
            pointerAdd(lhs.get<PointerValue>(), rhs.get<IntegerValue>().uint64());
        } else if (rhs->getType().kind() == Kind::pointer) {
            COMPILER_GUARANTEE(isInteger(lhs->getType().kind()), lib::format(
                    "invalid type combination `${}` `${}` of operands of binary +", lhs->getType(), rhs->getType()));
            pointerAdd(rhs.get<PointerValue>(), lhs.get<IntegerValue>().uint64());
            lhs = std::move(rhs);
        } else {
            if (lhs->getType().kind() == Kind::dissociative_pointer || rhs->getType().kind() == Kind::dissociative_pointer) {
                throw UBException{{UB::eva_ivd_lvalue, UB::ptr_addition_oob}, "dissociative pointer addition\n"};
            }
            COMPILER_GUARANTEE(isArithmetic(lhs->getType().kind()) && isArithmetic(rhs->getType().kind()), lib::format(
                    "invalid type combination `${}` `${}` of operands of binary +", lhs->getType(), rhs->getType()));
            lhs += rhs;
        }
        break;
    case Opcode::sub:
        if (lhs->getType().kind() == Kind::pointer) {
            if (rhs->getType().kind() == Kind::pointer) {
                COMPILER_GUARANTEE(&down_cast<const Pointer&>(lhs->getType()).referenced ==
                                   &down_cast<const Pointer&>(rhs->getType()).referenced, lib::format(
                        "invalid type combination `${}` `${}` of operands of binary -", lhs->getType(), rhs->getType()));
                lhs = pointerDiff(lhs.get<PointerValue>(), rhs.get<PointerValue>());
            } else {
                COMPILER_GUARANTEE(isInteger(rhs->getType().kind()), lib::format(
                        "invalid type combination `${}` `${}` of operands of binary -", lhs->getType(), rhs->getType()));
                pointerAdd(lhs.get<PointerValue>(), -rhs.get<IntegerValue>().uint64());
            }
        } else {
            if (lhs->getType().kind() == Kind::dissociative_pointer || rhs->getType().kind() == Kind::dissociative_pointer) {
                throw UBException{{UB::eva_ivd_lvalue, UB::ivd_ptr_subtraction}, "dissociative pointer subtraction\n"};
            }
            COMPILER_GUARANTEE(isArithmetic(lhs->getType().kind()) && isArithmetic(rhs->getType().kind()), lib::format(
                    "invalid type combination `${}` `${}` of operands of binary -", lhs->getType(), rhs->getType()));
            lhs -= rhs;
        }
        break;
    case Opcode::mul:
        COMPILER_GUARANTEE(isArithmetic(lhs->getType().kind()) && isArithmetic(rhs->getType().kind()), lib::format(
                "invalid type combination `${}` `${}` of operands of binary *", lhs->getType(), rhs->getType()));
        lhs *= rhs;
        break;
    case Opcode::div:
        COMPILER_GUARANTEE(isArithmetic(lhs->getType().kind()) && isArithmetic(rhs->getType().kind()), lib::format(
                "invalid type combination `${}` `${}` of operands of /", lhs->getType(), rhs->getType()));
        lhs /= rhs;
        break;
    case Opcode::mod:
        COMPILER_GUARANTEE(isInteger(lhs->getType().kind()) && isInteger(rhs->getType().kind()), lib::format(
                "invalid type combination `${}` `${}` of operands of %", lhs->getType(), rhs->getType()));
        lhs %= rhs;
        break;
    case Opcode::ls:
        COMPILER_GUARANTEE(isInteger(lhs->getType().kind()) && isInteger(rhs->getType().kind()), lib::format(
                "invalid type combination `${}` `${}` of operands of <<", lhs->getType(), rhs->getType()));
        lhs <<= rhs;
        break;
    case Opcode::rs:
        COMPILER_GUARANTEE(isInteger(lhs->getType().kind()) && isInteger(rhs->getType().kind()), lib::format(
                "invalid type combination `${}` `${}` of operands of >>", lhs->getType(), rhs->getType()));
        lhs >>= rhs;
        break;
    case Opcode::sl:
        if (isReal(lhs->getType().kind())) {
            COMPILER_GUARANTEE(isReal(rhs->getType().kind()), lib::format(
                    "invalid type combination `${}` `${}` of operands of <", lhs->getType(), rhs->getType()));
            lhs = std::move(lhs) < std::move(rhs);
        } else {
            if (lhs->getType().kind() == Kind::dissociative_pointer || rhs->getType().kind() == Kind::dissociative_pointer) {
                throw UBException{{UB::eva_ivd_lvalue, UB::ivd_ptr_compare}, "dissociative pointer compare\n"};
            }
            POINTER_CMP_COMPILER_GUARANTEE("<");
            lhs = ValueBox{new IntegerValue{&type_manager.getBasicType(Kind::i32),
                                            pointerCmp(lhs.get<PointerValue>(), rhs.get<PointerValue>()) < 0}};
        }
        break;
    case Opcode::sle:
        if (isReal(lhs->getType().kind())) {
            COMPILER_GUARANTEE(isReal(rhs->getType().kind()), lib::format(
                    "invalid type combination `${}` `${}` of operands of <=", lhs->getType(), rhs->getType()));
            lhs = std::move(lhs) <= std::move(rhs);
        } else {
            if (lhs->getType().kind() == Kind::dissociative_pointer || rhs->getType().kind() == Kind::dissociative_pointer) {
                throw UBException{{UB::eva_ivd_lvalue, UB::ivd_ptr_compare}, "dissociative pointer compare\n"};
            }
            POINTER_CMP_COMPILER_GUARANTEE("<=");
            lhs = ValueBox{new IntegerValue{&type_manager.getBasicType(Kind::i32),
                                            pointerCmp(lhs.get<PointerValue>(), rhs.get<PointerValue>()) <= 0}};
        }
        break;
    case Opcode::sg:
        if (isReal(lhs->getType().kind())) {
            COMPILER_GUARANTEE(isReal(rhs->getType().kind()), lib::format(
                    "invalid type combination `${}` `${}` of operands of >", lhs->getType(), rhs->getType()));
            lhs = std::move(lhs) > std::move(rhs);
        } else {
            if (lhs->getType().kind() == Kind::dissociative_pointer || rhs->getType().kind() == Kind::dissociative_pointer) {
                throw UBException{{UB::eva_ivd_lvalue, UB::ivd_ptr_compare}, "dissociative pointer compare\n"};
            }
            POINTER_CMP_COMPILER_GUARANTEE(">");
            lhs = ValueBox{new IntegerValue{&type_manager.getBasicType(Kind::i32),
                                            pointerCmp(lhs.get<PointerValue>(), rhs.get<PointerValue>()) > 0}};
        }
        break;
    case Opcode::sge:
        if (isReal(lhs->getType().kind())) {
            COMPILER_GUARANTEE(isReal(rhs->getType().kind()), lib::format(
                    "invalid type combination `${}` `${}` of operands of >=", lhs->getType(), rhs->getType()));
            lhs = std::move(lhs) >= std::move(rhs);
        } else {
            if (lhs->getType().kind() == Kind::dissociative_pointer || rhs->getType().kind() == Kind::dissociative_pointer) {
                throw UBException{{UB::eva_ivd_lvalue, UB::ivd_ptr_compare}, "dissociative pointer compare\n"};
            }
            POINTER_CMP_COMPILER_GUARANTEE(">=");
            lhs = ValueBox{new IntegerValue{&type_manager.getBasicType(Kind::i32),
                                            pointerCmp(lhs.get<PointerValue>(), rhs.get<PointerValue>()) >= 0}};
        }
        break;
    case Opcode::seq:
        if (lhs->getType().kind() == Kind::dissociative_pointer || rhs->getType().kind() == Kind::dissociative_pointer) {
            throw UBException{{UB::eva_ivd_lvalue, UB::ivd_ptr_compare}, "dissociative pointer compare\n"};
        }
        COMPILER_GUARANTEE(isArithmetic(lhs->getType().kind()) ?
                           isArithmetic(rhs->getType().kind()) :
                           isPointerLike(lhs->getType().kind()) && isPointerLike(rhs->getType().kind()), lib::format(
                "invalid type combination `${}` `${}` of operands of ==", lhs->getType(), rhs->getType()));
        lhs = std::move(lhs) == std::move(rhs);
        break;
    case Opcode::sne:
        if (lhs->getType().kind() == Kind::dissociative_pointer || rhs->getType().kind() == Kind::dissociative_pointer) {
            throw UBException{{UB::eva_ivd_lvalue, UB::ivd_ptr_compare}, "dissociative pointer compare\n"};
        }
        COMPILER_GUARANTEE(isArithmetic(lhs->getType().kind()) ?
                           isArithmetic(rhs->getType().kind()) :
                           isPointerLike(lhs->getType().kind()) && isPointerLike(rhs->getType().kind()), lib::format(
                "invalid type combination `${}` `${}` of operands of !=", lhs->getType(), rhs->getType()));
        lhs = std::move(lhs) != std::move(rhs);
        break;
    case Opcode::and_:
        COMPILER_GUARANTEE(isInteger(lhs->getType().kind()) && isInteger(rhs->getType().kind()), lib::format(
                "invalid type combination `${}` `${}` of operands of &", lhs->getType(), rhs->getType()));
        lhs &= rhs;
        break;
    case Opcode::or_:
        COMPILER_GUARANTEE(isInteger(lhs->getType().kind()) && isInteger(rhs->getType().kind()), lib::format(
                "invalid type combination `${}` `${}` of operands of |", lhs->getType(), rhs->getType()));
        lhs |= rhs;
        break;
    case Opcode::xor_:
        COMPILER_GUARANTEE(isInteger(lhs->getType().kind()) && isInteger(rhs->getType().kind()), lib::format(
                "invalid type combination `${}` `${}` of operands of ^", lhs->getType(), rhs->getType()));
        lhs ^= rhs;
        break;
    ALL_CASE_LISTED
    }
    am.operand_stack.push(std::move(lhs));
}

#undef POINTER_CMP_COMPILER_GUARANTEE
