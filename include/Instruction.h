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

#ifndef SIM3_INSTRUCTION_H
#define SIM3_INSTRUCTION_H
#include "Types.h"
enum class MEMFormatMode : uint8_t {
    // MEMA formats map to the top two bits with the lower of the two always being zero
    // This is why it is safe to do modeMajor & 1u because that lower bit will always be zero for mema and one for memb
    MEMA_AbsoluteOffset = 0b0000,
    Bad = 0b0001, // this will never show up in reality so I can reuse it
    MEMA_RegisterIndirectWithOffset = 0b1000,
    MEMB_RegisterIndirect = 0b0100,
    MEMB_IPWithDisplacement = 0b0101,
    MEMB_Reserved = 0b0110,
    MEMB_RegisterIndirectWithIndex = 0b0111,
    MEMB_AbsoluteDisplacement = 0b1100,
    MEMB_RegisterIndirectWithDisplacement = 0b1101,
    MEMB_IndexWithDisplacement = 0b1110,
    MEMB_RegisterIndirectWithIndexAndDisplacement = 0b1111,
};


constexpr auto isDoubleWideInstruction(MEMFormatMode mode) noexcept {
    switch (mode) {
        case MEMFormatMode::MEMB_IPWithDisplacement:
        case MEMFormatMode::MEMB_AbsoluteDisplacement:
        case MEMFormatMode::MEMB_RegisterIndirectWithDisplacement:
        case MEMFormatMode::MEMB_IndexWithDisplacement:
        case MEMFormatMode::MEMB_RegisterIndirectWithIndexAndDisplacement:
            return true;
        default:
            return false;
    }
}
template<MEMFormatMode mode>
constexpr auto isDoubleWideInstruction_v = isDoubleWideInstruction(mode);


constexpr FullOpcode makeFullOpcode(uint8_t majorOpcode) noexcept {
    return static_cast<FullOpcode>(majorOpcode);
}
constexpr FullOpcode makeFullOpcode(uint8_t baseOpcode, uint8_t secondaryOpcode) noexcept {
    return (static_cast<FullOpcode>(baseOpcode) << 4) | (static_cast<FullOpcode>(secondaryOpcode & 0x0F));
}
constexpr auto isCTRLFormat(uint8_t opcode) noexcept {
    return opcode < 0x20;
}
constexpr auto isCOBRFormat(uint8_t opcode) noexcept {
    return opcode >= 0x20 && opcode < 0x58;
}
constexpr auto isREGFormat(uint8_t opcode) noexcept {
    return opcode >= 0x58 && opcode < 0x80;
}
constexpr auto isMEMFormat(uint8_t opcode) noexcept {
    return opcode >= 0x80;
}
// based off of the i960 instruction set
union Instruction {
public:
    constexpr explicit Instruction(LongOrdinal value = 0) noexcept : wholeValue_(value) { }
    /**
     * @brief return the major opcode as an 8-bit quantity
     * @return The contents of the major opcode field without any modification
     */
    [[nodiscard]] constexpr uint8_t getMajorOpcode() const noexcept {
        return opcode;
    }
    /**
     * @brief Get the extra four bits used in REG format instructions
     * @return The extra four bits that make up a reg opcode
     */
    [[nodiscard]] constexpr uint8_t getMinorOpcode() const noexcept {
        return reg.opcodeExt;
    }
    [[nodiscard]] constexpr FullOpcode getOpcode() const noexcept {
        // The opcode is divided into two parts, the major opcode (8-bits) and the minor opcode (4-bits). The minor opcode only shows up in
        // reg format instructions. The intel manuals treat the REG instructions as coming _after_ all other instructions despite being
        // in the middle of the major opcode space. Thus we will do the same thing for simplicity. We should only pay for what we need.

        // For consistency, we will still be returning a 16-bit quantity but the reg format instructions will be the only ones that
        // are 12-bits long.
        if (auto majorOpcode = getMajorOpcode(); ::isREGFormat(majorOpcode)) {
            return makeFullOpcode(majorOpcode, getMinorOpcode());
        } else {
            return makeFullOpcode(majorOpcode);
        }
    }
    [[nodiscard]] constexpr uint8_t getEmbeddedMask() const noexcept { return getMajorOpcode() & 0b111; }
    [[nodiscard]] constexpr auto identifyOpcode() const noexcept { return static_cast<Opcode>(getOpcode()); }
    [[nodiscard]] constexpr auto isMEMFormat() const noexcept { return ::isMEMFormat(getMajorOpcode()); }
    [[nodiscard]] constexpr auto isREGFormat() const noexcept { return ::isREGFormat(getMajorOpcode()); }
    [[nodiscard]] constexpr auto isCOBRFormat() const noexcept { return ::isCOBRFormat(getMajorOpcode()); }
    [[nodiscard]] constexpr auto isCTRLFormat() const noexcept { return ::isCTRLFormat(getMajorOpcode()); }
    [[nodiscard]] constexpr Integer getDisplacement() const noexcept {
        if (isCOBRFormat()) {
            return cobr.displacement;
        } else if (isCTRLFormat()) {
            return ctrl.displacement;
        } else if (isMEMBFormat()) {
            return memb.optionalDisplacement;
        } else {
            // unsure what we should actually be returning when we have a failure
            return -1;
        }
    }
    [[nodiscard]] constexpr RegisterIndex getSrc1(bool ignoreM1 = false) const noexcept {
        if (isREGFormat()) {
            return makeRegisterIndex(reg.src1, reg.m1);
        } else if (isCOBRFormat()) {
            if (ignoreM1) {
                return makeRegister(cobr.src1);
            } else {
                return makeRegisterIndex(cobr.src1, cobr.m1);
            }
        } else {
            return RegisterIndex::Bad;
        }
    }
    [[nodiscard]] constexpr RegisterIndex getSrc2() const noexcept {
        if (isREGFormat()) {
            return makeRegisterIndex(reg.src2, reg.m2);
        } else if (isCOBRFormat()) {
            return makeRegister(cobr.src2);
        } else {
            return RegisterIndex::Bad;
        }
    }
    [[nodiscard]] constexpr RegisterIndex getSrcDest(bool treatAsSource) const noexcept {
        if (isREGFormat()) {
            return makeRegisterIndex(reg.srcDest, treatAsSource ? static_cast<bool>(reg.m3) : false);
        } else if (isMEMFormat()) {
            return makeRegister(mem.srcDest);
        } else {
            return RegisterIndex::Bad;
        }
    }
    [[nodiscard]] constexpr Ordinal getOffset() const noexcept {
        if (isMEMAFormat()) {
            return mema.offset;
        } else {
            return 0xFFFF'FFFF;
        }
    }
    [[nodiscard]] constexpr RegisterIndex getABase() const noexcept {
        if (isMEMFormat()) {
            return makeRegister(mem.abase);
        } else {
            return RegisterIndex::Bad;
        }
    }

