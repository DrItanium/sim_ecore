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
Core::executeInstruction(const Instruction &instruction) noexcept {
    switch (instruction.identifyOpcode()) {
        // CTRL Format opcodes
        case Opcode::b:
            ipRelativeBranch(instruction);
            break;
        case Opcode::bal:
            setDestination(RegisterIndex::Global14, ip_.getOrdinal() + 4, TreatAsOrdinal{});
            ipRelativeBranch(instruction);
            break;
            // branch instruction opcodes are 0x100-0x170, we shift right by four and then mask the lowest three bits to get the mask
            // intel encoded the mask directly into the encoding :)
        case Opcode::bno:
            if (ac_.getConditionCode() == 0) {
                ipRelativeBranch(instruction);
            }
            break;
        case Opcode::bg:
        case Opcode::be:
        case Opcode::bge:
        case Opcode::bl:
        case Opcode::bne:
        case Opcode::ble:
        case Opcode::bo:
            condBranch(instruction);
            break;
        case Opcode::faultno:
            if (ac_.getConditionCode() == 0) {
                generateFault(FaultType::Constraint_Range);
            }
            break;
            // like with the branch instructions, intel stashed the mask directly into the opcode itself
            // so do the same thing (except for 0b000 because that has a special meaning)
        case Opcode::faultg:
        case Opcode::faulte:
        case Opcode::faultge:
        case Opcode::faultl:
        case Opcode::faultne:
        case Opcode::faultle:
        case Opcode::faulto:
            condFault(instruction);
            break;
            // COBR Format
            // just like with fault and branch, test has the mask embedded in the opcode, so extract it out
        case Opcode::testno:
        case Opcode::testg:
        case Opcode::teste:
        case Opcode::testge:
        case Opcode::testl:
        case Opcode::testne:
        case Opcode::testle:
        case Opcode::testo:
            testOp(instruction);
            break;
        case Opcode::bbc:
            // branch if bit is clear
            bbc(instruction);
            break;
        case Opcode::bbs:
            bbs(instruction);
            break;
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
            // just like with the others, intel encoded the mask for cmpo operations into the opcode itself
            // we can just extract it
        case Opcode::cmpobg:
        case Opcode::cmpobe:
        case Opcode::cmpobge:
        case Opcode::cmpobl:
        case Opcode::cmpobne:
        case Opcode::cmpoble:
            cmpobx(instruction);
            break;

            // just like with the others, intel encoded the mask for cmpi operations into the opcode itself
            // we can just extract it
        case Opcode::cmpibno:
        case Opcode::cmpibg:
        case Opcode::cmpibe:
        case Opcode::cmpibge:
        case Opcode::cmpibl:
        case Opcode::cmpibne:
        case Opcode::cmpible:
        case Opcode::cmpibo:
            cmpibx(instruction);
            break;
        case Opcode::concmpi:
            concmpGeneric<TreatAsInteger>(instruction);
            break;
        case Opcode::concmpo:
            concmpGeneric<TreatAsOrdinal>(instruction);
            break;
            // MEM Format
        case Opcode::ldob:
            setDestinationFromSrcDest(instruction, loadByte(computeMemoryAddress(instruction)), TreatAsOrdinal {});
            break;
        case Opcode::bx:
            bx(instruction);
            break;
        case Opcode::balx:
            balx(instruction);
            break;
        case Opcode::ldos:
            setDestinationFromSrcDest(instruction, loadShort(computeMemoryAddress(instruction)), TreatAsOrdinal {});
            break;
        case Opcode::lda:
            lda(instruction);
            break;
        case Opcode::ld:
            setDestinationFromSrcDest(instruction, load(computeMemoryAddress(instruction)), TreatAsOrdinal {});
            break;
        case Opcode::ldl:
            getDoubleRegister(instruction.getSrcDest(false)).setLongOrdinal( loadLong(computeMemoryAddress(instruction)) );
            break;
        case Opcode::ldt:
            load(computeMemoryAddress(instruction),
                 getTripleRegister(instruction.getSrcDest(false)));
            break;
        case Opcode::ldq:
            load(computeMemoryAddress(instruction),
                 getQuadRegister(instruction.getSrcDest(false)));
            break;
            // REG format
        case Opcode::addi:
            arithmeticGeneric<ArithmeticOperation::Add, TreatAsInteger>(instruction);
            break;
        case Opcode::addo:
            arithmeticGeneric<ArithmeticOperation::Add, TreatAsOrdinal>(instruction);
            break;
        case Opcode::subi:
            arithmeticGeneric<ArithmeticOperation::Subtract, TreatAsInteger>(instruction);
            break;
        case Opcode::subo:
            arithmeticGeneric<ArithmeticOperation::Subtract, TreatAsOrdinal>(instruction);
            break;
        case Opcode::muli:
            arithmeticGeneric<ArithmeticOperation::Multiply, TreatAsInteger>(instruction);
            break;
        case Opcode::mulo:
            arithmeticGeneric<ArithmeticOperation::Multiply, TreatAsOrdinal>(instruction);
            break;
        case Opcode::divo:
            arithmeticGeneric<ArithmeticOperation::Divide, TreatAsOrdinal>(instruction);
            break;
        case Opcode::divi:
            arithmeticGeneric<ArithmeticOperation::Divide, TreatAsInteger>(instruction);
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
        case Opcode::remi:
            arithmeticGeneric<ArithmeticOperation::Remainder, TreatAsInteger>(instruction);
            break;
        case Opcode::remo:
            arithmeticGeneric<ArithmeticOperation::Remainder, TreatAsOrdinal>(instruction);
            break;
        case Opcode::rotate:
            arithmeticGeneric<ArithmeticOperation::Rotate, TreatAsOrdinal>(instruction);
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
        case Opcode::shli:
            arithmeticGeneric<ArithmeticOperation::ShiftLeft, TreatAsInteger>(instruction);
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
            setDestinationFromSrcDest(instruction, loadByte(computeMemoryAddress(instruction)), TreatAsInteger {});
            break;
        case Opcode::ldis:
            setDestinationFromSrcDest(instruction, loadShort(computeMemoryAddress(instruction)), TreatAsInteger {});
            break;
        case Opcode::st:
            store(computeMemoryAddress(instruction), sourceFromSrcDest(instruction).getOrdinal());
            break;
        case Opcode::stob:
            storeByte(computeMemoryAddress(instruction), sourceFromSrcDest(instruction).getByteOrdinal(0));
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
                auto src = static_cast<ByteInteger>(getSourceRegister(instruction.getSrcDest(true)).getInteger());
                storeByteInteger(computeMemoryAddress(instruction), src);
            }();
            break;
        case Opcode::stis:
            [this, &instruction]() {
                auto src = static_cast<ShortInteger>(getSourceRegister(instruction.getSrcDest(true)).getInteger());
                storeShortInteger(computeMemoryAddress(instruction), src);
            }();
            break;
        case Opcode::shri:
            arithmeticGeneric<ArithmeticOperation::ShiftRight, TreatAsInteger>(instruction);
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
            ret();
            break;
        default:
            generateFault(FaultType::Operation_InvalidOpcode);
            break;
    }
}

