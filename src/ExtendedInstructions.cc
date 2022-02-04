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
//
// Created by jwscoggins on 2/3/22.
//

#include "Core.h"

Register&
Core::destinationFromSrc2(const Instruction &instruction) noexcept {
    return getRegister(instruction.getSrc2());
}
const Register&
Core::registerFromSrc1(const Instruction& instruction) noexcept {
    return getRegister(instruction.getSrc1());
}
void
Core::bswap(const Instruction& inst) noexcept {
    // copies bytes 3:0 of src1 to src2 reversing order of the bytes.
    // Byte 0 of src1 -> Byte 3 of src2
    // Byte 1 of src1 -> Byte 2 of src2
    // Byte 2 of src1 -> Byte 1 of src2
    // Byte 3 of src1 -> Byte 0 of src2
    // from the Jx manual (not implemented on the Sx, Kx, and Cx processors)
    // dest = (rotate_left(src 8) & 0x00FF00FF) + (rotate_left(src 24) & 0xFF00FF00)
    auto& dest = destinationFromSrc2(inst);
    const auto& src = registerFromSrc1(inst);
    dest.set(src.get(0, TreatAsByteOrdinal{}), 3, TreatAsByteOrdinal{});
    dest.set(src.get(1, TreatAsByteOrdinal{}), 2, TreatAsByteOrdinal{});
    dest.set(src.get(2, TreatAsByteOrdinal{}), 1, TreatAsByteOrdinal{});
    dest.set(src.get(3, TreatAsByteOrdinal{}), 0, TreatAsByteOrdinal{});
}

void
Core::condSelect(const Instruction &inst) noexcept {
    if (auto mask = inst.getEmbeddedMask(); (mask & ac_.getConditionCode()) || (mask == ac_.getConditionCode())) {
        setDestinationFromSrcDest(inst, valueFromSrc2Register<Ordinal>(inst), TreatAsOrdinal{});
    } else {
        setDestinationFromSrcDest(inst, valueFromSrc1Register<Ordinal>(inst), TreatAsOrdinal{});
    }
}