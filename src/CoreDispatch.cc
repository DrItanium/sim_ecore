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
    if (instruction.isCTRLFormat()) {
        // CTRL Format opcodes
        (this->*ctrlFormatInstructions[instruction.getMajorOpcode()])(instruction);
    } else if (instruction.isCOBRFormat()) {
        // since these are separate tables, we need to do some offset calculation
        auto properOffset = instruction.getMajorOpcode() - 0x20;
        (this->*cobrFormatInstructions[properOffset])(instruction);
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
                // MEM Format
            case Opcode::ldob:
                setDestinationFromSrcDest(instruction, loadByte(computeMemoryAddress(instruction)), TreatAsOrdinal{});
                break;
            case Opcode::bx:
                bx(instruction);
                break;
            case Opcode::balx:
                balx(instruction);
                break;
            case Opcode::lda:
                lda(instruction);
                break;
            case Opcode::ldos:
                setDestinationFromSrcDest(instruction, loadShort(computeMemoryAddress(instruction)), TreatAsOrdinal{});
                break;
            case Opcode::ld:
                setDestinationFromSrcDest(instruction, load(computeMemoryAddress(instruction)), TreatAsOrdinal{});
                break;
            case Opcode::ldl:
                setDestinationFromSrcDest(instruction, loadLong(computeMemoryAddress(instruction)), TreatAsLongOrdinal{});
                break;
            case Opcode::ldt:
                load(computeMemoryAddress(instruction), destinationFromSrcDest(instruction, TreatAsTripleRegister{}));
                break;
            case Opcode::ldq:
                load(computeMemoryAddress(instruction), destinationFromSrcDest(instruction, TreatAsQuadRegister{}));
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
            case Opcode::ldib:
                setDestinationFromSrcDest(instruction, loadByte(computeMemoryAddress(instruction)), TreatAsInteger{});
                break;
            case Opcode::ldis:
                setDestinationFromSrcDest(instruction, loadShort(computeMemoryAddress(instruction)), TreatAsInteger{});
                break;
            case Opcode::st:
                store(computeMemoryAddress(instruction), valueFromSrcDestRegister<Ordinal>(instruction));
                break;
            case Opcode::stob:
                storeByte(computeMemoryAddress(instruction), valueFromSrcDestRegister<ByteOrdinal>(instruction));
                break;
            case Opcode::stos:
                storeShort(computeMemoryAddress(instruction), valueFromSrcDestRegister<ShortOrdinal>(instruction));
                break;
            case Opcode::stl:
                storeLong(computeMemoryAddress(instruction), valueFromSrcDestRegister<LongOrdinal>(instruction));
                break;
            case Opcode::stt:
                store(computeMemoryAddress(instruction), sourceFromSrcDest(instruction, TreatAsTripleRegister{}));
                break;
            case Opcode::stq:
                store(computeMemoryAddress(instruction), sourceFromSrcDest(instruction, TreatAsQuadRegister{}));
                break;
            case Opcode::stib:
                storeByteInteger(computeMemoryAddress(instruction), valueFromSrcDestRegister<ByteInteger>(instruction));
                break;
            case Opcode::stis:
                storeShortInteger(computeMemoryAddress(instruction), valueFromSrcDestRegister<ShortInteger>(instruction));
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
            case Opcode::ret:
                ret(instruction);
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