    [[nodiscard]] constexpr MEMFormatMode getMemFormatMode() const noexcept {
        if (isMEMAFormat()) {
            return mema.mode == 0 ? MEMFormatMode::MEMA_AbsoluteOffset : MEMFormatMode::MEMA_RegisterIndirectWithOffset;
        } else if (isMEMBFormat()) {
            return static_cast<MEMFormatMode>(memb.mode);
        } else {
            return MEMFormatMode::Bad;
        }
    }
    [[nodiscard]] constexpr RegisterIndex getIndex() const noexcept {
        if (isMEMBFormat()) {
            return makeRegister(memb.index);
        } else {
            return RegisterIndex::Bad;
        }
    }
    [[nodiscard]] constexpr uint8_t getScale() const noexcept {
        if (isMEMBFormat()) {
            return memb.scale; // this is already setup for proper shifting
        } else {
            return 0;
        }
    }
    [[nodiscard]] constexpr auto isDoubleWide() const noexcept {
        // if it is not a MEM instruction then we still get Bad out which
        // is legal and returns false
        return isDoubleWideInstruction(getMemFormatMode());
    }

private:
    [[nodiscard]] constexpr bool isMEMAFormat() const noexcept {
        return isMEMFormat() && ((mem.modeMajor & 1u) == 0);
    }
    [[nodiscard]] constexpr bool isMEMBFormat() const noexcept {
        return isMEMFormat() && ((mem.modeMajor & 1u) != 0);
    }
private:

    struct {
        Ordinal lower : 24;
        Ordinal opcode : 8;
    };
    struct {
        Ordinal src1: 5;
        Ordinal s1: 1;
        Ordinal s2: 1;
        Ordinal opcodeExt: 4;
        Ordinal m1: 1;
        Ordinal m2: 1;
        Ordinal m3: 1;
        Ordinal src2: 5;
        Ordinal srcDest: 5;
        Ordinal opcode: 8;
    } reg;
    struct {
        // normally the bottom two bits are ignored by the processor (at least on the Sx) and are forced to zero
        // so it is immediately word aligned. No need to actually keep it that accurate
        Integer displacement: 13;
        Ordinal m1: 1;
        Ordinal src2: 5;
        Ordinal src1: 5;
        Ordinal opcode: 8;
    } cobr;
    struct {
        // normally the bottom two bits are ignored by the processor (at least on the Sx) and are forced to zero
        // so it is immediately word aligned. No need to actually keep it that accurate
        Integer displacement: 24;
        Ordinal opcode: 8;
    } ctrl;
    struct {
        Ordinal differentiationBlock: 12;
        Ordinal modeMajor: 2;
        Ordinal abase: 5;
        Ordinal srcDest: 5;
        Ordinal opcode: 8;
    } mem;
    struct {
        Ordinal offset: 12;
        Ordinal mode: 2;
        Ordinal abase: 5;
        Ordinal srcDest: 5;
        Ordinal opcode: 8;
    } mema;

    struct {
        Ordinal index: 5;
        Ordinal unused0: 2;
        Ordinal scale: 3;
        Ordinal mode: 4;
        Ordinal abase: 5;
        Ordinal srcDest: 5;
        Ordinal opcode: 8;
        Integer optionalDisplacement;
    } memb;
    LongOrdinal wholeValue_;
};
#endif //SIM3_INSTRUCTION_H
