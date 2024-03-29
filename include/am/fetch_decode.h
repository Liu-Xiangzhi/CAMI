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

#ifndef CAMI_AM_FETCH_DECODE_H
#define CAMI_AM_FETCH_DECODE_H

#include <cstdint>
#include <lib/bitmap.h>
#include <lib/format.h>
#include "am.h"

namespace cami::am {

enum class Opcode : uint8_t
{
    dsg = 1, drf, read, mdf, zero, mdfi, zeroi, // mdfi means Init by MoDiFy
    eb = 16, lb, new_, del, fe,
    j = 32, jst, jnt, call, ij, ret,
    nop = 0, pushu /* push undef */ = 251, push = 252, pop = 253, dup = 254, halt = 255,
    dot = 128, arrow, addr, cast, cpl, pos, neg, not_, mul, div, mod,
    add, sub, ls, rs, sl, sle, sg, sge, seq, sne, and_, or_, xor_,
};

struct InstrInfo
{
private:
    friend class FetchDecode;

    union
    {
        uint32_t id;
        int64_t offset;
    };
public:
    static constexpr uint64_t ID_MAX = 0xff'ff'ff;
    static constexpr uint64_t FUNCTION_ID_MAX = ID_MAX >> 2;
    static constexpr uint64_t STATIC_OBJECT_ID_MAX = ID_MAX >> 2;
    static constexpr uint64_t AUTOMATIC_OBJECT_ID_MAX = ID_MAX >> 1;

    class IdentifierId
    {
        uint32_t id;
    public:
        explicit IdentifierId(uint32_t id) : id(id) {}

    public:
        static uint32_t fromFunctionIndex(uint64_t idx)
        {
            return (idx << 2) | 0x3;
        }

        static uint32_t fromStaticObject(uint64_t idx)
        {
            return (idx << 2) | 0x1;
        }

        static uint32_t fromAutomaticObject(uint64_t idx)
        {
            return idx << 2;
        }

        [[nodiscard]] bool isGlobal() const noexcept
        {
            return this->id & 1;
        }

        [[nodiscard]] bool isFuntion() const noexcept
        {
            return this->id & 2;
        }

        [[nodiscard]] bool isObject() const noexcept
        {
            return !this->isFuntion();
        }

        [[nodiscard]] uint32_t value() const noexcept
        {
            return this->id >> 2;
        }
    };

    struct InnerID
    {
        static bool isCoexisting(uint32_t id)
        {
            return id & 1;
        }

    };

    [[nodiscard]] IdentifierId getIdentifierID() const noexcept
    {
        return IdentifierId{this->id};
    }

    [[nodiscard]] int64_t getOffset() const noexcept
    {
        return this->offset;
    }

    [[nodiscard]] uint32_t getConstantID() const noexcept
    {
        return this->id;
    }

    [[nodiscard]] uint32_t getMemberID() const noexcept
    {
        return this->id;
    }

    [[nodiscard]] uint32_t getTypeID() const noexcept
    {
        return this->id;
    }

    [[nodiscard]] uint32_t getBlockID() const noexcept
    {
        return this->id;
    }

    [[nodiscard]] uint32_t getFullExprID() const noexcept
    {
        return this->id;
    }

    [[nodiscard]] uint32_t getInnerID() const noexcept
    {
        return this->id;
    }
};

class AbstractMachine;

class FetchDecode
{
    static constexpr const lib::Bitmap<256> ops{
            Opcode::dsg, Opcode::read, Opcode::mdf, Opcode::zero, Opcode::eb, Opcode::new_, Opcode::del, Opcode::fe,
            Opcode::j, Opcode::jst, Opcode::jnt, Opcode::call, Opcode::push, Opcode::dot, Opcode::arrow, Opcode::cast};
public:
    static constexpr bool isJump(Opcode op)
    {
        return op >= Opcode::j && op <= Opcode::jnt;
    }

    static constexpr bool isUnaryOperator(Opcode op)
    {
        return op >= Opcode::cpl && op <= Opcode::not_;
    }

    static constexpr bool isBinaryOperator(Opcode op)
    {
        return op >= Opcode::mul && op <= Opcode::xor_;
    }

    static constexpr bool isOperator(Opcode op)
    {
        return op >= Opcode::dot && op <= Opcode::xor_;
    }

    static constexpr bool hasExtraInfo(Opcode op)
    {
        return ops.test(static_cast<uint64_t>(op));
    }

    static std::pair<Opcode, InstrInfo> decode(AbstractMachine& am);
private:
    static uint32_t readUint24(VirtualMemory& memory, uint64_t pc);
    static int64_t readInt24(VirtualMemory& memory, uint64_t pc);
};
} // namespace cami::am

CAMI_DECLARE_FORMATTER(cami::am::Opcode);

#endif //CAMI_AM_FETCH_DECODE_H
