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

#ifndef CAMI_AM_EVALUATION_H
#define CAMI_AM_EVALUATION_H

#include <deque>
#include <utility>
#include <foundation/value.h>
#include <lib/compiler_guarantee.h>
#include <lib/format.h>
#include "exception.h"

namespace cami::am {

ValueBox operator+(ValueBox lhs, ValueBox rhs);
ValueBox operator-(ValueBox lhs, ValueBox rhs);
ValueBox operator*(ValueBox lhs, ValueBox rhs);
ValueBox operator/(ValueBox lhs, ValueBox rhs);
ValueBox operator%(ValueBox lhs, ValueBox rhs);
ValueBox operator<<(ValueBox lhs, ValueBox rhs);
ValueBox operator>>(ValueBox lhs, ValueBox rhs);
ValueBox operator<(ValueBox lhs, ValueBox rhs);
ValueBox operator<=(ValueBox lhs, ValueBox rhs);
ValueBox operator>(ValueBox lhs, ValueBox rhs);
ValueBox operator>=(ValueBox lhs, ValueBox rhs);
ValueBox operator==(ValueBox lhs, ValueBox rhs);
ValueBox operator!=(ValueBox lhs, ValueBox rhs);
ValueBox operator&(ValueBox lhs, ValueBox rhs);
ValueBox operator|(ValueBox lhs, ValueBox rhs);
ValueBox operator^(ValueBox lhs, ValueBox rhs);
void operator+=(ValueBox& lhs, ValueBox rhs);
void operator-=(ValueBox& lhs, ValueBox rhs);
void operator*=(ValueBox& lhs, ValueBox rhs);
void operator/=(ValueBox& lhs, ValueBox rhs);
void operator%=(ValueBox& lhs, ValueBox rhs);
void operator<<=(ValueBox& lhs, ValueBox rhs);
void operator>>=(ValueBox& lhs, ValueBox rhs);
void operator&=(ValueBox& lhs, ValueBox rhs);
void operator|=(ValueBox& lhs, ValueBox rhs);
void operator^=(ValueBox& lhs, ValueBox rhs);

class OperandStack
{
public:
    struct Attribute
    {
        lib::Optional<Object*> directly_read_from{};
        bool indeterminate = false;
    };

    struct RichValue
    {
        ValueBox vb;
        Attribute attr;

        RichValue(ValueBox vb, const Attribute& attr) : vb(std::move(vb)), attr(attr) {}
    };

private:
    std::deque<RichValue> stack;
public:
    RichValue& top()
    {
        COMPILER_GUARANTEE(!this->stack.empty(), "read empty operand stack");
        return stack.back();
    }

    RichValue pop()
    {
        auto rich_value = std::move(this->top());
        this->stack.pop_back();
        return rich_value;
    }
    RichValue& topDeterminate()
    {
        auto& rich_value = this->top();
        if (rich_value.attr.indeterminate) {
            throw UBException{{UB::store_nvr, UB::return_undefined}, lib::format(
                    "indeterminate value of type `${}` is used", rich_value.vb->getType())};
        }
        return rich_value;
    }

    RichValue popDeterminate()
    {
        auto rich_value = std::move(this->top());
        this->stack.pop_back();
        if (rich_value.attr.indeterminate) {
            throw UBException{{UB::store_nvr, UB::return_undefined}, lib::format(
                    "indeterminate value of type `${}` is used", rich_value.vb->getType())};
        }
        return rich_value;
    }

    ValueBox popValue()
    {
        return this->pop().vb;
    }

    ValueBox popDeterminateValue()
    {
        return this->popDeterminate().vb;
    }

    void push(ValueBox vb)
    {
        this->stack.emplace_back(std::move(vb), Attribute{});
    }

    void push(const RichValue& rich_value)
    {
        this->stack.push_back(rich_value);
    }

    void push(RichValue&& rich_value)
    {
        this->stack.push_back(std::move(rich_value));
    }

    [[nodiscard]] const std::deque<RichValue>& getStack() const noexcept
    {
        return this->stack;
    }
};

} // namespace cami::am

CAMI_DECLARE_FORMATTER(cami::am::OperandStack::RichValue);

CAMI_DECLARE_FORMATTER(cami::am::OperandStack);

#endif //CAMI_AM_EVALUATION_H
