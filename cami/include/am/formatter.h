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

#ifndef CAMI_AM_FORMATTER_H
#define CAMI_AM_FORMATTER_H

#include <string>
#include <foundation/type/def.h>
#include <foundation/value.h>
#include "fetch_decode.h"
#include "evaluation.h"
#include "object.h"
#include "heap_allocator.h"
#include "obj_man.h"
#include "spd.h"
#include "state.h"

namespace cami::am {
class AbstractMachine;

struct Formatter
{
    size_t indent = 0;
    const AbstractMachine* am;

    explicit Formatter(const AbstractMachine* am) : am(am) {}

    Formatter(size_t indent, const AbstractMachine* am) : indent(indent), am(am) {}

public:
    // static functions mean formatted result is in one line
    // non-static means multiply lines(and all indented)
    static std::string opcode(Opcode opcode);
    static std::string type(const ts::Type& type);
    static std::string integerValue(const IntegerValue& value, std::string_view specifier);
    static std::string f32Value(const F32Value& value);
    static std::string f64Value(const F64Value& value);
    static std::string pointerValue(const PointerValue& value);
    static std::string dissociativePointerValue(const DissociativePointerValue& value);
    static std::string structOrUnionValue(const StructOrUnionValue& value);
    static std::string nullValue();
    static std::string undefinedValue();
    static std::string valueBox(const ValueBox& vb, std::string_view specifier);
    static std::string richValue(const OperandStack::RichValue& rv, std::string_view specifier);
    static std::string objectStatus(Object::Status status);
    static std::string objectName(const Object& obj);
    static std::string briefObject(const Object& obj);
    std::string object(const Object& obj, bool full);
    [[nodiscard]] std::string tag(const Object::Tag& tag) const;
    static std::string operandStack(const OperandStack& operand_stack);
    static std::string simpleAllocator(const SimpleAllocator& allocator);
    static std::string objectManager(const ObjectManager& object_manager);
    static std::string staticFuncInfo(const spd::Function& function);
private:
    static std::string formatType(const ts::Type& type);
    std::string formatSubObject(const Object& obj);
    void formatObjectStandardInfo(std::string& result, const Object& obj);
    void formatObjectDetailInfo(std::string& result, const Object& obj);
};

} // namespace cami::am

#endif //CAMI_AM_FORMATTER_H
