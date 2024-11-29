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

#include <fetch_decode.h>
#include <am.h>

using namespace cami;
using am::FetchDecode;
using am::Opcode;
using am::InstrInfo;
using am::VirtualMemory;

std::pair<Opcode, InstrInfo> FetchDecode::decode(AbstractMachine& am)
{
    auto op = static_cast<Opcode>(am.memory.read8(am.state.pc));
    InstrInfo extra_info{};
    if (FetchDecode::hasExtraInfo(op)) {
        if (FetchDecode::isJump(op)) {
            extra_info.offset = FetchDecode::readInt24(am.memory, am.state.pc + 1);
        } else {
            extra_info.id = FetchDecode::readUint24(am.memory, am.state.pc + 1);
        }
        am.state.pc += 4;
    } else {
        am.state.pc += 1;
    }
    return {op, extra_info};
}

int64_t FetchDecode::readInt24(VirtualMemory& memory, uint64_t pc)
{
    int64_t value = FetchDecode::readUint24(memory, pc);
    return (value << 40) >> 40;
}

uint32_t FetchDecode::readUint24(VirtualMemory& memory, uint64_t pc)
{
#ifdef CAMI_TARGET_INFO_LITTLE_ENDIAN
    uint32_t value = 0;
    memory.read(reinterpret_cast<uint8_t*>(&value), pc, 3);
#else
    value = memory.read8(pc);
    value |= memory.read8(pc + 1) << 8;
    value |= memory.read8(pc + 2) << 16;
#endif
    return value;
}
