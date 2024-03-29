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

#ifndef CAMI_AM_EXECUTE_H
#define CAMI_AM_EXECUTE_H

#include "am.h"
#include "fetch_decode.h"
#include "trace.h"
#include "trace_data.h"

namespace cami::am {
struct Execute
{
    static void designate(AbstractMachine& am, InstrInfo info);
    static void dereference(AbstractMachine& am);
    static void read(AbstractMachine& am, InstrInfo info);
    static void modify(AbstractMachine& am, InstrInfo info);
    static void zero(AbstractMachine& am, InstrInfo info);
    static void writeInit(AbstractMachine& am);
    static void zeroInit(AbstractMachine& am);
    static void enterBlock(AbstractMachine& am, InstrInfo info);
    static void leaveBlock(AbstractMachine& am);
    static void newObject(AbstractMachine& am, InstrInfo info);
    static void deleteObject(AbstractMachine& am, InstrInfo info);
    static void fullExpression(AbstractMachine& am, InstrInfo info);
    static void jump(AbstractMachine& am, InstrInfo info);
    static void jumpIfSet(AbstractMachine& am, InstrInfo info);
    static void jumpIfNotSet(AbstractMachine& am, InstrInfo info);
    static void call(AbstractMachine& am, InstrInfo);
    static void indirectJump(AbstractMachine& am);
    static void ret(AbstractMachine& am);
    static void pushUndefined(AbstractMachine& am);
    static void push(AbstractMachine& am, InstrInfo info);
    static void pop(AbstractMachine& am);
    static void duplicate(AbstractMachine& am);
    static void dot(AbstractMachine& am, InstrInfo info);
    static void arrow(AbstractMachine& am, InstrInfo info);
    static void address(AbstractMachine& am);
    static void cast(AbstractMachine& am, InstrInfo info);
    static void unaryOperator(AbstractMachine& am, Opcode op);
    static void binaryOperator(AbstractMachine& am, Opcode op);
private:
    static void modifyCheck(AbstractMachine& am, bool ignore_const);
    static void basicModifyCheck(AbstractMachine& am, bool ignore_const);
    static void do_read(AbstractMachine& am);
    static void do_modify(AbstractMachine& am, ValueBox vb);
    static void accessMember(AbstractMachine& am, const Entity* entity, const ts::Type* lvalue_type, uint32_t member_id);
    static void do_enterBlock(AbstractMachine& am, uint32_t block_id);
    static void checkJumpAddr(AbstractMachine& am, uint64_t target_pc);
    static void castPointerToInteger(cami::ValueBox& operand, const ts::Type& type);
    static void castIntegerToPointer(AbstractMachine& am, ValueBox& operand, const ts::Type& type);
    static void castPointerToPointer(ValueBox& operand, const ts::Type& type);

    static void attachTag(AbstractMachine& am, Object& object, InnerID inner_id)
    {
        auto& cur_func = am.state.current_function();
        Trace::attachTag(am, object,
                         {cur_func.context, {cur_func.full_expr_exec_cnt, cur_func.cur_full_expr_id, inner_id}});
    }

};

} // namespace cami::am

#endif //CAMI_AM_EXECUTE_H
