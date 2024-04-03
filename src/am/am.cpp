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

#include <am.h>
#include <fetch_decode.h>
#include <execute.h>
#include <exception.h>
#include <iostream>
#include <numeric>
#include <foundation/type/helper.h>
#include <foundation/logger.h>
#include <lib/utils.h>

using namespace cami;
using am::AbstractMachine;
using namespace am::spd;

AbstractMachine::ExitCode AbstractMachine::run()
{
    try {
        this->execute();
    } catch (const ObjectStorageOutOfMemoryException& e) {
        std::cerr << e.what() << std::endl;
        return ExitCode::abort;
    } catch (const IgnorableException& e) {
        std::cerr << e.what() << std::endl;
        return ExitCode::exception;
    }
    return ExitCode::halt;
}

void AbstractMachine::execute()
{
    while (true) {
        const auto [op, extraInfo] = FetchDecode::decode(*this);
//        log::unbuffered.dprintln("${}", op);
        switch (op) {
        case Opcode::nop:
            break;
        case Opcode::halt:
            if (this->operand_stack.getStack().empty()) {
                log::buffered.iprintln("Abstract machine halt with no return code");
                return;
            }
            {
                auto rv = this->operand_stack.pop();
                if (rv.attr.indeterminate) {
                    log::buffered.iprintln("Abstract machine halt with indeterminate value");
                    return;
                }
                if (!isInteger(rv.vb->getType().kind())) {
                    log::buffered.iprintln("Abstract machine halt with non-integer value ${}", rv.vb);
                    return;
                }
                auto val = rv.vb.get<IntegerValue>().uint64();
                log::buffered.iprintln("Abstract machine halt with return code ${}", *reinterpret_cast<int64_t*>(&val));
            }
            return;
        case Opcode::dsg:
            Execute::designate(*this, extraInfo);
            break;
        case Opcode::drf:
            Execute::dereference(*this);
            break;
        case Opcode::read:
            Execute::read(*this, extraInfo);
            break;
        case Opcode::mdf:
            Execute::modify(*this, extraInfo);
            break;
        case Opcode::zero:
            Execute::zero(*this, extraInfo);
            break;
        case Opcode::mdfi:
            Execute::writeInit(*this);
            break;
        case Opcode::zeroi:
            Execute::zeroInit(*this);
            break;
        case Opcode::eb:
            Execute::enterBlock(*this, extraInfo);
            break;
        case Opcode::lb:
            Execute::leaveBlock(*this);
            break;
        case Opcode::new_:
            Execute::newObject(*this, extraInfo);
            break;
        case Opcode::del:
            Execute::deleteObject(*this, extraInfo);
            break;
        case Opcode::fe:
            Execute::fullExpression(*this, extraInfo);
            break;
        case Opcode::j:
            Execute::jump(*this, extraInfo);
            break;
        case Opcode::jst:
            Execute::jumpIfSet(*this, extraInfo);
            break;
        case Opcode::jnt:
            Execute::jumpIfNotSet(*this, extraInfo);
            break;
        case Opcode::call:
            Execute::call(*this, extraInfo);
            break;
        case Opcode::ij:
            Execute::indirectJump(*this);
            break;
        case Opcode::ret:
            Execute::ret(*this);
            break;
        case Opcode::pushu:
            Execute::pushUndefined(*this);
            break;
        case Opcode::push:
            Execute::push(*this, extraInfo);
            break;
        case Opcode::pop:
            Execute::pop(*this);
            break;
        case Opcode::dup:
            Execute::duplicate(*this);
            break;
        case Opcode::dot:
            Execute::dot(*this, extraInfo);
            break;
        case Opcode::arrow:
            Execute::arrow(*this, extraInfo);
            break;
        case Opcode::addr:
            Execute::address(*this);
            break;
        case Opcode::cast:
            Execute::cast(*this, extraInfo);
            break;
        default:
            if (FetchDecode::isUnaryOperator(op)) {
                Execute::unaryOperator(*this, op);
            } else if (FetchDecode::isBinaryOperator(op)) {
                Execute::binaryOperator(*this, op);
            } else {
                throw InvalidOpcodeException{static_cast<uint8_t>(op)};
            }
        }
    }
}

Global AbstractMachine::initStaticInfo(tr::LinkedMBC& bytecode)
{
    lib::Array<Object*> static_objects(bytecode.static_objects.length());
    for (size_t i = 0; i < static_objects.length(); ++i) {
        auto& sod = bytecode.static_objects[i];
        static_objects[i] = this->object_manager.newPermanent(std::move(sod.name), *sod.type, sod.address);
    }
    return Global{std::move(static_objects), std::move(bytecode.constants), std::move(bytecode.types),
                  std::move(bytecode.functions)};
}

uint64_t AbstractMachine::countPermanentObject(tr::LinkedMBC& bytecode)
{
    return VirtualMemory::MMIO_OBJECT_NUM + std::accumulate(
            bytecode.static_objects.begin(), bytecode.static_objects.end(), 0,
            [](uint64_t val, const StaticObjectDescription& obj) {
                return val + ts::countCorrespondingObjectFamily(*obj.type);
            });
}

tr::LinkedMBC& AbstractMachine::preprocessBytecode(tr::LinkedMBC& bytecode)
{
    AbstractMachine::checkMetadataCnt(bytecode);
    lib::Array<uint8_t> data(bytecode.data.length() + bytecode.bss_size);
    std::memcpy(data.data(), bytecode.data.data(), bytecode.data.length());
    std::memset(data.data() + bytecode.data.length(), 0, bytecode.bss_size);
    std::map<std::string_view, uint64_t> address_map;
    bytecode.data.assign(std::move(data));
    for (auto& item: bytecode.static_objects) {
        item.address += layout::DATA_BASE;
        address_map.emplace(item.name, item.address);
    }
    for (auto& item: bytecode.functions) {
        item.address += layout::CODE_BASE;
        address_map.emplace(item.name, item.address);
    }
    for (const auto& [offset, symbol]: bytecode.data_relocate) {
        auto itr = address_map.find(symbol);
        if (itr == address_map.end()) {
            throw AMInitialFailedException{std::string{"cannot find symbol: "}.append(symbol)};
        }
        lib::write<8>(&bytecode.data[offset], lib::readU<8>(&bytecode.data[offset]) + itr->second);
    }
    return bytecode;
}

void AbstractMachine::checkMetadataCnt(tr::LinkedMBC& bytecode)
{
    if (bytecode.types.length() > InstrInfo::ID_MAX) {
        throw AMInitialFailedException{"Too many types"};
    }
    if (bytecode.constants.length() > InstrInfo::ID_MAX) {
        throw AMInitialFailedException{"Too many constants"};
    }
    if (bytecode.functions.length() > InstrInfo::FUNCTION_ID_MAX) {
        throw AMInitialFailedException{"Too many functions"};
    }
    if (bytecode.static_objects.length() > InstrInfo::STATIC_OBJECT_ID_MAX) {
        throw AMInitialFailedException{"Too many static_objects"};
    }
}
