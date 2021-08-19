// sim3
// Copyright (c) 2021, Joshua Scoggins
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "Core.h"
#include <iostream>
void
Core::syncf() noexcept {
    if (ac_.getNoImpreciseFaults()) {

    } else {
        /// @todo wait until no imprecise faults can occur associated with any uncompleted instructions
    }
}

void
Core::cycle() noexcept {
    advanceIPBy = 4;
    auto instruction = loadInstruction(ip_.getOrdinal());
    executeInstruction(instruction);
    //executeInstruction(loadInstruction(ip_.getOrdinal()));
    if (advanceIPBy > 0)  {
        ip_.setOrdinal(ip_.getOrdinal() + advanceIPBy);
    }
}

void
Core::cmpi(Integer src1, Integer src2) noexcept {
    if (src1 < src2) {
        ac_.setConditionCode(0b100);
    } else if (src1 == src2) {
        ac_.setConditionCode(0b010);
    } else {
        ac_.setConditionCode(0b001);
    }
}
void
Core::cmpo(Ordinal src1, Ordinal src2) noexcept {
    if (src1 < src2) {
        ac_.setConditionCode(0b100);
    } else if (src1 == src2) {
        ac_.setConditionCode(0b010);
    } else {
        ac_.setConditionCode(0b001);
    }
}

Register&
Core::getRegister(RegisterIndex targetIndex) {
    if (isLocalRegister(targetIndex)) {
        return locals.getRegister(static_cast<uint8_t>(targetIndex));
    } else if (isGlobalRegister(targetIndex)) {
        return globals.getRegister(static_cast<uint8_t>(targetIndex));
    } else if (isLiteral(targetIndex)) {
        throw "Literals cannot be modified";
    } else {
        throw "Illegal register requested";
    }
}

DoubleRegister&
Core::getDoubleRegister(RegisterIndex targetIndex) {
    if (isLocalRegister(targetIndex)) {
        return locals.getDoubleRegister(static_cast<int>(targetIndex));
    } else if (isGlobalRegister(targetIndex)) {
        return globals.getDoubleRegister(static_cast<int>(targetIndex));
    } else if (isLiteral(targetIndex)) {
        throw "Literals cannot be modified";
    } else {
        throw "Illegal register requested";
    }
}


TripleRegister&
Core::getTripleRegister(RegisterIndex targetIndex) {
    if (isLocalRegister(targetIndex)) {
        return locals.getTripleRegister(static_cast<int>(targetIndex));
    } else if (isGlobalRegister(targetIndex)) {
        return globals.getTripleRegister(static_cast<int>(targetIndex));
    } else if (isLiteral(targetIndex)) {
        throw "Literals cannot be modified";
    } else {
        throw "Illegal register requested";
    }
}

QuadRegister&
Core::getQuadRegister(RegisterIndex targetIndex) {
    if (isLocalRegister(targetIndex)) {
        return locals.getQuadRegister(static_cast<int>(targetIndex));
    } else if (isGlobalRegister(targetIndex)) {
        return globals.getQuadRegister(static_cast<int>(targetIndex));
    } else if (isLiteral(targetIndex)) {
        throw "Literals cannot be modified";
    } else {
        throw "Illegal register requested";
    }
}

const Register&
Core::getRegister(RegisterIndex targetIndex) const {
    if (isLocalRegister(targetIndex)) {
        return locals.getRegister(static_cast<uint8_t>(targetIndex));
    } else if (isGlobalRegister(targetIndex)) {
        return globals.getRegister(static_cast<uint8_t>(targetIndex));
    } else if (isLiteral(targetIndex)) {
        return OrdinalLiterals[static_cast<uint8_t>(targetIndex) & 0b11111];
    } else {
        throw "Illegal register requested";
    }
}

const DoubleRegister&
Core::getDoubleRegister(RegisterIndex targetIndex) const {
    if (isLocalRegister(targetIndex)) {
        return locals.getDoubleRegister(static_cast<uint8_t>(targetIndex));
    } else if (isGlobalRegister(targetIndex)) {
        return globals.getDoubleRegister(static_cast<uint8_t>(targetIndex));
    } else if (isLiteral(targetIndex)) {
        /// @todo implement double register literal support, according to the docs it is allowed
        return LongOrdinalLiterals[static_cast<uint8_t>(targetIndex) & 0b11111];
    } else {
        throw "Illegal register requested";
    }
}

const TripleRegister&
Core::getTripleRegister(RegisterIndex targetIndex) const {
    if (isLocalRegister(targetIndex)) {
        return locals.getTripleRegister(static_cast<uint8_t>(targetIndex));
    } else if (isGlobalRegister(targetIndex)) {
        return globals.getTripleRegister(static_cast<uint8_t>(targetIndex));
    } else if (isLiteral(targetIndex)) {
        /// @todo implement double register literal support, according to the docs it is allowed
        return TripleOrdinalLiterals[static_cast<uint8_t>(targetIndex) & 0b11111];
    } else {
        throw "Illegal register requested";
    }
}

const QuadRegister&
Core::getQuadRegister(RegisterIndex targetIndex) const {
    if (isLocalRegister(targetIndex)) {
        return locals.getQuadRegister(static_cast<uint8_t>(targetIndex));
    } else if (isGlobalRegister(targetIndex)) {
        return globals.getQuadRegister(static_cast<uint8_t>(targetIndex));
    } else if (isLiteral(targetIndex)) {
        /// @todo implement double register literal support, according to the docs it is allowed
        return QuadOrdinalLiterals[static_cast<uint8_t>(targetIndex) & 0b11111];
    } else {
        throw "Illegal register requested";
    }
}

Instruction
Core::loadInstruction(Address baseAddress) noexcept {
    // load words 64-bits at a time for simplicity, we increment by eight on double wide instructions and four on single wide
    auto targetAddress = baseAddress & ~(static_cast<Address>(0b11));
    auto theLong = loadLong(targetAddress);
    return Instruction(theLong);
}

void
Core::saveRegisterFrame(const RegisterFrame &theFrame, Address baseAddress) noexcept {
    for (int i = 0; i < 16; ++i, baseAddress += 4) {
        store(baseAddress, theFrame.getRegister(i).getOrdinal());
    }
}

void
Core::restoreRegisterFrame(RegisterFrame &theFrame, Address baseAddress) noexcept {
    for (auto& reg : theFrame.gprs) {
        reg.setOrdinal(load(baseAddress));
        baseAddress += 4;
    }
}

