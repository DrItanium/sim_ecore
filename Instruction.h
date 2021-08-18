//
// Created by jwscoggins on 8/18/21.
//

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


constexpr FullOpcode makeFullOpcode(uint8_t baseOpcode, uint8_t secondaryOpcode = 0) noexcept {
    return (static_cast<FullOpcode>(baseOpcode) << 4) | (static_cast<FullOpcode>(secondaryOpcode & 0x0F));
}
constexpr auto isCTRLFormat(FullOpcode opcode) noexcept {
    return opcode < 0x200;
}
constexpr auto isCOBRFormat(FullOpcode opcode) noexcept {
    return opcode >= 0x200 && opcode < 0x580;
}
constexpr auto isREGFormat(FullOpcode opcode) noexcept {
    return opcode >= 0x580 && opcode < 0x800;
}
constexpr auto isMEMFormat(FullOpcode opcode) noexcept {
    return opcode >= 0x800;
}
// based off of the i960 instruction set
union Instruction {
public:
    constexpr Instruction(Ordinal lower, Ordinal upper) noexcept : parts{lower, upper} { }
    constexpr explicit Instruction(LongOrdinal value = 0) noexcept : wholeValue_(value) { }
    constexpr auto getWideValue() const noexcept { return wholeValue_; }
    constexpr auto getHalf(int offset) const noexcept { return parts[offset & 1]; }
    constexpr auto getLowerHalf() const noexcept { return getHalf(0); }
    constexpr auto getUpperHalf() const noexcept { return getHalf(1); }
    constexpr FullOpcode getOpcode() const noexcept {
        // by default the opcode is only 8-bits wide...except for REG format instructions, they are 12-bits instead
        // so to make everything much simpler, we should just make a 16-bit opcode in all cases.
        // The normal opcode bits are mapped in the same place on all forms so we should just shift it right by 4 bits
        // so 0x11 => 0x110. This maintains sanity in all cases with a tad amount of overhead
        FullOpcode normalOpcode = makeFullOpcode(opcode);
        // okay now we need to figure out what kind of operation we are actually looking at.
        // Fortunately, the classes are range based :D
        // We only need to modify the opcode if it is a reg format instruction
        if (::isREGFormat(normalOpcode)) {
            normalOpcode |= reg.opcodeExt;
        }
        return normalOpcode;
    }
    constexpr auto identifyOpcode() const noexcept { return static_cast<Opcode>(getOpcode()); }
    constexpr auto isMEMFormat() const noexcept { return ::isMEMFormat(getOpcode()); }
    constexpr auto isREGFormat() const noexcept { return ::isREGFormat(getOpcode()); }
    constexpr auto isCOBRFormat() const noexcept { return ::isCOBRFormat(getOpcode()); }
    constexpr auto isCTRLFormat() const noexcept { return ::isCTRLFormat(getOpcode()); }
    constexpr Integer getDisplacement() const noexcept {
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
    constexpr RegisterIndex getSrc1() const noexcept {
        if (isREGFormat()) {
            return makeRegisterIndex(reg.src1, reg.m1);
        } else if (isCOBRFormat()) {
            return makeRegister(cobr.src1);
        } else {
            return RegisterIndex::Bad;
        }
    }
    constexpr RegisterIndex getSrc2() const noexcept {
        if (isREGFormat()) {
            return makeRegisterIndex(reg.src2, reg.m2);
        } else if (isCOBRFormat()) {
            return makeRegister(cobr.src2);
        } else {
            return RegisterIndex::Bad;
        }
    }
    constexpr RegisterIndex getSrcDest(bool treatAsSource) const noexcept {
        if (isREGFormat()) {
            return makeRegisterIndex(reg.srcDest, treatAsSource ? reg.m3 : 0);
        } else if (isMEMFormat()) {
            return makeRegister(mem.srcDest);
        } else {
            return RegisterIndex::Bad;
        }
    }
    constexpr Ordinal getOffset() const noexcept {
        if (isMEMAFormat()) {
            return mema.offset;
        } else {
            return 0xFFFF'FFFF;
        }
    }
    constexpr RegisterIndex getABase() const noexcept {
        if (isMEMFormat()) {
            return makeRegister(mem.abase);
        } else {
            return RegisterIndex::Bad;
        }
    }

    constexpr MEMFormatMode getMemFormatMode() const noexcept {
        if (isMEMAFormat()) {
            return mema.mode == 0 ? MEMFormatMode::MEMA_AbsoluteOffset : MEMFormatMode::MEMA_RegisterIndirectWithOffset;
        } else if (isMEMBFormat()) {
            return static_cast<MEMFormatMode>(memb.mode);
        } else {
            return MEMFormatMode::Bad;
        }
    }
    constexpr RegisterIndex getIndex() const noexcept {
        if (isMEMBFormat()) {
            return makeRegister(memb.index);
        } else {
            return RegisterIndex::Bad;
        }
    }
    constexpr uint8_t getScale() const noexcept {
        if (isMEMBFormat()) {
            return memb.scale; // this is already setup for proper shifting
        } else {
            return 0;
        }
    }
    constexpr auto isDoubleWide() const noexcept {
        // if it is not a MEM instruction then we still get Bad out which
        // is legal and returns false
        return isDoubleWideInstruction(getMemFormatMode());
    }

private:
    constexpr Ordinal isMEMAFormat() const noexcept {
        return isMEMFormat() && ((mem.modeMajor & 1u) == 0);
    }
    constexpr Ordinal isMEMBFormat() const noexcept {
        return isMEMFormat() && ((mem.modeMajor & 1u) != 0);
    }
private:
    Ordinal parts[sizeof(LongOrdinal)/sizeof(Ordinal)];

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
