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
    static constexpr byte REGBaseOffset = 0x58;
    // unlike the other instructions, there is a second level of decode necessary for these
    // so this contains the first dispatch level only
    using LookupTable16 = TargetFunction[16];
    static constexpr LookupTable16 REGTable_0x58 {
            &Core::notbit,
            &Core::logicalOpGeneric<LogicalOp::And>,
            &Core::logicalOpGeneric<LogicalOp::AndNot>,
            &Core::setbit,
            &Core::logicalOpGeneric<LogicalOp::NotAnd>,
            &Core::illegalInstruction,
            &Core::logicalOpGeneric<Core::LogicalOp::Xor>,
            &Core::logicalOpGeneric<Core::LogicalOp::Or>,
            &Core::logicalOpGeneric<Core::LogicalOp::Nor>,
            &Core::logicalOpGeneric<Core::LogicalOp::Xnor>,
            &Core::logicalOpGeneric<Core::LogicalOp::Not>,
            &Core::logicalOpGeneric<Core::LogicalOp::OrNot>,
            &Core::clrbit,
            &Core::logicalOpGeneric<Core::LogicalOp::NotOr>,
            &Core::logicalOpGeneric<Core::LogicalOp::Nand>,
            &Core::alterbit,
    };
    static constexpr LookupTable16 REGTable_0x59 {
            &Core::arithmeticGeneric<ArithmeticOperation::Add, Ordinal>,
            &Core::arithmeticGeneric<ArithmeticOperation::Add, Integer>,
            &Core::arithmeticGeneric<ArithmeticOperation::Subtract, Ordinal>,
            &Core::arithmeticGeneric<ArithmeticOperation::Subtract, Integer>,
            /// @todo implement extended operations
            &Core::illegalInstruction, // cmpob in extended
            &Core::illegalInstruction, // cmpib in extended
            &Core::illegalInstruction, // cmpos in extended
            &Core::illegalInstruction, // cmpis in extended
            &Core::shro,
            &Core::shrdi,
            &Core::arithmeticGeneric<ArithmeticOperation::ShiftRight, Integer>,
            &Core::shlo,
            &Core::arithmeticGeneric<ArithmeticOperation::Rotate, Ordinal>,
            &Core::arithmeticGeneric<ArithmeticOperation::ShiftLeft, Integer>,
            &Core::illegalInstruction,
    };
    static constexpr LookupTable16 REGTable_0x5A {
            &Core::cmpo,
            &Core::cmpi,
            &Core::concmpGeneric<Ordinal>,
            &Core::concmpGeneric<Integer>,
            &Core::cmpinco,
            &Core::cmpinci,
            &Core::cmpdeco,
            &Core::cmpdeci,
            &Core::illegalInstruction, // 0x08
            &Core::illegalInstruction, // 0x09
            &Core::illegalInstruction, // 0x0A
            &Core::illegalInstruction, // 0x0B
            &Core::scanbyte, // 0x0C
            &Core::bswap, // 0x0D
            &Core::chkbit, // 0x0E
            &Core::illegalInstruction, // 0x0F
    };
    static constexpr LookupTable16 REGTable_0x5B{
            &Core::addc, // 0x00
            &Core::illegalInstruction, // 0x01
            &Core::subc, // 0x02
            &Core::illegalInstruction, // 0x03
            &Core::illegalInstruction, // 0x04, intdis
            &Core::illegalInstruction, // 0x05, inten
            &Core::illegalInstruction, // 0x06
            &Core::illegalInstruction, // 0x07
            &Core::illegalInstruction, // 0x08
            &Core::illegalInstruction, // 0x09
            &Core::illegalInstruction, // 0x0A
            &Core::illegalInstruction, // 0x0B
            &Core::illegalInstruction, // 0x0C
            &Core::illegalInstruction, // 0x0D
            &Core::illegalInstruction, // 0x0E
            &Core::illegalInstruction, // 0x0F
    };
    static constexpr LookupTable16 REGTable_0x5C{
            &Core::illegalInstruction, // 0x00
            &Core::illegalInstruction, // 0x01
            &Core::illegalInstruction, // 0x02
            &Core::illegalInstruction, // 0x03
            &Core::illegalInstruction, // 0x04
            &Core::illegalInstruction, // 0x05
            &Core::illegalInstruction, // 0x06
            &Core::illegalInstruction, // 0x07
            &Core::illegalInstruction, // 0x08
            &Core::illegalInstruction, // 0x09
            &Core::illegalInstruction, // 0x0A
            &Core::illegalInstruction, // 0x0B
            &Core::mov, // 0x0C
            &Core::illegalInstruction, // 0x0D
            &Core::illegalInstruction, // 0x0E
            &Core::illegalInstruction, // 0x0F
    };
    static constexpr LookupTable16 REGTable_0x5D {
            &Core::illegalInstruction, // 0x00
            &Core::illegalInstruction, // 0x01
            &Core::illegalInstruction, // 0x02
            &Core::illegalInstruction, // 0x03
            &Core::illegalInstruction, // 0x04
            &Core::illegalInstruction, // 0x05
            &Core::illegalInstruction, // 0x06
            &Core::illegalInstruction, // 0x07
            &Core::illegalInstruction, // 0x08, eshro
            &Core::illegalInstruction, // 0x09
            &Core::illegalInstruction, // 0x0A
            &Core::illegalInstruction, // 0x0B
            &Core::movl, // 0x0C
            &Core::illegalInstruction, // 0x0D
            &Core::illegalInstruction, // 0x0E
            &Core::illegalInstruction, // 0x0F
    };
    static constexpr LookupTable16 REGTable_0x5E {
            &Core::illegalInstruction, // 0x00
            &Core::illegalInstruction, // 0x01
            &Core::illegalInstruction, // 0x02
            &Core::illegalInstruction, // 0x03
            &Core::illegalInstruction, // 0x04
            &Core::illegalInstruction, // 0x05
            &Core::illegalInstruction, // 0x06
            &Core::illegalInstruction, // 0x07
            &Core::illegalInstruction, // 0x08
            &Core::illegalInstruction, // 0x09
            &Core::illegalInstruction, // 0x0A
            &Core::illegalInstruction, // 0x0B
            &Core::movt, // 0x0C
            &Core::illegalInstruction, // 0x0D
            &Core::illegalInstruction, // 0x0E
            &Core::illegalInstruction, // 0x0F
    };
    static constexpr LookupTable16 REGTable_0x5F {
            &Core::illegalInstruction, // 0x00
            &Core::illegalInstruction, // 0x01
            &Core::illegalInstruction, // 0x02
            &Core::illegalInstruction, // 0x03
            &Core::illegalInstruction, // 0x04
            &Core::illegalInstruction, // 0x05
            &Core::illegalInstruction, // 0x06
            &Core::illegalInstruction, // 0x07
            &Core::illegalInstruction, // 0x08
            &Core::illegalInstruction, // 0x09
            &Core::illegalInstruction, // 0x0A
            &Core::illegalInstruction, // 0x0B
            &Core::movq, // 0x0C
            &Core::illegalInstruction, // 0x0D
            &Core::illegalInstruction, // 0x0E
            &Core::illegalInstruction, // 0x0F
    };
    static constexpr LookupTable16 REGTable_0x60 {
            &Core::synmov, // 0x00
            &Core::synmovl, // 0x01
            &Core::synmovq, // 0x02
            &Core::illegalInstruction, // 0x03, cmpstr
            &Core::illegalInstruction, // 0x04, movqstr
            &Core::illegalInstruction, // 0x05, movstr
            &Core::illegalInstruction, // 0x06
            &Core::illegalInstruction, // 0x07
            &Core::illegalInstruction, // 0x08
            &Core::illegalInstruction, // 0x09
            &Core::illegalInstruction, // 0x0A
            &Core::illegalInstruction, // 0x0B
            &Core::illegalInstruction, // 0x0C
            &Core::illegalInstruction, // 0x0D
            &Core::illegalInstruction, // 0x0E
            &Core::illegalInstruction, // 0x0F
    };
    static constexpr LookupTable16 REGTable_0x61{
            &Core::atmod, // 0x00
            &Core::illegalInstruction, // 0x01
            &Core::atadd, // 0x02
            &Core::illegalInstruction, // 0x03, inspacc
            &Core::illegalInstruction, // 0x04, ldphy
            &Core::synld, // 0x05,
            &Core::illegalInstruction, // 0x06
            &Core::illegalInstruction, // 0x07, fill
            &Core::illegalInstruction, // 0x08
            &Core::illegalInstruction, // 0x09
            &Core::illegalInstruction, // 0x0A
            &Core::illegalInstruction, // 0x0B
            &Core::illegalInstruction, // 0x0C
            &Core::illegalInstruction, // 0x0D
            &Core::illegalInstruction, // 0x0E
            &Core::illegalInstruction, // 0x0F
    };
    static constexpr LookupTable16 REGTable_0x64 {
            &Core::spanbit, // 0x00
            &Core::scanbit, // 0x01
            &Core::illegalInstruction, // 0x02, daddc
            &Core::illegalInstruction, // 0x03, dsubc
            &Core::illegalInstruction, // 0x04, dmovt
            &Core::modac, // 0x05
            &Core::illegalInstruction, // 0x06, condrec
            &Core::illegalInstruction, // 0x07
            &Core::illegalInstruction, // 0x08
            &Core::illegalInstruction, // 0x09
            &Core::illegalInstruction, // 0x0A
            &Core::illegalInstruction, // 0x0B
            &Core::illegalInstruction, // 0x0C
            &Core::illegalInstruction, // 0x0D
            &Core::illegalInstruction, // 0x0E
            &Core::illegalInstruction, // 0x0F
    };
    static constexpr LookupTable16 REGTable_0x65 {
            &Core::modify, // 0x00
            &Core::extract, // 0x01
            &Core::illegalInstruction, // 0x02
            &Core::illegalInstruction, // 0x03
            &Core::modtc, // 0x04
            &Core::modpc, // 0x05
            &Core::illegalInstruction, // 0x06, receive
            &Core::illegalInstruction, // 0x07
            &Core::illegalInstruction, // 0x08, intctl
            &Core::illegalInstruction, // 0x09, sysctl
            &Core::illegalInstruction, // 0x0A
            &Core::illegalInstruction, // 0x0B, icctl
            &Core::illegalInstruction, // 0x0C, dcctl
            &Core::illegalInstruction, // 0x0D, halt
            &Core::illegalInstruction, // 0x0E
            &Core::illegalInstruction, // 0x0F
    };
    static constexpr LookupTable16 REGTable_0x66{
            &Core::calls, // 0x00
            &Core::illegalInstruction, // 0x01
            &Core::illegalInstruction, // 0x02, send
            &Core::illegalInstruction, // 0x03, sendserv
            &Core::illegalInstruction, // 0x04, resumprcs
            &Core::illegalInstruction, // 0x05, schedprcs
            &Core::illegalInstruction, // 0x06, saveprcs
            &Core::illegalInstruction, // 0x07
            &Core::illegalInstruction, // 0x08, condwait
            &Core::illegalInstruction, // 0x09, wait
            &Core::illegalInstruction, // 0x0A, signal
            &Core::mark, // 0x0B
            &Core::fmark, // 0x0C
            &Core::flushreg, // 0x0D
            &Core::illegalInstruction, // 0x0E
            &Core::syncf, // 0x0F
    };
    static constexpr LookupTable16 REGTable_0x67 {
            &Core::emul, // 0x00
            &Core::ediv, // 0x01
            &Core::illegalInstruction, // 0x02
            &Core::illegalInstruction, // 0x03, ldtime
            &Core::illegalInstruction, // 0x04, cvtir
            &Core::illegalInstruction, // 0x05, cvtilr
            &Core::illegalInstruction, // 0x06, scalerl
            &Core::illegalInstruction, // 0x07, scaler
            &Core::illegalInstruction, // 0x08
            &Core::illegalInstruction, // 0x09
            &Core::illegalInstruction, // 0x0A
            &Core::illegalInstruction, // 0x0B
            &Core::illegalInstruction, // 0x0C
            &Core::illegalInstruction, // 0x0D
            &Core::illegalInstruction, // 0x0E
            &Core::illegalInstruction, // 0x0F
    };
    static constexpr LookupTable16 REGTable_0x70{
            &Core::illegalInstruction, // 0x00
            &Core::arithmeticGeneric<ArithmeticOperation::Multiply, Ordinal>, // 0x01
            &Core::illegalInstruction, // 0x02
            &Core::illegalInstruction, // 0x03
            &Core::illegalInstruction, // 0x04
            &Core::illegalInstruction, // 0x05
            &Core::illegalInstruction, // 0x06
            &Core::illegalInstruction, // 0x07
            &Core::arithmeticGeneric<ArithmeticOperation::Remainder, Ordinal>, // 0x08
            &Core::illegalInstruction, // 0x09
            &Core::illegalInstruction, // 0x0A
            &Core::arithmeticGeneric<ArithmeticOperation::Divide, Ordinal>, // 0x0B
            &Core::illegalInstruction, // 0x0C
            &Core::illegalInstruction, // 0x0D
            &Core::illegalInstruction, // 0x0E
            &Core::illegalInstruction, // 0x0F
    };
    static constexpr LookupTable16 REGTable_0x74{
            &Core::illegalInstruction, // 0x00
            &Core::arithmeticGeneric<ArithmeticOperation::Multiply, Integer>, // 0x01
            &Core::illegalInstruction, // 0x02
            &Core::illegalInstruction, // 0x03
            &Core::illegalInstruction, // 0x04
            &Core::illegalInstruction, // 0x05
            &Core::illegalInstruction, // 0x06
            &Core::illegalInstruction, // 0x07
            &Core::arithmeticGeneric<ArithmeticOperation::Remainder, Integer>, // 0x08
            &Core::modi, // 0x09
            &Core::illegalInstruction, // 0x0A
            &Core::arithmeticGeneric<ArithmeticOperation::Divide, Integer>, // 0x0B
            &Core::illegalInstruction, // 0x0C
            &Core::illegalInstruction, // 0x0D
            &Core::illegalInstruction, // 0x0E
            &Core::illegalInstruction, // 0x0F
    };
    static constexpr LookupTable16 EmptyTable {
            &Core::illegalInstruction, // 0x00
            &Core::illegalInstruction, // 0x01
            &Core::illegalInstruction, // 0x02
            &Core::illegalInstruction, // 0x03
            &Core::illegalInstruction, // 0x04
            &Core::illegalInstruction, // 0x05
            &Core::illegalInstruction, // 0x06
            &Core::illegalInstruction, // 0x07
            &Core::illegalInstruction, // 0x08
            &Core::illegalInstruction, // 0x09
            &Core::illegalInstruction, // 0x0A
            &Core::illegalInstruction, // 0x0B
            &Core::illegalInstruction, // 0x0C
            &Core::illegalInstruction, // 0x0D
            &Core::illegalInstruction, // 0x0E
            &Core::illegalInstruction, // 0x0F
    };
    static constexpr const TargetFunction* REGLookupTable[40] {
            REGTable_0x58,
            REGTable_0x59,
            REGTable_0x5A,
            REGTable_0x5B,
            REGTable_0x5C,
            REGTable_0x5D,
            REGTable_0x5E,
            REGTable_0x5F,
            REGTable_0x60,
            REGTable_0x61,
            EmptyTable,
            EmptyTable,
            REGTable_0x64,
            REGTable_0x65,
            REGTable_0x66,
            REGTable_0x67,
            EmptyTable, // floating point only
            EmptyTable, // floating point only
            EmptyTable,
            EmptyTable,
            EmptyTable, // floating point only
            EmptyTable, // floating point only
            EmptyTable, // floating point only
            EmptyTable,
            REGTable_0x70,
            EmptyTable,
            EmptyTable,
            EmptyTable,
            REGTable_0x74,
            EmptyTable,
            EmptyTable,
            EmptyTable,
            EmptyTable, // core extended and floating point
            EmptyTable, // core extended and floating point
            EmptyTable, // core extended and floating point
            EmptyTable, // core extended and floating point
            EmptyTable, // core extended and floating point
            EmptyTable, // core extended and floating point
            EmptyTable, // core extended and floating point
            EmptyTable, // core extended and floating point
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
    } else if (instruction.isREGFormat()) {
        /// @todo handle 0x5C specially since there is only one operation in that space (saves ram)
        auto properOffset = instruction.getMajorOpcode() - REGBaseOffset;
        (this->*REGLookupTable[properOffset][instruction.getMinorOpcode()])(instruction);
    } else {
        illegalInstruction(instruction);
    }
}

