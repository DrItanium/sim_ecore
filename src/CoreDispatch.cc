// sim_ecore
// Copyright (c) 2021-2022, Joshua Scoggins
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
void
Core::illegalInstruction(const Instruction &inst) noexcept {
    generateFault(FaultType::Operation_InvalidOpcode) ;
}
void
Core::stob(const Instruction &instruction) noexcept {
    storeByte(computeMemoryAddress(instruction), valueFromSrcDestRegister<ByteOrdinal>(instruction));
}
void
Core::stib(const Instruction &instruction) noexcept {
    storeByteInteger(computeMemoryAddress(instruction), valueFromSrcDestRegister<ByteInteger>(instruction));
}
void
Core::stis(const Instruction &instruction) noexcept {
    storeShortInteger(computeMemoryAddress(instruction), valueFromSrcDestRegister<ShortInteger>(instruction));
}

void
Core::st(const Instruction &instruction) noexcept {
    store(computeMemoryAddress(instruction), valueFromSrcDestRegister<Ordinal>(instruction));
}
void
Core::stos(const Instruction &instruction) noexcept {
    storeShort(computeMemoryAddress(instruction), valueFromSrcDestRegister<ShortOrdinal>(instruction));
}
void
Core::stl(const Instruction &instruction) noexcept {
    storeLong(computeMemoryAddress(instruction), valueFromSrcDestRegister<LongOrdinal>(instruction));
}
void
Core::stt(const Instruction &instruction) noexcept {
    store(computeMemoryAddress(instruction), sourceFromSrcDest(instruction, TreatAsTripleRegister{}));
}
void
Core::stq(const Instruction &instruction) noexcept {
    store(computeMemoryAddress(instruction), sourceFromSrcDest(instruction, TreatAsQuadRegister{}));
}
void
Core::ldis(const Instruction &instruction) noexcept {
    setDestinationFromSrcDest(instruction, loadShort(computeMemoryAddress(instruction)), TreatAsInteger{});
}
void
Core::ldib(const Instruction &instruction) noexcept {
    setDestinationFromSrcDest(instruction, loadByte(computeMemoryAddress(instruction)), TreatAsInteger{});
}
void
Core::ldob(const Instruction &instruction) noexcept {
    setDestinationFromSrcDest(instruction, loadByte(computeMemoryAddress(instruction)), TreatAsOrdinal{});
}
void
Core::ldos(const Instruction &inst) noexcept {
    setDestinationFromSrcDest(inst, loadShort(computeMemoryAddress(inst)), TreatAsOrdinal{});
}
void
Core::ld(const Instruction &inst) noexcept {
    setDestinationFromSrcDest(inst, load(computeMemoryAddress(inst)), TreatAsOrdinal{});
}
void
Core::ldl(const Instruction &inst) noexcept {
    setDestinationFromSrcDest(inst, loadLong(computeMemoryAddress(inst)), TreatAsLongOrdinal{});
}
void
Core::ldt(const Instruction &inst) noexcept {
    load(computeMemoryAddress(inst), destinationFromSrcDest(inst, TreatAsTripleRegister{}));
}
void
Core::ldq(const Instruction &inst) noexcept {
    load(computeMemoryAddress(inst), destinationFromSrcDest(inst, TreatAsQuadRegister{}));
}
void
Core::executeInstruction(const Instruction &instruction) noexcept {
    using TargetFunction = void (Core::*)(const Instruction& inst);
    static const TargetFunction ctrlFormatInstructions [] {
            &Core::illegalInstruction, // 0x00
        &Core::illegalInstruction, // 0x01
        &Core::illegalInstruction, // 0x02
        &Core::illegalInstruction, // 0x03
        &Core::illegalInstruction, // 0x04
        &Core::illegalInstruction, // 0x05,
        &Core::illegalInstruction, // 0x06,
        &Core::illegalInstruction, // 0x07,
        &Core::b, // 0x08,
        &Core::call, // 0x09
        &Core::ret, // 0x0a
        &Core::bal, // 0x0b
        &Core::illegalInstruction, // 0x0C
        &Core::illegalInstruction, // 0x0D
        &Core::illegalInstruction, // 0x0E
        &Core::illegalInstruction, // 0x0F
        // branch instruction opcodes are 0x100-0x170, we shift right by four and then mask the lowest three bits to get the mask
        // intel encoded the mask directly into the encoding :)
        &Core::condBranch, // bno
        &Core::condBranch, // bg
        &Core::condBranch, // be
        &Core::condBranch, // bge
        &Core::condBranch, // bl
        &Core::condBranch, // bne
        &Core::condBranch, // ble
        &Core::condBranch, // bo
        // like with the branch instructions, intel stashed the mask directly into the opcode itself
        // so do the same thing (except for 0b000 because that has a special meaning)
        &Core::condFault, // faultno
        &Core::condFault, // faultg
        &Core::condFault, // faulte
        &Core::condFault, // faultge
        &Core::condFault, // faultl
        &Core::condFault, // faultne
        &Core::condFault, // faultle
        &Core::condFault, // faulto
    };
    static constexpr byte COBRBaseOffset = 0x20;
    static const TargetFunction cobrFormatInstructions[] {
        // just like with fault and branch, test has the mask embedded in the opcode, so extract it out
        &Core::testOp, // testno
        &Core::testOp, // testg
        &Core::testOp, // teste
        &Core::testOp, // testge
        &Core::testOp, // testl
        &Core::testOp, // testne
        &Core::testOp, // testle
        &Core::testOp, // testo
        &Core::illegalInstruction, // 0x28
        &Core::illegalInstruction, // 0x29
        &Core::illegalInstruction, // 0x2A
        &Core::illegalInstruction, // 0x2B
        &Core::illegalInstruction, // 0x2C
        &Core::illegalInstruction, // 0x2D
        &Core::illegalInstruction, // 0x2E
        &Core::illegalInstruction, // 0x2F
        &Core::bbc, // branch if bit clear 0x30
            // just like with the others, intel encoded the mask for cmpo operations into the opcode itself
            // we can just extract it
        &Core::cmpobx, // cmpobg
        &Core::cmpobx, // cmpobe
        &Core::cmpobx, // cmpobge
        &Core::cmpobx, // cmpobl
        &Core::cmpobx, // cmpobne
        &Core::cmpobx, // cmpoble
        &Core::bbs, // 0x37, branch if bit set
            // just like with the others, intel encoded the mask for cmpi operations into the opcode itself
            // we can just extract it
        &Core::cmpibx, // cmpibno
        &Core::cmpibx, // cmpibg
        &Core::cmpibx, // cmpibe
        &Core::cmpibx, // cmpibge
        &Core::cmpibx, // cmpibl
        &Core::cmpibx, // cmpibne
        &Core::cmpibx, // cmpible
        &Core::cmpibx, // cmpibo
    };
    static constexpr byte MEMBaseOffset = 0x80;
    static const TargetFunction memFormatInstructions[] {
        &Core::ldob, // 0x80
        &Core::illegalInstruction, // 0x81
        &Core::stob, // 0x82
        &Core::illegalInstruction, // 0x83
        &Core::bx, // 0x84
        &Core::balx, // 0x85
        &Core::callx, // 0x86
        &Core::illegalInstruction, // 0x87
        &Core::ldos, // 0x88
        &Core::illegalInstruction, // 0x89
        &Core::stos, // 0x8a
        &Core::illegalInstruction, // 0x8b
        &Core::lda, // 0x8c
        &Core::illegalInstruction, // 0x8d
        &Core::illegalInstruction, // 0x8e
        &Core::illegalInstruction, // 0x8f
        &Core::ld, // 0x90
        &Core::illegalInstruction, // 0x91
        &Core::st, // 0x92
        &Core::illegalInstruction, // 0x93
        &Core::illegalInstruction, // 0x94
        &Core::illegalInstruction, // 0x95
        &Core::illegalInstruction, // 0x96
        &Core::illegalInstruction, // 0x97
        &Core::ldl, // 0x98
        &Core::illegalInstruction, // 0x99
        &Core::stl, // 0x9A
        &Core::illegalInstruction, // 0x9B
        &Core::illegalInstruction, // 0x9C
        &Core::illegalInstruction, // 0x9D
        &Core::illegalInstruction, // 0x9E
        &Core::illegalInstruction, // 0x9F
        &Core::ldt, // 0xA0
        &Core::illegalInstruction, // 0xA1
        &Core::stt, // 0xA2
        &Core::illegalInstruction, // 0xA3
        &Core::illegalInstruction, // 0xA4
        &Core::illegalInstruction, // 0xA5
        &Core::illegalInstruction, // 0xA6
        &Core::illegalInstruction, // 0xA7
        &Core::illegalInstruction, // 0xA8
        &Core::illegalInstruction, // 0xA9
        &Core::illegalInstruction, // 0xAA
        &Core::illegalInstruction, // 0xAB
        &Core::illegalInstruction, // 0xAC
        &Core::illegalInstruction, // 0xAD
        &Core::illegalInstruction, // 0xAE
        &Core::illegalInstruction, // 0xAF
        &Core::ldq, // 0xB0
        &Core::illegalInstruction, // 0xB1
        &Core::stq, // 0xB2
        &Core::illegalInstruction, // 0xB3
        &Core::illegalInstruction, // 0xB4
        &Core::illegalInstruction, // 0xB5
        &Core::illegalInstruction, // 0xB6
        &Core::illegalInstruction, // 0xB7
        &Core::illegalInstruction, // 0xB8
        &Core::illegalInstruction, // 0xB9
        &Core::illegalInstruction, // 0xBA
        &Core::illegalInstruction, // 0xBB
        &Core::illegalInstruction, // 0xBC
        &Core::illegalInstruction, // 0xBD
        &Core::illegalInstruction, // 0xBE
        &Core::illegalInstruction, // 0xBF
        &Core::ldib, // 0xC0
        &Core::illegalInstruction, // 0xC1
        &Core::stib, // 0xC2
        &Core::illegalInstruction, // 0xC3
        &Core::illegalInstruction, // 0xC4
        &Core::illegalInstruction, // 0xC5
        &Core::illegalInstruction, // 0xC6
        &Core::illegalInstruction, // 0xC7
        &Core::ldis, // 0xC8
        &Core::illegalInstruction, // 0xC9
        &Core::stis, // 0xCA
        &Core::illegalInstruction, // 0xCB
        &Core::illegalInstruction, // 0xCC
        &Core::illegalInstruction, // 0xCD
        &Core::illegalInstruction, // 0xCE
        &Core::illegalInstruction, // 0xCF
    };

    if (instruction.isCTRLFormat()) {
        // CTRL Format opcodes
        (this->*ctrlFormatInstructions[instruction.getMajorOpcode()])(instruction);
    } else if (instruction.isCOBRFormat()) {
        // since these are separate tables, we need to do some offset calculation
        auto properOffset = instruction.getMajorOpcode() - COBRBaseOffset;
        (this->*cobrFormatInstructions[properOffset])(instruction);
    } else if (instruction.isMEMFormat()) {
        auto properOffset = instruction.getMajorOpcode() - MEMBaseOffset;
        (this->*memFormatInstructions[properOffset])(instruction);
    } else {
        switch (instruction.identifyOpcode()) {
            case Opcode::cmpo:
                cmpo(instruction);
                break;
            case Opcode::cmpi:
                cmpi(instruction);
                break;
            case Opcode::cmpdeco:
                cmpdeco(instruction);
                break;
            case Opcode::cmpdeci:
                cmpdeci(instruction);
                break;
            case Opcode::cmpinco:
                cmpinco(instruction);
                break;
            case Opcode::cmpinci:
                cmpinci(instruction);
                break;
            case Opcode::concmpi:
                concmpGeneric(instruction, TreatAsInteger{});
                break;
            case Opcode::concmpo:
                concmpGeneric(instruction, TreatAsOrdinal{});
                break;
                // REG format
            case Opcode::addi:
                arithmeticGeneric<ArithmeticOperation::Add>(instruction, TreatAsInteger{});
                break;
            case Opcode::addo:
                arithmeticGeneric<ArithmeticOperation::Add>(instruction, TreatAsOrdinal{});
                break;
            case Opcode::subi:
                arithmeticGeneric<ArithmeticOperation::Subtract>(instruction, TreatAsInteger{});
                break;
            case Opcode::subo:
                arithmeticGeneric<ArithmeticOperation::Subtract>(instruction, TreatAsOrdinal{});
                break;
            case Opcode::muli:
                arithmeticGeneric<ArithmeticOperation::Multiply>(instruction, TreatAsInteger{});
                break;
            case Opcode::mulo:
                arithmeticGeneric<ArithmeticOperation::Multiply>(instruction, TreatAsOrdinal{});
                break;
            case Opcode::divo:
                arithmeticGeneric<ArithmeticOperation::Divide>(instruction, TreatAsOrdinal{});
                break;
            case Opcode::divi:
                arithmeticGeneric<ArithmeticOperation::Divide>(instruction, TreatAsInteger{});
                break;
            case Opcode::remi:
                arithmeticGeneric<ArithmeticOperation::Remainder>(instruction, TreatAsInteger{});
                break;
            case Opcode::remo:
                arithmeticGeneric<ArithmeticOperation::Remainder>(instruction, TreatAsOrdinal{});
                break;
            case Opcode::rotate:
                arithmeticGeneric<ArithmeticOperation::Rotate>(instruction, TreatAsOrdinal{});
                break;
            case Opcode::shli:
                arithmeticGeneric<ArithmeticOperation::ShiftLeft>(instruction, TreatAsInteger{});
                break;
            case Opcode::shri:
                arithmeticGeneric<ArithmeticOperation::ShiftRight>(instruction, TreatAsInteger{});
                break;
            case Opcode::notbit:
                notbit(instruction);
                break;
            case Opcode::logicalAnd:
                logicalOpGeneric<LogicalOp::And>(instruction);
                break;
            case Opcode::logicalOr:
                logicalOpGeneric<LogicalOp::Or>(instruction);
                break;
            case Opcode::logicalXor:
                logicalOpGeneric<LogicalOp::Xor>(instruction);
                break;
            case Opcode::logicalXnor:
                logicalOpGeneric<LogicalOp::Xnor>(instruction);
                break;
            case Opcode::logicalNor:
                logicalOpGeneric<LogicalOp::Nor>(instruction);
                break;
            case Opcode::logicalNand:
                logicalOpGeneric<LogicalOp::Nand>(instruction);
                break;
            case Opcode::logicalNot:
                logicalOpGeneric<LogicalOp::Not>(instruction);
                break;
            case Opcode::andnot:
                logicalOpGeneric<LogicalOp::AndNot>(instruction);
                break;
            case Opcode::notand:
                logicalOpGeneric<LogicalOp::NotAnd>(instruction);
                break;
            case Opcode::ornot:
                logicalOpGeneric<LogicalOp::OrNot>(instruction);
                break;
            case Opcode::notor:
                logicalOpGeneric<LogicalOp::NotOr>(instruction);
                break;
            case Opcode::mov:
                mov(instruction);
                break;
            case Opcode::movl:
                movl(instruction);
                break;
            case Opcode::movt:
                movt(instruction);
                break;
            case Opcode::movq:
                movq(instruction);
                break;
            case Opcode::alterbit:
                alterbit(instruction);
                break;
            case Opcode::ediv:
                ediv(instruction);
                break;
            case Opcode::emul:
                emul(instruction);
                break;
            case Opcode::extract:
                extract(instruction);
                break;
            case Opcode::flushreg:
                flushreg();
                break;
            case Opcode::fmark:
                fmark(instruction);
                break;
            case Opcode::mark:
                mark(instruction);
                break;
            case Opcode::modac:
                modac(instruction);
                break;
            case Opcode::modi:
                modi(instruction);
                break;
            case Opcode::modify:
                modify(instruction);
                break;
            case Opcode::call:
                call(instruction);
                break;
            case Opcode::callx:
                callx(instruction);
                break;
            case Opcode::shlo:
                shlo(instruction);
                break;
            case Opcode::shro:
                shro(instruction);
                break;
            case Opcode::scanbyte:
                scanbyte(instruction);
                break;
            case Opcode::scanbit:
                scanbit(instruction);
                break;
            case Opcode::spanbit:
                spanbit(instruction);
                break;
            case Opcode::syncf:
                syncf();
                break;
            case Opcode::atadd:
                atadd(instruction);
                break;
            case Opcode::atmod:
                atmod(instruction);
                break;
            case Opcode::chkbit:
                chkbit(instruction);
                break;
            case Opcode::addc:
                addc(instruction);
                break;
            case Opcode::subc:
                subc(instruction);
                break;
            case Opcode::shrdi:
                shrdi(instruction);
                break;
            case Opcode::synld:
                synld(instruction);
                break;
            case Opcode::synmov:
                synmov(instruction);
                break;
            case Opcode::synmovl:
                synmovl(instruction);
                break;
            case Opcode::synmovq:
                synmovq(instruction);
                break;
            case Opcode::modpc:
                modpc(instruction);
                break;
            case Opcode::modtc:
                modtc(instruction);
                break;
            case Opcode::setbit:
                setbit(instruction);
                break;
            case Opcode::clrbit:
                clrbit(instruction);
                break;
            case Opcode::calls:
                calls(instruction);
                break;
            case Opcode::bswap:
                bswap(instruction);
                break;
            default:
                illegalInstruction(instruction);
                break;
        }
    }
}