void
Core::generateFault(FaultType) {
    /// @todo implement this at some point
    // lookup fault information
    // setup fault data frame
    // call fault handler
    // probably should exit or something here
}
Ordinal
Core::computeMemoryAddress(const Instruction &instruction) noexcept {
    // assume we are looking at a correct style instruction :)
    if (instruction.isDoubleWide()) {
        // also make sure that we jump ahead by eight bytes instead of four
        advanceIPBy += 4;
    }
    switch (instruction.getMemFormatMode()) {
        case MEMFormatMode::MEMA_AbsoluteOffset:
            return instruction.getOffset();
        case MEMFormatMode::MEMA_RegisterIndirectWithOffset:
            return instruction.getOffset() + getSourceRegister(instruction.getABase()).getOrdinal();
        case MEMFormatMode::MEMB_RegisterIndirect:
            return getRegister(instruction.getABase()).getOrdinal();
        case MEMFormatMode::MEMB_RegisterIndirectWithIndex:
            return getSourceRegister(instruction.getABase()).getOrdinal() +
                   (getSourceRegister(instruction.getIndex()).getOrdinal() << instruction.getScale());
        case MEMFormatMode::MEMB_IPWithDisplacement:
            return static_cast<Ordinal>(ip_.getInteger() + instruction.getDisplacement() + 8);
        case MEMFormatMode::MEMB_AbsoluteDisplacement:
            return instruction.getDisplacement(); // this will return the optional displacement
        case MEMFormatMode::MEMB_RegisterIndirectWithDisplacement:
            return static_cast<Ordinal>(getRegister(instruction.getABase()).getInteger() + instruction.getDisplacement());
        case MEMFormatMode::MEMB_IndexWithDisplacement:
            return static_cast<Ordinal>((getRegister(instruction.getIndex()).getInteger() << instruction.getScale()) + instruction.getDisplacement());
        case MEMFormatMode::MEMB_RegisterIndirectWithIndexAndDisplacement:
            return static_cast<Ordinal>(
                    getRegister(instruction.getABase()).getInteger() +
                    (getRegister(instruction.getIndex()).getInteger() << instruction.getScale()) + instruction.getDisplacement());
        default:
            return -1;
    }
}
void
Core::executeInstruction(const Instruction &instruction) noexcept {
    std::cout << "IP: 0x" << std::hex << ip_.getOrdinal() << std::endl;
    static constexpr Ordinal bitPositions[32] {
#define X(base) 1u << (base + 0), 1u << (base + 1), 1u << (base + 2), 1u << (base + 3)
            X(0), X(4), X(8), X(12),
            X(16), X(20), X(24), X(28)
#undef X
    };
    auto cmpobx = [this, &instruction](uint8_t mask) noexcept {
        auto s1k = instruction.getSrc1();
        auto& sr1 = getSourceRegister(s1k);
        auto src1 = sr1.getOrdinal();
        auto src2 = getSourceRegister(instruction.getSrc2()).getOrdinal();
        cmpo(src1, src2);
        if ((mask & ac_.getConditionCode()) != 0) {
            // while the docs show (displacement * 4), I am currently including the bottom two bits being forced to zero in displacement
            // in the future (the HX uses those two bits as "S2" so that will be a fun future change...).
            // I do not know why the Sx manual shows adding four while the hx manual does not
            // because of this, I'm going to drop the +4  from both paths and only disable automatic incrementation if we are successful
            advanceIPBy = 0;
            auto destination = ip_.getInteger() + instruction.getDisplacement();
            ip_.setInteger(destination);
        }
    };
    auto cmpibx = [this, &instruction](uint8_t mask) noexcept {
        auto src1 = getSourceRegister(instruction.getSrc1()).getInteger();
        auto src2 = getSourceRegister(instruction.getSrc2()).getInteger();
        cmpi(src1, src2);
        if ((mask & ac_.getConditionCode()) != 0) {
            // while the docs show (displacement * 4), I am currently including the bottom two bits being forced to zero in displacement
            // in the future (the HX uses those two bits as "S2" so that will be a fun future change...)

            // I do not know why the Sx manual shows adding four while the hx manual does not
            // because of this, I'm going to drop the +4  from both paths and only disable automatic incrementation if we are successful
            // this will fix an off by four problem I'm currently encountering
            advanceIPBy = 0;
            ip_.setInteger(ip_.getInteger() + instruction.getDisplacement());
        }
    };
    auto condBranch = [this, &instruction](uint8_t mask) {
        if ((ac_.getConditionCode()& mask) != 0) {
            ipRelativeBranch(instruction.getDisplacement()) ;
        }
    };
    auto condFault = [this](uint8_t mask) {
        if ((ac_.getConditionCode()& mask) != 0) {
            generateFault(FaultType::Constraint_Range);
        }
    };
    switch (instruction.identifyOpcode()) {
        // CTRL Format opcodes
        case Opcode::b:
            ipRelativeBranch(instruction.getDisplacement()) ;
            break;
        case Opcode::bal:
            getRegister(RegisterIndex::Global14).setOrdinal(ip_.getOrdinal() + 4);
            ipRelativeBranch(instruction.getDisplacement()) ;
            break;
        case Opcode::bno:
            if (ac_.getConditionCode() == 0) {
                ipRelativeBranch(instruction.getDisplacement()) ;
            }
            break;
        case Opcode::bg:
            condBranch(0b001);
            break;
        case Opcode::be:
            condBranch(0b010);
            break;
        case Opcode::bge:
            condBranch(0b011);
            break;
        case Opcode::bl:
            condBranch(0b100);
            break;
        case Opcode::bne:
            condBranch(0b101);
            break;
        case Opcode::ble:
            condBranch(0b110);
            break;
        case Opcode::bo:
            condBranch(0b111);
            break;
        case Opcode::faultno:
            if (ac_.getConditionCode() == 0) {
                generateFault(FaultType::Constraint_Range);
            }
            break;
        case Opcode::faultg:
            condFault(0b001);
            break;
        case Opcode::faulte:
            condFault(0b010);
            break;
        case Opcode::faultge:
            condFault(0b011);
            break;
        case Opcode::faultl:
            condFault(0b100);
            break;
        case Opcode::faultne:
            condFault(0b101);
            break;
        case Opcode::faultle:
            condFault(0b110);
            break;
        case Opcode::faulto:
            condFault(0b111);
            break;
            // COBR Format
        case Opcode::testno:
            [this, &instruction]() {
                getRegister(instruction.getSrc1(true)).setOrdinal(ac_.conditionCodeIs<0b000>() ? 1 : 0);
            }();
            break;
        case Opcode::testg:
            [this, &instruction]() {
                getRegister(instruction.getSrc1(true)).setOrdinal(ac_.conditionCodeIs<0b001>() ? 1 : 0);

            }();
            break;
        case Opcode::teste:
            [this, &instruction]() {
                getRegister(instruction.getSrc1(true)).setOrdinal(ac_.conditionCodeIs<0b010>() ? 1 : 0);

            }();
            break;
        case Opcode::testge:
            [this, &instruction]() {
                getRegister(instruction.getSrc1(true)).setOrdinal(ac_.conditionCodeIs<0b011>() ? 1 : 0);

            }();
            break;
        case Opcode::testl:
            [this, &instruction]() {
                getRegister(instruction.getSrc1(true)).setOrdinal(ac_.conditionCodeIs<0b100>() ? 1 : 0);
            }();
            break;
        case Opcode::testne:
            [this, &instruction]() {
                getRegister(instruction.getSrc1(true)).setOrdinal(ac_.conditionCodeIs<0b101>() ? 1 : 0);
            }();
            break;
        case Opcode::testle:
            [this, &instruction]() {
                getRegister(instruction.getSrc1(true)).setOrdinal(ac_.conditionCodeIs<0b110>() ? 1 : 0);
            }();
            break;
        case Opcode::testo:
            [this, &instruction]() {
                getRegister(instruction.getSrc1(true)).setOrdinal(ac_.conditionCodeIs<0b111>() ? 1 : 0);
            }();
            break;
        case Opcode::bbc:
            // branch if bit is clear
            [this, &instruction]() {
                auto bitpos = bitPositions[getSourceRegister(instruction.getSrc1()).getOrdinal() & 0b11111];
                auto src = getSourceRegister(instruction.getSrc2()).getOrdinal();
                if ((bitpos & src) == 0) {
                    ac_.setConditionCode(0b010);
                    advanceIPBy = 0;
                    // while the docs show (displacement * 4), I am currently including the bottom two bits being forced to zero in displacement
                    // in the future (the HX uses those two bits as "S2" so that will be a fun future change...)
                    ip_.setInteger(ip_.getInteger() + instruction.getDisplacement());
                } else {
                    ac_.setConditionCode(0b000);
                }
            }();
            break;
        case Opcode::bbs:
            [this, &instruction]() {
                auto targetRegister = instruction.getSrc1();
                auto& bpReg = getSourceRegister(targetRegister);
                auto bpOrd = bpReg.getOrdinal();
                auto masked = bpOrd & 0b11111;
                auto srcIndex = instruction.getSrc2();
                const auto& srcReg = getSourceRegister(srcIndex);
                auto src = srcReg.getOrdinal();
                auto bitpos = bitPositions[masked];
                if ((bitpos & src) != 0) {
                    advanceIPBy = 0;
                    ac_.setConditionCode(0b010);
                    // while the docs show (displacement * 4), I am currently including the bottom two bits being forced to zero in displacement
                    // in the future (the HX uses those two bits as "S2" so that will be a fun future change...)
                    ip_.setInteger(ip_.getInteger() + instruction.getDisplacement());
                } else {
                    ac_.setConditionCode(0b000);
                }
            }();
            break;
        case Opcode::cmpo:
            [this, &instruction]() {
                cmpo(getSourceRegister(instruction.getSrc1()).getOrdinal(),
                     getSourceRegister(instruction.getSrc2()).getOrdinal());
            }();
            break;
        case Opcode::cmpi:
            [this, &instruction]() {
                cmpi(getSourceRegister(instruction.getSrc1()).getInteger(),
                     getSourceRegister(instruction.getSrc2()).getInteger());
            }();
            break;
        case Opcode::cmpdeco:
            [this, &instruction]() {
                auto src2 = getSourceRegister(instruction.getSrc2()).getOrdinal();
                cmpo(getSourceRegister(instruction.getSrc1()).getOrdinal(), src2);
                getRegister(instruction.getSrcDest(false)).setOrdinal(src2 - 1);
            }();
            break;
        case Opcode::cmpdeci:
            [this, &instruction]() {
                auto src2 = getSourceRegister(instruction.getSrc2()).getInteger();
                cmpi(getSourceRegister(instruction.getSrc1()).getInteger(), src2);
                getRegister(instruction.getSrcDest(false)).setInteger(src2 - 1);
            }();
            break;
        case Opcode::cmpinco:
            [this, &instruction]() {
                auto src2 = getSourceRegister(instruction.getSrc2()).getOrdinal();
                cmpo(getSourceRegister(instruction.getSrc1()).getOrdinal(), src2);
                getRegister(instruction.getSrcDest(false)).setOrdinal(src2 + 1);
            }();
            break;
        case Opcode::cmpinci:
            [this, &instruction]() {
                auto src2 = getSourceRegister(instruction.getSrc2()).getInteger();
                cmpi(getSourceRegister(instruction.getSrc1()).getInteger(), src2);
                getRegister(instruction.getSrcDest(false)).setInteger(src2 + 1);
            }();
            break;
        case Opcode::cmpobg:
            cmpobx(0b001);
            break;
        case Opcode::cmpobe:
            cmpobx(0b010);
            break;
        case Opcode::cmpobge:
            cmpobx(0b011);
            break;
        case Opcode::cmpobl:
            cmpobx(0b100);
            break;
        case Opcode::cmpobne:
            cmpobx(0b101);
            break;
        case Opcode::cmpoble:
            cmpobx(0b110);
            break;
        case Opcode::cmpibno:
            cmpibx(0b000);
            break;
        case Opcode::cmpibg:
            cmpibx(0b001);
            break;
        case Opcode::cmpibe:
            cmpibx(0b010);
            break;
        case Opcode::cmpibge:
            cmpibx(0b011);
            break;
        case Opcode::cmpibl:
            cmpibx(0b100);
            break;
        case Opcode::cmpibne:
            cmpibx(0b101);
            break;
        case Opcode::cmpible:
            cmpibx(0b110);
            break;
        case Opcode::cmpibo:
            cmpibx(0b111);
            break;
        case Opcode::concmpi:
            [this, &instruction]() {
                if ((ac_.getConditionCode() & 0b100) == 0) {
                    auto src1 = getRegister(instruction.getSrc1()).getInteger();
                    auto src2 = getRegister(instruction.getSrc2()).getInteger();
                    ac_.setConditionCode((src1 <= src2) ? 0b010 : 0b001);
                }
            }();
            break;
        case Opcode::concmpo:
            [this, &instruction]() {
                if ((ac_.getConditionCode() & 0b100) == 0) {
                    auto src1 = getRegister(instruction.getSrc1()).getOrdinal();
                    auto src2 = getRegister(instruction.getSrc2()).getOrdinal();
                    ac_.setConditionCode((src1 <= src2) ? 0b010 : 0b001);
                }
            }();
            break;
            // MEM Format
        case Opcode::ldob:
            [this, &instruction]() {
                auto& dest = getRegister(instruction.getSrcDest(false));
                auto address = computeMemoryAddress(instruction);
                auto result = loadByte(address);
                dest.setOrdinal(result);
            }();
            break;
        case Opcode::bx:
            [this, &instruction]() {
                auto memoryAddress = computeMemoryAddress(instruction);
                ip_.setOrdinal(memoryAddress);
                advanceIPBy = 0;
            }();
            break;
        case Opcode::balx:
            [this, &instruction]() {
                auto& g14 = getRegister(instruction.getSrcDest(false));
                auto address = computeMemoryAddress(instruction);
                g14.setOrdinal(ip_.getOrdinal() + advanceIPBy);
                ip_.setOrdinal(address);
                advanceIPBy = 0;
            }();
            break;
        case Opcode::ldos:
            [this, &instruction]() {
                auto& dest = getRegister(instruction.getSrcDest(false));
                dest.setOrdinal(loadShort(computeMemoryAddress(instruction)));
            }();
            break;
        case Opcode::lda:
            [this, &instruction]() {
                // compute the effective address (memory address) and store it in destination
                auto& dest = getRegister(instruction.getSrcDest(false));
                auto addr = computeMemoryAddress(instruction);
                dest.setOrdinal(addr);
            }();
            break;
        case Opcode::ld:
            [this, &instruction]() {
                auto& dest = getRegister(instruction.getSrcDest(false));
                auto address = computeMemoryAddress(instruction);
                auto result = load(address);
                dest.setOrdinal(result);
            }();
            break;
        case Opcode::ldl:
            [this, &instruction]() {
                auto& dest = getDoubleRegister(instruction.getSrcDest(false));
                dest.setLongOrdinal(loadLong(computeMemoryAddress(instruction)));
            }();
            break;
        case Opcode::ldt:
            [this, &instruction]() {
                load(computeMemoryAddress(instruction),
                     getTripleRegister(instruction.getSrcDest(false)));
            }();
            break;
        case Opcode::ldq:
            [this, &instruction]() {
                load(computeMemoryAddress(instruction),
                     getQuadRegister(instruction.getSrcDest(false)));
            }();
            break;
            // REG format
        case Opcode::addi:
            [this, &instruction]() {
                getRegister(instruction.getSrcDest(false)).setInteger(getSourceRegister(instruction.getSrc2()).getInteger() +
                                                                      getSourceRegister(instruction.getSrc1()).getInteger());
            }( );
            break;
        case Opcode::addo:
            [this, &instruction]() {
                getRegister(instruction.getSrcDest(false)).setOrdinal(getSourceRegister(instruction.getSrc2()).getOrdinal() +
                                                                      getSourceRegister(instruction.getSrc1()).getOrdinal());
            }();
            break;
        case Opcode::subi:
            [this, &instruction]() {
                getRegister(instruction.getSrcDest(false)).setInteger(
                        getSourceRegister(instruction.getSrc2()).getInteger() - getSourceRegister(instruction.getSrc1()).getInteger());
            }();
            break;
        case Opcode::subo:
            [this, &instruction]() {
                getRegister(instruction.getSrcDest(false)).setOrdinal(
                        getSourceRegister(instruction.getSrc2()).getOrdinal() - getSourceRegister(instruction.getSrc1()).getOrdinal());
            }();
            break;
        case Opcode::muli:
            [this, &instruction]() {
                getRegister(instruction.getSrcDest(false)).setInteger(
                        getSourceRegister(instruction.getSrc2()).getInteger() * getSourceRegister(instruction.getSrc1()).getInteger());
            }();
            break;
        case Opcode::mulo:
            [this, &instruction]() {
                getRegister(instruction.getSrcDest(false)).setOrdinal(
                        getSourceRegister(instruction.getSrc2()).getOrdinal() * getSourceRegister(instruction.getSrc1()).getOrdinal());
            }();
            break;
        case Opcode::divo:
            [this, &instruction]() {
                /// @todo check denominator and do proper handling
                getRegister(instruction.getSrcDest(false)).setOrdinal(
                        getSourceRegister(instruction.getSrc2()).getOrdinal() / getSourceRegister(instruction.getSrc1()).getOrdinal());
            }();
            break;
        case Opcode::divi:
            [this, &instruction]() {
                /// @todo check denominator and do proper handling
                getRegister(instruction.getSrcDest(false)).setInteger(
                        getSourceRegister(instruction.getSrc2()).getInteger() / getSourceRegister(instruction.getSrc1()).getInteger());
            }();
            break;
        case Opcode::notbit:
            [this, &instruction]() {
                auto bitpos = bitPositions[getSourceRegister(instruction.getSrc1()).getOrdinal() & 0b11111];
                auto src = getSourceRegister(instruction.getSrc2()).getOrdinal();
                auto& dest = getRegister(instruction.getSrcDest(false));
                dest.setOrdinal(src ^ bitpos);
            }();
            break;
        case Opcode::logicalAnd: [this, &instruction]() { getRegister(instruction.getSrcDest(false)).setOrdinal(getSourceRegister(instruction.getSrc2()).getOrdinal() & getSourceRegister(instruction.getSrc1()).getOrdinal()); }(); break;
        case Opcode::logicalOr: [this, &instruction]() { getRegister(instruction.getSrcDest(false)).setOrdinal(getSourceRegister(instruction.getSrc2()).getOrdinal() | getSourceRegister(instruction.getSrc1()).getOrdinal()); }(); break;
        case Opcode::logicalXor: [this, &instruction]() { getRegister(instruction.getSrcDest(false)).setOrdinal(getSourceRegister(instruction.getSrc2()).getOrdinal() ^ getSourceRegister(instruction.getSrc1()).getOrdinal()); }(); break;
        case Opcode::logicalXnor: [this, &instruction]() { getRegister(instruction.getSrcDest(false)).setOrdinal(~(getSourceRegister(instruction.getSrc2()).getOrdinal() ^ getSourceRegister(instruction.getSrc1()).getOrdinal())); }(); break;
        case Opcode::logicalNor: [this, &instruction]() { getRegister(instruction.getSrcDest(false)).setOrdinal(~(getSourceRegister(instruction.getSrc2()).getOrdinal() | getSourceRegister(instruction.getSrc1()).getOrdinal())); }(); break;
        case Opcode::logicalNand: [this, &instruction]() { getRegister(instruction.getSrcDest(false)).setOrdinal(~(getSourceRegister(instruction.getSrc2()).getOrdinal() & getSourceRegister(instruction.getSrc1()).getOrdinal())); }(); break;
        case Opcode::logicalNot: [this, &instruction]() { getRegister(instruction.getSrcDest(false)).setOrdinal(~(getSourceRegister(instruction.getSrc1()).getOrdinal())); }(); break;
        case Opcode::andnot: [this, &instruction]() { getRegister(instruction.getSrcDest(false)).setOrdinal(getSourceRegister(instruction.getSrc2()).getOrdinal() & ~getSourceRegister(instruction.getSrc1()).getOrdinal()); }(); break;
        case Opcode::notand: [this, &instruction]() { getRegister(instruction.getSrcDest(false)).setOrdinal(~getSourceRegister(instruction.getSrc2()).getOrdinal() & getSourceRegister(instruction.getSrc1()).getOrdinal()); }(); break;
        case Opcode::ornot: [this, &instruction]() { getRegister(instruction.getSrcDest(false)).setOrdinal(getRegister(instruction.getSrc2()).getOrdinal() | ~getRegister(instruction.getSrc1()).getOrdinal()); }(); break;
        case Opcode::notor: [this, &instruction]() { getRegister(instruction.getSrcDest(false)).setOrdinal(~getRegister(instruction.getSrc2()).getOrdinal() | getRegister(instruction.getSrc1()).getOrdinal()); }(); break;
        case Opcode::remi:
            [this, &instruction]() {
                auto& dest = getRegister(instruction.getSrcDest(false));
                auto src2 = getSourceRegister(instruction.getSrc2()).getInteger();
                auto src1 = getSourceRegister(instruction.getSrc1()).getInteger();
                // taken from the i960Sx manual
                dest.setInteger(src2 - ((src2 / src1) * src1));
            }();
            break;
        case Opcode::remo:
            [this, &instruction]() {
                auto& dest = getRegister(instruction.getSrcDest(false));
                auto src2 = getSourceRegister(instruction.getSrc2()).getOrdinal();
                auto src1 = getSourceRegister(instruction.getSrc1()).getOrdinal();
                // taken from the i960Sx manual
                dest.setOrdinal(src2 - ((src2 / src1) * src1));
            }();
            break;
        case Opcode::rotate:
            [this, &instruction]() {
                auto rotateOperation = [](Ordinal src, Ordinal length)  {
                    return (src << length)  | (src >> ((-length) & 31u));
                };
                auto& dest = getRegister(instruction.getSrcDest(false));
                auto src = getSourceRegister(instruction.getSrc2()).getOrdinal();
                auto len = getSourceRegister(instruction.getSrc1()).getOrdinal();
                dest.setOrdinal(rotateOperation(src, len));
            }();
            break;
        case Opcode::mov:
            [this, &instruction]() {
                auto& dest = getRegister(instruction.getSrcDest(false));
                auto srcValue = getSourceRegister(instruction.getSrc1()).getOrdinal();
                dest.setOrdinal(srcValue);
            }();
            break;
        case Opcode::movl:
            [this, &instruction]() {
                auto& dest = getDoubleRegister(instruction.getSrcDest(false));
                const auto& src = getSourceDoubleRegister(instruction.getSrc1());
                auto srcValue = src.getLongOrdinal();
                dest.setLongOrdinal(srcValue);
            }();
            break;
        case Opcode::movt:
            [this, &instruction]() {
                auto& dest = getTripleRegister(instruction.getSrcDest(false));
                const auto& src = getTripleRegister(instruction.getSrc1());
                dest.setOrdinal(src.getOrdinal(0), 0);
                dest.setOrdinal(src.getOrdinal(1), 1);
                dest.setOrdinal(src.getOrdinal(2), 2);
            }();
            break;
        case Opcode::movq:
            [this, &instruction]() {
                auto& dest = getQuadRegister(instruction.getSrcDest(false));
                const auto& src = getQuadRegister(instruction.getSrc1());
                dest.setOrdinal(src.getOrdinal(0), 0);
                dest.setOrdinal(src.getOrdinal(1), 1);
                dest.setOrdinal(src.getOrdinal(2), 2);
                dest.setOrdinal(src.getOrdinal(3), 3);
            }();
            break;
        case Opcode::alterbit:
            [this, &instruction]() {
                auto bitpos = bitPositions[getRegister(instruction.getSrc1()).getOrdinal() & 0b11111];
                auto src = getRegister(instruction.getSrc2()).getOrdinal();
                auto& dest = getRegister(instruction.getSrcDest(false));
                if (ac_.getConditionCode() & 0b010) {
                    dest.setOrdinal(src | bitpos);
                } else {
                    dest.setOrdinal(src & ~bitpos);
                }
            }();
            break;
        case Opcode::ediv:
            [this, &instruction]() {
                auto denomord = getRegister(instruction.getSrc1()).getOrdinal();
                if (denomord == 0) {
                    // raise an arithmetic zero divide fault
                    generateFault(FaultType::Arithmetic_ArithmeticZeroDivide);
                } else {
                    auto numerator = getDoubleRegister(instruction.getSrc2()).getLongOrdinal();
                    auto denominator = static_cast<LongOrdinal>(denomord);
                    auto& dest = getDoubleRegister(instruction.getSrcDest(false));
                    // taken from the manual
                    auto remainder = static_cast<Ordinal>(numerator - (numerator / denominator) * denominator);
                    auto quotient = static_cast<Ordinal>(numerator / denominator);
                    dest.setOrdinal(remainder, 0);
                    dest.setOrdinal(quotient, 1);
                }
            }();
            break;
        case Opcode::emul:
            [this, &instruction]() {
                auto src2 = static_cast<LongOrdinal>(getSourceRegister(instruction.getSrc2()).getOrdinal());
                auto src1 = static_cast<LongOrdinal>(getSourceRegister(instruction.getSrc2()).getOrdinal());
                auto& dest = getDoubleRegister(instruction.getSrcDest(false));
                // taken from the manual
                dest.setLongOrdinal(src2 * src1);
            }();
            break;
        case Opcode::extract:
            [this, &instruction]() {
                auto& dest = getRegister(instruction.getSrcDest(false));
                auto bitpos = getSourceRegister(instruction.getSrc1()).getOrdinal();
                auto len = getSourceRegister(instruction.getSrc2()).getOrdinal();
                // taken from the Hx manual as it isn't insane
                auto shiftAmount = bitpos > 32 ? 32 : bitpos;
                dest.setOrdinal((dest.getOrdinal() >> shiftAmount) & ~(0xFFFF'FFFF << len));
            }();
            break;
        case Opcode::flushreg:
            [this, &instruction]() {
                /// @todo expand this instruction to dump saved register sets to stack in the right places
                // currently this does nothing because I haven't implemented the register frame stack yet
                //saveRegisterFrame(locals, getFramePointer().getOrdinal());
            }();
            break;
        case Opcode::fmark:
            [this, &instruction]() {
                // Generates a breakpoint trace-event. This instruction causes a breakpoint trace-event to be generated, regardless of the
                // setting of the breakpoint trace mode flag (to be implemented), providing the trace-enable bit (bit 0) of the process
                // controls is set.

                // if pc.te == 1 then raiseFault(BreakpointTraceFault)
                /// @todo implement
                if (pc_.getTraceEnable()) {
                    generateFault(FaultType::Breakpoint_Trace); /// @todo raise trace breakpoint fault
                }
            }();
            break;
        case Opcode::mark:
            [this, &instruction]() {
                // Generates a breakpoint trace-event if the breakpoint trace mode has been enabled.
                // The breakpoint trace mode is enabled if the trace-enable bit (bit 0) of the process
                // controls and the breakpoint-trace mode bit (bit 7) of the trace controls have been zet
                if (pc_.getTraceEnable() && tc_.getBreakpointTraceMode()) {
                    generateFault(FaultType::Breakpoint_Trace); /// @todo raise trace breakpoint fault
                }
                // if pc.te == 1 && breakpoint_trace_flag then raise trace breakpoint fault
                /// @todo implement
            }();
            break;

        case Opcode::modac:
            [this, &instruction]() {
                auto& dest = getRegister(instruction.getSrcDest(false));
                auto mask = getSourceRegister(instruction.getSrc1()).getOrdinal();
                auto src = getSourceRegister(instruction.getSrc2()).getOrdinal();
                dest.setOrdinal(ac_.modify(mask, src));
            }( );
            break;
        case Opcode::modi:
            [this, &instruction]() {
                auto denominator = getSourceRegister(instruction.getSrc1()) .getInteger();
                if (denominator == 0) {
                    generateFault(FaultType::Arithmetic_ArithmeticZeroDivide);
                } else {
                    auto numerator = getSourceRegister(instruction.getSrc2()).getInteger();
                    auto& dest = getRegister(instruction.getSrcDest(false));
                    auto result = numerator - ((numerator / denominator) * denominator);
                    if (((numerator * denominator) < 0) && (result != 0)) {
                        result += denominator;
                    }
                    dest.setInteger(result);
                }
            }();
            break;
        case Opcode::modify:
            [this, &instruction]() {
                // this is my encode operation but expanded out
                auto& dest = getRegister(instruction.getSrcDest(false));
                auto mask = getRegister(instruction.getSrc1()).getOrdinal();
                auto src = getRegister(instruction.getSrc2()).getOrdinal();
                dest.setOrdinal((src & mask) | (dest.getOrdinal() & ~mask));
            }();
            break;
        case Opcode::call:
            [this, &instruction]() {
                /// @todo implement
                // wait for any uncompleted instructions to finish
                auto spPos = getStackPointer().getOrdinal();
                auto temp = (spPos + c_) & ~c_; // round to next boundary
                auto fp = getFramePointer().getOrdinal();
                getRIP().setOrdinal(ip_.getOrdinal() + advanceIPBy);
                /// @todo implement support for caching register frames
                // okay we have to properly mask out the frame pointer address like we do for ret
                auto targetAddress = fp & (~c_);
                saveRegisterFrame(locals, targetAddress);
                ip_.setInteger(ip_.getInteger() + instruction.getDisplacement());
                /// @todo expand pfp and fp to accurately model how this works
                getPFP().setOrdinal(fp);
                getFramePointer().setOrdinal(temp);
                getStackPointer().setOrdinal(temp + 64);
                advanceIPBy = 0; // we already know where we are going so do not jump ahead
            }();
            break;
        case Opcode::callx:
            [this, &instruction]() {
                // wait for any uncompleted instructions to finish
                auto temp = (getStackPointer().getOrdinal() + c_) & ~c_; // round to next boundary
                auto fp = getFramePointer().getOrdinal();
                auto memAddr = computeMemoryAddress(instruction);
                auto rip = ip_.getOrdinal() + advanceIPBy;
                getRIP().setOrdinal(rip); // we need to save the result correctly
                /// @todo implement support for caching register frames
                auto targetAddress = fp & (~c_);
                saveRegisterFrame(locals, targetAddress);
                ip_.setOrdinal(memAddr);
                getPFP().setOrdinal(fp);
                getFramePointer().setOrdinal(temp);
                getStackPointer().setOrdinal(temp + 64);
                advanceIPBy = 0; // we already know where we are going so do not jump ahead
            }();
            break;
        case Opcode::shlo:
            [this, &instruction]() {
                auto& dest = getRegister(instruction.getSrcDest(false));
                auto len = getSourceRegister(instruction.getSrc1()).getOrdinal();
                if (len < 32) {
                    auto src = getSourceRegister(instruction.getSrc2()).getOrdinal();
                    dest.setOrdinal(src << len);
                } else {
                    dest.setOrdinal(0);
                }
            }();
            break;
        case Opcode::shro:
            [this, &instruction]() {
                auto& dest = getRegister(instruction.getSrcDest(false));
                auto len = getSourceRegister(instruction.getSrc1()).getOrdinal();
                /// @todo implement "speed" optimization by only getting src if we need it
                auto src = getSourceRegister(instruction.getSrc2()).getOrdinal();
                dest.setOrdinal(src >> len);
            }();
            break;
        case Opcode::shli:
            [this, &instruction]() {
                auto& dest = getRegister(instruction.getSrcDest(false));
                auto len = getSourceRegister(instruction.getSrc1()).getInteger();
                auto src = getSourceRegister(instruction.getSrc2()).getInteger();
                dest.setInteger(src << len);
            }();
            break;
        case Opcode::scanbyte:
            [this, &instruction]() {
                auto& src1 = getRegister(instruction.getSrc1());
                auto& src2 = getRegister(instruction.getSrc2());
                auto bytesEqual = [&src1, &src2](int which) constexpr { return src1.getByteOrdinal(which) == src2.getByteOrdinal(which); };
                ac_.setConditionCode((bytesEqual(0) || bytesEqual(1) || bytesEqual(2) || bytesEqual(3)) ? 0b010 : 0b000);
            }();
            break;
        case Opcode::scanbit:
            [this, &instruction]() {
                constexpr Ordinal reverseBitPositions[32] {
#define X(base) 1u << (base + 3), 1u << (base + 2), 1u << (base + 1), 1u << (base + 0)
                        X(28), X(24), X(20), X(16),
                        X(12), X(8), X(4), X(0),
#undef X
                };
                // perform a sanity check
                static_assert(reverseBitPositions[0] == (1u << 31));
                auto& dest = getRegister(instruction.getSrcDest(false));
                auto src = getSourceRegister(instruction.getSrc1()).getOrdinal();
                dest.setOrdinal(0xFFFF'FFFF);
                ac_.setConditionCode(0);
                Ordinal index = 31;
                for (auto mask : reverseBitPositions) {
                    if ((src & mask) != 0) {
                        dest.setOrdinal(index);
                        ac_.setConditionCode(0b010);
                        return;
                    }
                    --index;
                }
            }();
            break;
        case Opcode::spanbit:
            [this, &instruction]() {
                constexpr Ordinal reverseBitPositions[32] {
#define X(base) 1u << (base + 3), 1u << (base + 2), 1u << (base + 1), 1u << (base + 0)
                        X(28), X(24), X(20), X(16),
                        X(12), X(8), X(4), X(0),
#undef X
                };
                // perform a sanity check
                static_assert(reverseBitPositions[0] == (1u << 31));
                auto& dest = getRegister(instruction.getSrcDest(false));
                auto src = getSourceRegister(instruction.getSrc1()).getOrdinal();
                dest.setOrdinal(0xFFFF'FFFF);
                ac_.setConditionCode(0);
                Ordinal index = 31;
                for (auto mask : reverseBitPositions) {
                    if ((src & mask) == 0) {
                        dest.setOrdinal(index);
                        ac_.setConditionCode(0b010);
                        return;
                    }
                    --index;
                }
            }();
            break;
        case Opcode::syncf:
            syncf();
            break;
        case Opcode::atadd:
            [this, &instruction]() {
                // adds the src (src2 internally) value to the value in memory location specified with the addr (src1 in this case) operand.
                // The initial value from memory is stored in dst (internally src/dst).
                syncf();
                auto addr = getSourceRegister(instruction.getSrc1()).getOrdinal() & 0xFFFF'FFFC; // force alignment to word boundary
                auto temp = atomicLoad(addr);
                auto src = getSourceRegister(instruction.getSrc2()).getOrdinal();
                atomicStore(addr, temp + src);
                getRegister(instruction.getSrcDest(false)).setOrdinal(temp);
            }();
            break;
        case Opcode::atmod:
            [this, &instruction]() {
                // copies the src/dest value (logical version) into the memory location specifeid by src1.
                // The bits set in the mask (src2) operand select the bits to be modified in memory. The initial
                // value from memory is stored in src/dest
                syncf();
                auto addr = getSourceRegister(instruction.getSrc1()).getOrdinal() & 0xFFFF'FFFC; // force alignment to word boundary
                auto temp = atomicLoad(addr);
                auto& dest = getRegister(instruction.getSrcDest(false));
                auto mask = getSourceRegister(instruction.getSrc2()).getOrdinal();
                atomicStore(addr, (dest.getOrdinal() & mask) | (temp & ~mask));
                dest.setOrdinal(temp);
            }();
            break;
        case Opcode::chkbit:
            [this, &instruction]() {
                auto src = getSourceRegister(instruction.getSrc2()).getOrdinal();
                auto bitpos = bitPositions[getSourceRegister(instruction.getSrc1()).getOrdinal() & 0b11111];
                ac_.setConditionCode((src & bitpos) == 0 ? 0b000 : 0b010);
            }();
            break;
        case Opcode::addc:
            [this, &instruction]() {
                auto& src1 = getSourceRegister(instruction.getSrc1());
                auto& src2 = getSourceRegister(instruction.getSrc2());
                union {
                    LongOrdinal value = 0;
                    Ordinal halves[2];
                } result;
                result.value = static_cast<LongOrdinal>(src2.getOrdinal()) + static_cast<LongOrdinal>(src1.getOrdinal()) + (ac_.getCarryBit() ? 1 : 0);
                // the result will be larger than 32-bits so we have to keep that in mind
                auto& dest = getRegister(instruction.getSrcDest(false));
                dest.setOrdinal(result.halves[0]);
                // do computation here
                ac_.setConditionCode(0);
                if ((src2.getMostSignificantBit() == src1.getMostSignificantBit()) && (src2.getMostSignificantBit() != dest.getMostSignificantBit())) {
                    // set the overflow bit in ac
                    ac_.setOverflowBit(1);
                }
                ac_.setCarryBit(result.halves[1] != 0);

                // set the carry out bit
            }();
            break;
        case Opcode::subc:
            [this, &instruction]() {
                auto& src1 = getSourceRegister(instruction.getSrc1());
                auto& src2 = getSourceRegister(instruction.getSrc2());
                union {
                    LongOrdinal value = 0;
                    Ordinal halves[2];
                } result;
                result.value = static_cast<LongOrdinal>(src2.getOrdinal()) - static_cast<LongOrdinal>(src1.getOrdinal()) - 1 + (ac_.getCarryBit() ? 1 : 0);
                // the result will be larger than 32-bits so we have to keep that in mind
                auto& dest = getRegister(instruction.getSrcDest(false));
                dest.setOrdinal(result.halves[0]);
                // do computation here
                ac_.setConditionCode(0);
                if ((src2.getMostSignificantBit() == src1.getMostSignificantBit()) && (src2.getMostSignificantBit() != dest.getMostSignificantBit())) {
                    // set the overflow bit in ac
                    ac_.setOverflowBit(1);
                }
                ac_.setCarryBit(result.halves[1] != 0);
                // set the carry out bit
            }();
            break;
        case Opcode::ldib:
            [this, &instruction]() {
                auto& dest = getRegister(instruction.getSrcDest(false));
                dest.setInteger(loadByte(computeMemoryAddress(instruction)));
            }();
            break;
        case Opcode::ldis:
            [this, &instruction]() {
                auto& dest = getRegister(instruction.getSrcDest(false));
                dest.setInteger(loadShort(computeMemoryAddress(instruction)));
            }();
            break;
        case Opcode::st:
            [this, &instruction]() {
                auto addr = computeMemoryAddress(instruction);
                auto src = getSourceRegister(instruction.getSrcDest(true)).getOrdinal();
                store(addr, src);
            }();
            break;
        case Opcode::stob:
            [this, &instruction]() {
                auto addr = computeMemoryAddress(instruction);
                auto theIndex = instruction.getSrcDest(true);
                auto& srcReg = getSourceRegister(theIndex);
                auto src = srcReg.getByteOrdinal(0);
                storeByte(addr, src);
            }();
            break;
        case Opcode::stos:
            [this, &instruction]() {
                auto src = getSourceRegister(instruction.getSrcDest(true)).getShortOrdinal();
                storeShort(computeMemoryAddress(instruction), src);
            }();
            break;
        case Opcode::stl:
            [this, &instruction]() {
                auto src = getDoubleRegister(instruction.getSrcDest(true)).getLongOrdinal();
                storeLong(computeMemoryAddress(instruction), src);
            }();
            break;
        case Opcode::stt:
            [this, &instruction]() {
                auto& src = getTripleRegister(instruction.getSrcDest(true));
                store(computeMemoryAddress(instruction), src);
            }();
            break;
        case Opcode::stq:
            [this, &instruction]() {
                auto& src = getQuadRegister(instruction.getSrcDest(true));
                store(computeMemoryAddress(instruction), src);
            }();
            break;
        case Opcode::stib:
            [this, &instruction]() {
                auto src = getSourceRegister(instruction.getSrcDest(true)).getInteger();
                storeByteInteger(computeMemoryAddress(instruction), static_cast<ByteInteger>(src));
            }();
            break;
        case Opcode::stis:
            [this, &instruction]() {
                auto src = getSourceRegister(instruction.getSrcDest(true)).getInteger();
                storeShortInteger(computeMemoryAddress(instruction), static_cast<ShortInteger>(src));
            }();
            break;
        case Opcode::shri:
            [this, &instruction]() {
                /*
                 * if (src >= 0) {
                 *  if (len < 32) {
                 *      dest <- src/2^len
                 *  } else {
                 *      dest <- 0
                 *  }
                 * }else {
                 *  if (len < 32) {
                 *      dest <- (src - 2^len + 1)/2^len;
                 *  } else {
                 *      dest <- -1;
                 *   }
                 * }
                 *
                 */
                auto& dest = getRegister(instruction.getSrcDest(false));
                auto src = getSourceRegister(instruction.getSrc2()).getInteger();
                auto len = getSourceRegister(instruction.getSrc1()).getInteger();
                /// @todo perhaps implement the extra logic if necessary
                dest.setInteger(src >> len);
            }();
            break;
        case Opcode::shrdi:
            [this, &instruction]() {
                static constexpr Integer bitPositions[32] {
#define X(base) 1 << (base + 0), 1 << (base + 1), 1 << (base + 2), 1 << (base + 3)
                        X(0), X(4), X(8), X(12),
                        X(16), X(20), X(24), X(28)
#undef X
                };
                // according to the manual, equivalent to divi value, 2 so that is what we're going to do for correctness sake
                auto& dest = getRegister(instruction.getSrcDest(false));
                auto src = getSourceRegister(instruction.getSrc2()).getInteger();
                auto len = getSourceRegister(instruction.getSrc1()).getInteger();
                if (len < 32) {
                    dest.setInteger(src / bitPositions[len]);
                } else {
                    dest.setInteger(0);
                }
            }();
            break;
        case Opcode::synld:
            [this, &instruction]() {
                // wait until another execution unit sets the condition codes to continue after requesting a load.
                // In the case of this emulator, it really doesn't mean anything but I can see this being a synld followed by a wait
                // for synchronization. It also allows access to internal memory mapped items.
                // So I'm not sure how to implement this yet, however I think at this point I'm just going to treat is as a special kind of load
                // with condition code assignments and forced alignments

                auto address = getSourceRegister(instruction.getSrc1()).getOrdinal() & 0xFFFF'FFFC; // force word alignment
                auto& dest = getRegister(instruction.getSrcDest(false));
                // load basically takes care of accessing different registers and such even memory mapped ones
                dest.setOrdinal(load(address));
                // there is a _fail_ condition where a bad access condition will result in 0b000
                /// @todo implement support for bad access conditions
                ac_.setConditionCode(0b010);
            }();
            break;
        case Opcode::synmov:
            [this, &instruction]() {
                // load from memory and then store to another address in a synchronous fashion
                auto src = getSourceRegister(instruction.getSrc2()).getOrdinal(); // source address
                auto addr = getSourceRegister(instruction.getSrc1()).getOrdinal() & 0xFFFF'FFFC; // align
                Register temp(load(src));
                synchronizedStore(addr, temp);
                /// @todo figure out how to support bad access conditions
                ac_.setConditionCode(0b010);
            }();
            break;
        case Opcode::synmovl:
            [this, &instruction]() {
                auto src = getSourceRegister(instruction.getSrc2()).getOrdinal(); // source address
                auto addr = getSourceRegister(instruction.getSrc1()).getOrdinal() & 0xFFFF'FFF8; // align
                DoubleRegister temp(loadLong(src));
                synchronizedStore(addr, temp);
                /// @todo figure out how to support bad access conditions
                ac_.setConditionCode(0b010);
            }();
            break;
        case Opcode::synmovq:
            [this, &instruction]() {
                auto src  = getSourceRegister(instruction.getSrc2()).getOrdinal(); // source address
                auto addr = getSourceRegister(instruction.getSrc1()).getOrdinal() & 0xFFFF'FFF0; // align
                QuadRegister temp = loadQuad(src);
                synchronizedStore(addr, temp);
                /// @todo figure out how to support bad access conditions
                ac_.setConditionCode(0b010);
            }();
            break;
        case Opcode::modpc:
            [this, &instruction]() {
                auto mask = getSourceRegister(instruction.getSrc1()).getOrdinal();
                auto& dest = getRegister(instruction.getSrcDest(false));
                if (mask != 0) {
                    if (!pc_.inSupervisorMode()) {
                        generateFault(FaultType::Type_Mismatch); /// @todo TYPE.MISMATCH
                    } else {
                        auto src = getSourceRegister(instruction.getSrc2()).getOrdinal();
                        dest.setOrdinal(pc_.modify(mask, src));
                        ProcessControls tmp(dest.getOrdinal());
                        if (tmp.getPriority() > pc_.getPriority()) {
                            /// @todo check for pending interrupts
                        }
                    }
                } else {
                    dest.setOrdinal(pc_.getValue());
                }
            }( );
            break;
        case Opcode::modtc:
            [this, &instruction]() {
                auto& dest = getRegister(instruction.getSrcDest(false));
                auto mask = getSourceRegister(instruction.getSrc1()).getOrdinal();
                auto src = getSourceRegister(instruction.getSrc2()).getOrdinal();
                dest.setOrdinal(tc_.modify(mask, src));
            }( );
            break;
        case Opcode::setbit:
            [this, &instruction]() {
                auto& dest = getRegister(instruction.getSrcDest(false));
                auto bitpos = bitPositions[getSourceRegister(instruction.getSrc1()).getOrdinal() & 0b11111];
                auto src = getSourceRegister(instruction.getSrc2()).getOrdinal();
                dest.setOrdinal(src | bitpos);
            }();
            break;
        case Opcode::clrbit:
            [this, &instruction]() {
                auto& dest = getRegister(instruction.getSrcDest(false));
                auto bitpos = bitPositions[getSourceRegister(instruction.getSrc1()).getOrdinal() & 0b11111];
                auto src = getSourceRegister(instruction.getSrc2()).getOrdinal();
                dest.setOrdinal(src & ~bitpos);
            }();
            break;
        case Opcode::calls:
            [this, &instruction]() {
                auto targ = getSourceRegister(instruction.getSrc1()).getOrdinal();
                if (targ > 259) {
                    generateFault(FaultType::Protection_Length);
                } else {
                    syncf();
                    auto tempPE = load(getSystemProcedureTableBase() + 48 + (4 * targ));
                    auto type = tempPE & 0b11;
                    auto procedureAddress = tempPE & ~0b11;
                    // read entry from system-procedure table, where sptbase is address of system-procedure table from IMI
                    getRegister(RegisterIndex::RIP).setOrdinal(ip_.getOrdinal());
                    ip_.setOrdinal(procedureAddress);
                    Ordinal temp = 0;
                    Ordinal tempRRR = 0;
                    if ((type == 0b00) || pc_.inSupervisorMode()) {
                        temp = (getStackPointer().getOrdinal() + c_) & ~c_;
                        tempRRR = 0;
                    } else {
                        temp = getSupervisorStackPointer();
                        tempRRR = 0b010 | (pc_.getTraceEnable() ? 0b001 : 0);
                        pc_.setExecutionMode(true);
                        pc_.setTraceEnable(temp & 0b1);
                    }
                    auto frameAddress = getFramePointer().getOrdinal() & (~c_); // make sure the lowest n bits are ignored
                    /// @todo implement support for caching register frames
                    saveRegisterFrame(locals, frameAddress);
                    /// @todo expand pfp and fp to accurately model how this works
                    PreviousFramePointer pfp(getPFP());
                    pfp.setAddress(getFramePointer().getOrdinal());
                    pfp.setReturnType(tempRRR);
                    getFramePointer().setOrdinal(temp);
                    getStackPointer().setOrdinal(temp + 64);
                    // we do not want to jump ahead on calls
                    advanceIPBy = 0;
                }
            }();
            break;
        case Opcode::ret:
            [this]() {
                syncf();
                PreviousFramePointer pfp(getPFP());
                auto restoreStandardFrame = [this]() {
                    getFramePointer().setOrdinal(getPFP().getOrdinal());
                    /// @todo implement local register frame stack
                    // we have to remember that a given number of bits needs to be ignored when dealing with the frame pointer
                    // we have to use the "c_" parameter for this
                    auto actualAddress = getFramePointer().getOrdinal() & (~c_);
                    restoreRegisterFrame(locals, actualAddress);
                    auto returnValue = getRIP().getOrdinal();
                    ip_.setOrdinal(returnValue);
                    advanceIPBy = 0; // we already computed ahead of time where we will return to
                };
                switch (pfp.getReturnType()) {
                    case 0b000:
                        restoreStandardFrame();
                        break;
                    case 0b001:
                        [this, &restoreStandardFrame]() {
                            auto& fp = getFramePointer();
                            auto x = load(fp.getOrdinal() - 16);
                            auto y = load(fp.getOrdinal() - 12);
                            restoreStandardFrame();
                            ac_.setValue(y);
                            if (pc_.inSupervisorMode()) {
                                pc_.setValue(x);
                            }
                        }();
                        break;
                    case 0b010:
                        [this, &restoreStandardFrame]() {
                            if (pc_.inSupervisorMode()) {
                                pc_.setTraceEnable(false);
                                pc_.setExecutionMode(false);
                            }
                            restoreStandardFrame();
                        }();
                        break;
                    case 0b011:
                        [this, &restoreStandardFrame]() {
                            if (pc_.inSupervisorMode())  {
                                pc_.setTraceEnable(true);
                                pc_.setExecutionMode(false);
                            }
                            restoreStandardFrame();
                        }();
                        break;
                    case 0b111: // interrupt return
                        [this,&restoreStandardFrame]() {
                            auto& fp = getFramePointer();
                            auto x = load(fp.getOrdinal() - 16);
                            auto y = load(fp.getOrdinal() - 12);
                            restoreStandardFrame();
                            ac_.setValue(y);
                            if (pc_.inSupervisorMode()) {
                                pc_.setValue(x);
                                /// @todo check_pending_interrupts
                            }
                        }();
                        break;
                    default:
                        // undefined
                        break;
                }
            }();
            break;
        default:
            generateFault(FaultType::Operation_InvalidOpcode);
            break;
    }
}
void
Core::run() {
    resetExecutionStatus();
    boot();
    while(continueToExecute()) {
        cycle();
    }
}
Ordinal
Core::getSystemProcedureTableBase() {
    return load(getSystemAddressTableBase() + 120);
}
Ordinal
Core::getFaultProcedureTableBase() {
    return load(getSystemAddressTableBase() + 152);

}
Ordinal
Core::getTraceTablePointer() {
    return load(getSystemAddressTableBase() + 168);
}
Ordinal
Core::getInterruptTableBase() {
    return load(getPRCBPtrBase() + 16);
}
Ordinal
Core::getFaultTableBase() {
    return load(getPRCBPtrBase() + 40);
}

Ordinal
Core::getInterruptStackPointer() {
   return load(getPRCBPtrBase() + 20);
}