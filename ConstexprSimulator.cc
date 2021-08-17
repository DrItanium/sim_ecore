#include <cstdint>
using LongOrdinal = uint64_t;
using Ordinal = uint32_t;
using LongInteger = int64_t;
using Integer = int32_t;
using ShortOrdinal = uint16_t;
using ShortInteger = int16_t;
using ByteOrdinal = uint8_t;
using ByteInteger = int8_t;

using FullOpcode = uint16_t;
#ifdef NUMERICS_ARCHITECTURE
using Real = float;
using LongReal = double;
// right now, we can just exploit the fact that extended real and long real are different sizes on x86_64, just like i960
using ExtendedReal = long double;
#endif

enum class Opcode : FullOpcode  {
    None = 0,
#define X(value, name) name = value ,
#include "OpcodesRaw.h"
#undef X
    Bad = 0xFFFF,
};

enum class RegisterIndex : uint8_t {
    Local0 = 0,
    Local1,
    Local2,
    Local3,
    Local4,
    Local5,
    Local6,
    Local7,
    Local8,
    Local9,
    Local10,
    Local11,
    Local12,
    Local13,
    Local14,
    Local15,
    Global0,
    Global1,
    Global2,
    Global3,
    Global4,
    Global5,
    Global6,
    Global7,
    Global8,
    Global9,
    Global10,
    Global11,
    Global12,
    Global13,
    Global14,
    Global15,
    Literal0,
    Literal1,
    Literal2,
    Literal3,
    Literal4,
    Literal5,
    Literal6,
    Literal7,
    Literal8,
    Literal9,
    Literal10,
    Literal11,
    Literal12,
    Literal13,
    Literal14,
    Literal15,
    Literal16,
    Literal17,
    Literal18,
    Literal19,
    Literal20,
    Literal21,
    Literal22,
    Literal23,
    Literal24,
    Literal25,
    Literal26,
    Literal27,
    Literal28,
    Literal29,
    Literal30,
    Literal31,
#ifdef NUMERICS_ARCHITECTURE
    // floating point literals and registers have to be handled a tad
    // differently they must be converted from the standard bits to this
    // extended format to make it unique. The instruction bits itself will 
    // denote if the instruction operates on float data or not
    FP0 = 0b010'00000,
    FP1,
    FP2,
    FP3,
    // reserved entries here
    Literal0_0f = 0b010'10000,
    // reserved
    Literal1_0f = 0b010'10110,
#endif
    Bad = 0b1111'1111,
    PFP = Local0,
    SP = Local1,
    RIP = Local2,
    FP = Global15,
};
static_assert(static_cast<uint8_t>(RegisterIndex::Literal0) == 0b1'00000, "Literal 0 needs to be 0b100000 to accurately reflect the i960 design");

constexpr auto makeLiteral(uint8_t value) noexcept {
    return static_cast<RegisterIndex>(0b100000 | (value & 0b011111));
}
constexpr auto makeRegister(uint8_t value) noexcept {
    return static_cast<RegisterIndex>(value & 0b11111);
}
constexpr auto makeRegisterIndex(uint8_t value, bool isLiteral) noexcept {
    if (isLiteral) {
        return makeLiteral(value);
    } else {
        return makeRegister(value);
    }
}

constexpr auto isLiteral(RegisterIndex index) noexcept {
    return static_cast<uint8_t>(index) >= static_cast<uint8_t>(RegisterIndex::Literal0) &&
           static_cast<uint8_t>(index) <= static_cast<uint8_t>(RegisterIndex::Literal31);
}
constexpr auto isRegister(RegisterIndex index) noexcept {
    return static_cast<uint8_t>(index) < static_cast<uint8_t>(RegisterIndex::Literal0);
}
constexpr auto isLocalRegister(RegisterIndex index) noexcept {
    return static_cast<uint8_t>(index) < static_cast<uint8_t>(RegisterIndex::Global0);
}
constexpr auto isGlobalRegister(RegisterIndex index) noexcept {
    return isRegister(index) && !isLocalRegister(index);
}
#ifdef NUMERICS_ARCHITECTURE
constexpr auto toFloatingPointForm(RegisterIndex index) noexcept {
    if (isLiteral(index)) {
        switch (index) {
            case RegisterIndex::Literal0: return RegisterIndex::FP0;
            case RegisterIndex::Literal1: return RegisterIndex::FP1;
            case RegisterIndex::Literal2: return RegisterIndex::FP2;
            case RegisterIndex::Literal3: return RegisterIndex::FP3;
            case RegisterIndex::Literal16: return RegisterIndex::Literal0_0f;
            case RegisterIndex::Literal22: return RegisterIndex::Literal1_0f;
            default: return RegisterIndex::Bad;
        }
    } else {
        return index;
    }
}

constexpr auto isFloatingPointRegister(RegisterIndex index) noexcept {
    switch (index) {
        case RegisterIndex::FP0:
        case RegisterIndex::FP1:
        case RegisterIndex::FP2:
        case RegisterIndex::FP3:
            return true;
        default:
            return false;
    }
}

constexpr auto isFloatingPointLiteral(RegisterIndex index) noexcept {
    return index == RegisterIndex::Literal0_0f || index == RegisterIndex::Literal1_0f;
}
static_assert(toFloatingPointForm(RegisterIndex::Bad) == RegisterIndex::Bad, "toFloatingPointForm is not passing through correctly!");
#endif

constexpr Ordinal getLiteralOrdinal(RegisterIndex index) noexcept {
    if (isLiteral(index)) {
        return static_cast<Ordinal>(static_cast<uint8_t>(index) & 0b11111);
    } else {
        return 0xFFFF'FFFF;
    }
}



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
union Register {
public:
    constexpr explicit Register(Ordinal value = 0) noexcept : ord_(value) { }
    constexpr auto getOrdinal() const noexcept { return ord_; }
    constexpr auto getInteger() const noexcept { return integer_; }
    constexpr auto getShortOrdinal(int which = 0) const noexcept { return sords_[which&0b01]; }
    constexpr auto getShortInteger(int which = 0) const noexcept { return sints_[which&0b01]; }
    constexpr auto getByteOrdinal(int which = 0) const noexcept { return bords_[which&0b11]; }
    constexpr auto getByteInteger(int which = 0) const noexcept { return bints_[which&0b11]; }
    void setOrdinal(Ordinal value) noexcept { ord_ = value; }
    void setInteger(Integer value) noexcept { integer_ = value; }
    void setShortOrdinal(ShortOrdinal value) noexcept { setOrdinal(value); }
    void setShortInteger(ShortInteger value) noexcept { setInteger(value); }
    void setByteOrdinal(ByteOrdinal value) noexcept { setOrdinal(value); }
    void setByteInteger(ByteInteger value) noexcept { setInteger(value); }
    void setShortOrdinal(ShortOrdinal value, int which) noexcept { sords_[which&0b01] = value; }
    void setShortInteger(ShortInteger value, int which) noexcept { sints_[which&0b01] = value; }
    void setByteOrdinal(ByteOrdinal value, int which) noexcept { bords_[which&0b11] = value; }
    void setByteInteger(ByteInteger value, int which) noexcept { bints_[which&0b11] = value; }
#ifdef NUMERICS_ARCHTIECTURE
    constexpr auto getReal() const noexcept { return real_; }
    void setReal(Real value) noexcept { real_ = value; }
#endif
private:
    Ordinal ord_ = 0;
    Integer integer_;
    ShortOrdinal sords_[sizeof(Ordinal)/sizeof(ShortOrdinal)];
    ShortInteger sints_[sizeof(Integer)/sizeof(ShortInteger)];
    ByteOrdinal bords_[sizeof(Ordinal)/sizeof(ByteOrdinal)];
    ByteInteger bints_[sizeof(Integer)/sizeof(ByteInteger)];
#ifdef NUMERICS_ARCHITECTURE
    Real real_;
#endif
};

class PreviousFramePointer {
public:
    explicit PreviousFramePointer(Register& targetRegister) : reg_(targetRegister) {}
    constexpr bool getPrereturnTraceFlag() const noexcept { return (reg_.getOrdinal() & 0b1000); }
    void enablePrereturnTraceFlag() const noexcept { reg_.setOrdinal(reg_.getOrdinal() | 0b1000); }
    void disablePrereturnTraceFlag() const noexcept { reg_.setOrdinal(reg_.getOrdinal() & ~static_cast<Ordinal>(0b1000)); }
    void setPrereturnTraceFlag(bool value) const noexcept {
        value ? enablePrereturnTraceFlag() : disablePrereturnTraceFlag();
    }
private:
    Register& reg_;
};

union ArithmeticControls {
public:
    constexpr explicit ArithmeticControls(Ordinal value = 0) noexcept : ord_(value) { }
    constexpr auto getValue() const noexcept { return ord_; }
    void setValue(Ordinal value) noexcept { ord_ = value; }
#define X(name, field, type) \
        constexpr type get ## name () const noexcept { return field ; } \
        void set ## name( type value) noexcept { field  = value ; }
    X(ConditionCode, conditionCode, Ordinal);
    X(NoImpreciseFaults, noImpreciseFaults, bool);
#ifdef NUMERICS_ARCHITECTURE
    X(ArithmeticStatus, arithmeticStatus, Ordinal);
        X(FloatingPointRoundingControl, floatingPointRoundingControl, Ordinal);
        X(FloatingPointNormalizingMode, floatingPointNormalizingMode, bool);
#endif
#define Y(name, field) \
        X(name ## Flag , field ## Flag, bool); \
        X(name ## Mask , field ## Mask, bool);
    Y(IntegerOverflow, integerOverflow);
#ifdef NUMERICS_ARCHITECTURE
    Y(FloatingOverflow, floatingOverflow);
        Y(FloatingUnderflow, floatingUnderflow);
        Y(FloatingInvalidOp, floatingInvalidOp);
        Y(FloatingZeroDivide, floatingZeroDivide);
        Y(FloatingInexact, floatingInexact);
#endif
#undef Y
#undef X
public:
    template<uint8_t value>
    constexpr bool conditionCodeIs() const noexcept {
        if constexpr ((value & 0b111) == 0b000) {
            return (getConditionCode() & (value & 0b111)) == 0;
        } else {
            return (getConditionCode() & (value & 0b111)) != 0;
        }
    };
    /**
     * Reads and modifies contents of this object
     */
    Ordinal modify(Ordinal mask, Ordinal src) noexcept;
private:
    Ordinal ord_ = 0;
    struct {
        Ordinal conditionCode : 3;
#ifdef NUMERICS_ARCHITECTURE
        Ordinal arithmeticStatus : 4;
            Ordinal unused0 : 1;
#else
        Ordinal unused0 : 5;
#endif // end defined(NUMERICS_ARCHITECTURE)
        Ordinal integerOverflowFlag : 1;
        Ordinal unused1 : 3;
        Ordinal integerOverflowMask : 1;
        Ordinal unused2 : 2;
        Ordinal noImpreciseFaults : 1;
#ifdef NUMERICS_ARCHITECTURE
        Ordinal floatingOverflowFlag : 1;
            Ordinal floatingUnderflowFlag : 1;
            Ordinal floatingInvalidOpFlag : 1;
            Ordinal floatingZeroDivideFlag : 1;
            Ordinal floatingInexactFlag : 1;
            Ordinal unused3 : 3;
            Ordinal floatingOverflowMask : 1;
            Ordinal floatingUnderflowMask : 1;
            Ordinal floatingInvalidOpMask : 1;
            Ordinal floatingZeroDivideMask : 1;
            Ordinal floatingInexactMask : 1;
            Ordinal floatingPointNormalizingMode : 1;
            Ordinal floatingPointRoundingControl : 2;
#else
        Ordinal unused3 : 16;
#endif // end defined(NUMERICS_ARCHITECTURE)
    };

};
static_assert (sizeof(ArithmeticControls) == sizeof(Ordinal), "ArithmeticControls must be the width of a single Ordinal");

constexpr Ordinal modify(Ordinal mask, Ordinal src, Ordinal srcDest) noexcept {
    return (src & mask) | (srcDest & ~mask);
}


Ordinal
ArithmeticControls::modify(Ordinal mask, Ordinal src) noexcept {
    auto tmp = ord_;
    ord_ = ::modify(mask, src, ord_);
    return tmp;
}

union DoubleRegister {
public:
    constexpr explicit DoubleRegister(LongOrdinal value = 0) noexcept : lord_(value) { }
    constexpr auto getLongOrdinal() const noexcept { return lord_; }
    constexpr auto getLongInteger() const noexcept { return lint_; }
    constexpr auto getOrdinal(int which = 0) const noexcept { return parts_[which & 0b01]; }
    void setLongOrdinal(LongOrdinal value) noexcept { lord_ = value; }
    void setLongInteger(LongInteger value) noexcept { lint_ = value; }
    void setOrdinal(Ordinal value, int which = 0) noexcept { parts_[which & 0b01] = value; }

#ifdef NUMERICS_ARCHITECTURE
    constexpr auto getLongReal() const noexcept { return lreal_; }
    void setLongReal(LongReal value) noexcept { lreal_ = value; }
#endif
private:
    LongOrdinal lord_ = 0;
    LongInteger lint_;
    Ordinal parts_[sizeof(LongOrdinal)/ sizeof(Ordinal)];
#ifdef NUMERICS_ARCHITECTURE
    LongReal lreal_;
#endif
};

union TripleRegister {
public:
    constexpr explicit TripleRegister(Ordinal a = 0, Ordinal b = 0, Ordinal c = 0) noexcept : parts_{a, b, c, 0}{ }
    constexpr auto getOrdinal(int which = 0) const noexcept { return parts_[which % 3]; } // very expensive!
    void setOrdinal(Ordinal value, int which = 0) noexcept { parts_[which % 3] = value; }
#ifdef NUMERICS_ARCHITECTURE
    constexpr auto getExtendedReal() const noexcept { return lreal_; }
    void setExtendedReal(LongReal value) noexcept { lreal_ = value; }
#endif
private:
    Ordinal parts_[4]; // the fourth Ordinal is for alignment purposes, there is no way to modify it through the class
#ifdef NUMERICS_ARCHITECTURE
    ExtendedReal lreal_;
#endif
};

union QuadRegister {
public:
    constexpr explicit QuadRegister(Ordinal a = 0, Ordinal b = 0, Ordinal c = 0, Ordinal d = 0) noexcept : parts_{a, b, c, d}{ }
    constexpr explicit QuadRegister(LongOrdinal lower, LongOrdinal upper) noexcept : halves_{lower, upper} { }
    constexpr auto getOrdinal(int which = 0) const noexcept { return parts_[which & 0b11]; } // very expensive!
    void setOrdinal(Ordinal value, int which = 0) noexcept { parts_[which & 0b11] = value; }
    constexpr auto getHalf(int which = 0) const noexcept { return halves_[which & 0b01];}
    void setHalf(LongOrdinal value, int which = 0) noexcept { halves_[which & 0b01] = value; }
#ifdef NUMERICS_ARCHITECTURE
    constexpr auto getExtendedReal() const noexcept { return lreal_; }
void setExtendedReal(LongReal value) noexcept { lreal_ = value; }
#endif
private:
    Ordinal parts_[4];
    LongOrdinal halves_[2];
#ifdef NUMERICS_ARCHITECTURE
    ExtendedReal lreal_;
#endif
};

union RegisterFrame {
    RegisterFrame() noexcept : gprs { Register(), Register(), Register(), Register(),
                                      Register(), Register(), Register(), Register(),
                                      Register(), Register(), Register(), Register(),
                                      Register(), Register(), Register(), Register(),
    } {

    }
    const Register& getRegister(int index) const noexcept { return gprs[index & 0b1111]; }
    Register& getRegister(int index) noexcept { return gprs[index & 0b1111]; }
    const DoubleRegister& getDoubleRegister(int index) const noexcept { return dprs[(index >> 1) & 0b111]; }
    DoubleRegister& getDoubleRegister(int index) noexcept { return dprs[(index >> 1) & 0b111]; }
    const TripleRegister& getTripleRegister(int index) const noexcept { return tprs[(index >> 2) & 0b11]; }
    TripleRegister& getTripleRegister(int index) noexcept { return tprs[(index >> 2) & 0b11]; }
    const QuadRegister& getQuadRegister(int index) const noexcept { return qprs[(index >> 2) & 0b11]; }
    QuadRegister& getQuadRegister(int index) noexcept { return qprs[(index >> 2) & 0b11]; }

    constexpr auto getNumRegisters() const noexcept { return 16; }
    constexpr auto getNumDoubleRegisters() const noexcept { return 8; }
    constexpr auto getNumTripleRegisters () const noexcept { return 4; }
    constexpr auto getNumQuadRegisters () const noexcept { return 4; }

    Register gprs[16];
    DoubleRegister dprs[sizeof(gprs)/sizeof(DoubleRegister)];
    TripleRegister tprs[sizeof(gprs)/sizeof(TripleRegister)]; // this will have the same alignment as quad registers by ignoring the fourth ordinal
    QuadRegister qprs[sizeof(gprs)/sizeof(QuadRegister)];
    // we put the extended reals in a different location
};


class Core {
public:
    using Address = Ordinal;
    static constexpr Register OrdinalLiterals[32] {
#define X(base) Register(base + 0), Register(base + 1), Register(base + 2), Register(base + 3)
            X(0),
            X(4),
            X(8),
            X(12),
            X(16),
            X(20),
            X(24),
            X(28),
#undef X
    };
    static constexpr DoubleRegister LongOrdinalLiterals[32] {
#define X(base) DoubleRegister(base + 0), DoubleRegister(base + 1), DoubleRegister(base + 2), DoubleRegister(base + 3)
            X(0),
            X(4),
            X(8),
            X(12),
            X(16),
            X(20),
            X(24),
            X(28),
#undef X
};
    static constexpr TripleRegister TripleOrdinalLiterals[32] {
#define X(base) TripleRegister(base + 0), TripleRegister(base + 1), TripleRegister(base + 2), TripleRegister(base + 3)
            X(0),
            X(4),
            X(8),
            X(12),
            X(16),
            X(20),
            X(24),
            X(28),
#undef X
};
    static constexpr QuadRegister QuadOrdinalLiterals[32] {
#define X(base) QuadRegister(base + 0), QuadRegister(base + 1), QuadRegister(base + 2), QuadRegister(base + 3)
            X(0),
            X(4),
            X(8),
            X(12),
            X(16),
            X(20),
            X(24),
            X(28),
#undef X
};
public:
public:
    explicit Core(Ordinal salign = 4) : ip_(0), ac_(0), salign_(salign), c_((salign * 16) - 1) { };
    virtual ~Core() = default;
protected:
    virtual void storeByte(Address destination, ByteOrdinal value) = 0;
    virtual void storeShort(Address destination, ShortOrdinal value) = 0;
    virtual void storeLong(Address destination, LongOrdinal value) = 0;
    virtual void storeWord(Address destination, Ordinal value) = 0;
    virtual void storeTriple(Address destination, const TripleRegister& reg) {
        storeWord(destination + 0, reg.getOrdinal(0));
        storeWord(destination + 4, reg.getOrdinal(1));
        storeWord(destination + 8, reg.getOrdinal(2));
    }
    virtual void storeQuad(Address destination, const QuadRegister& reg) {
        storeLong(destination + 0, reg.getHalf(0));
        storeLong(destination + 8, reg.getHalf(1));
    }
    virtual Ordinal load(Address destination) = 0;
    virtual ByteOrdinal loadByte(Address destination) = 0;
    virtual ShortOrdinal loadShort(Address destination) = 0;
    virtual LongOrdinal loadLong(Address destination) = 0;
    void load(Address destination, TripleRegister& reg) noexcept {
        reg.setOrdinal(load(destination + 0), 0);
        reg.setOrdinal(load(destination + 4), 1);
        reg.setOrdinal(load(destination + 8), 2);
    }
    void load(Address destination, QuadRegister& reg) noexcept {
        reg.setHalf(loadLong(destination + 0), 0);
        reg.setHalf(loadLong(destination + 8), 1);
    }
    Register& getRegister(RegisterIndex targetIndex);
    const Register& getRegister(RegisterIndex targetIndex) const;
    DoubleRegister& getDoubleRegister(RegisterIndex targetIndex);
    const DoubleRegister& getDoubleRegister(RegisterIndex targetIndex) const;
    TripleRegister& getTripleRegister(RegisterIndex targetIndex);
    const TripleRegister& getTripleRegister(RegisterIndex targetIndex) const;
    QuadRegister& getQuadRegister(RegisterIndex targetIndex);
    const QuadRegister& getQuadRegister(RegisterIndex targetIndex) const;
    Register& getStackPointer() noexcept {
        return getRegister(RegisterIndex::SP);
    }
    const Register& getStackPointer() const noexcept {
        return getRegister(RegisterIndex::SP);
    }
    Register& getFramePointer() noexcept {
        return getRegister(RegisterIndex::FP);
    }
    const Register& getFramePointer() const noexcept {
        return getRegister(RegisterIndex::FP);
    }
    Register& getPFP() noexcept {
        return getRegister(RegisterIndex::PFP);
    }
    const Register& getPFP() const noexcept {
        return getRegister(RegisterIndex::PFP);
    }
    Register& getRIP() noexcept {
        return getRegister(RegisterIndex::RIP);
    }
    const Register& getRIP() const noexcept {
        return getRegister(RegisterIndex::RIP);
    }

private:
    void ipRelativeBranch(Integer displacement) noexcept {
        advanceIPBy = 0;
        ip_.setInteger(ip_.getInteger() + displacement);
    }
    Instruction loadInstruction(Address baseAddress) noexcept;
    void executeInstruction(const Instruction& instruction) noexcept;
    void generateFault(ByteOrdinal faultType, ByteOrdinal faultSubtype) noexcept;
    void cmpi(Integer src1, Integer src2) noexcept;
    void cmpo(Ordinal src1, Ordinal src2) noexcept;
    void cycle() noexcept;
private:
    void saveRegisterFrame(const RegisterFrame& theFrame, Address baseAddress) noexcept;
    inline void saveLocals(Address baseAddress) noexcept { saveRegisterFrame(locals, baseAddress); }
    void restoreRegisterFrame(RegisterFrame& theFrame, Address baseAddress) noexcept;
    inline void restoreLocals(Address baseAddress) noexcept { restoreRegisterFrame(locals, baseAddress); }
    Ordinal computeMemoryAddress(const Instruction& instruction) noexcept;
private:
    Register ip_; // start at address zero
    ArithmeticControls ac_;
    RegisterFrame locals;
    RegisterFrame globals;
#ifdef NUMERICS_ARCHITECTURE
    ExtendedReal fpRegs[4] = { 0 };
#endif
    Ordinal advanceIPBy = 0;
    Ordinal salign_;
    Ordinal c_;
    bool executing_ = false;
};

void
Core::cycle() noexcept {
    advanceIPBy = 4;
    executeInstruction(loadInstruction(ip_.getOrdinal()));
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
    return Instruction(loadLong(baseAddress & (~static_cast<Address>(0b111))));
}

void
Core::saveRegisterFrame(const RegisterFrame &theFrame, Address baseAddress) noexcept {
    for (int i = 0; i < 16; ++i, baseAddress += 4) {
        storeWord(baseAddress, theFrame.getRegister(i).getOrdinal());
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
Core::generateFault(ByteOrdinal faultType, ByteOrdinal faultSubtype) noexcept {

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
            return instruction.getOffset() + getRegister(instruction.getABase()).getOrdinal();
        case MEMFormatMode::MEMB_RegisterIndirect:
            return getRegister(instruction.getABase()).getOrdinal();
        case MEMFormatMode::MEMB_RegisterIndirectWithIndex:
            return getRegister(instruction.getABase()).getOrdinal() +
                   (getRegister(instruction.getIndex()).getOrdinal() << instruction.getScale());
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
    static constexpr Ordinal bitPositions[32] {
#define X(base) 1u << (base + 0), 1u << (base + 1), 1u << (base + 2), 1u << (base + 3)
            X(0), X(4), X(8), X(12),
            X(16), X(20), X(24), X(28)
#undef X
    };
    auto cmpobx = [this, &instruction](uint8_t mask) noexcept {
        auto src1 = getRegister(instruction.getSrc1()).getOrdinal();
        auto src2 = getRegister(instruction.getSrc2()).getOrdinal();
        cmpo(src1, src2);
        advanceIPBy = 0;
        if ((mask & ac_.getConditionCode()) != 0) {
            // while the docs show (displacement * 4), I am currently including the bottom two bits being forced to zero in displacement
            // in the future (the HX uses those two bits as "S2" so that will be a fun future change...)
            ip_.setInteger(ip_.getInteger() + 4 + instruction.getDisplacement());
        } else {
            ip_.setOrdinal(ip_.getOrdinal() + 4);
        }
    };
    auto cmpibx = [this, &instruction](uint8_t mask) noexcept {
        auto src1 = getRegister(instruction.getSrc1()).getInteger();
        auto src2 = getRegister(instruction.getSrc2()).getInteger();
        cmpi(src1, src2);
        advanceIPBy = 0;
        if ((mask & ac_.getConditionCode()) != 0) {
            // while the docs show (displacement * 4), I am currently including the bottom two bits being forced to zero in displacement
            // in the future (the HX uses those two bits as "S2" so that will be a fun future change...)
            ip_.setInteger(ip_.getInteger() + 4 + instruction.getDisplacement());
        } else {
            ip_.setOrdinal(ip_.getOrdinal() + 4);
        }
    };
    auto condBranch = [this, &instruction](uint8_t mask) {
        if ((ac_.getConditionCode()& mask) != 0) {
            ipRelativeBranch(instruction.getDisplacement()) ;
        }
    };
    auto condFault = [this](uint8_t mask) {
        if ((ac_.getConditionCode()& mask) != 0) {
            /// @todo constraint range fault
            generateFault(0, 0);
        }
    };
    switch (instruction.identifyOpcode()) {
        case Opcode::None:
        case Opcode::Bad:
            ipRelativeBranch(instruction.getDisplacement()) ;
            /// @todo generate fault here
            break;
        // CTRL Format opcodes
        case Opcode::b:
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
                /// @todo make target constraint range fault
                generateFault(0, 0);
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
                getRegister(instruction.getSrc1()).setOrdinal(ac_.conditionCodeIs<0b000>() ? 1 : 0);
            }();
            break;
        case Opcode::testg:
            [this, &instruction]() {
                getRegister(instruction.getSrc1()).setOrdinal(ac_.conditionCodeIs<0b001>() ? 1 : 0);

            }();
            break;
        case Opcode::teste:
            [this, &instruction]() {
                getRegister(instruction.getSrc1()).setOrdinal(ac_.conditionCodeIs<0b010>() ? 1 : 0);

            }();
            break;
        case Opcode::testge:
            [this, &instruction]() {
                getRegister(instruction.getSrc1()).setOrdinal(ac_.conditionCodeIs<0b011>() ? 1 : 0);

            }();
            break;
        case Opcode::testl:
            [this, &instruction]() {
                getRegister(instruction.getSrc1()).setOrdinal(ac_.conditionCodeIs<0b100>() ? 1 : 0);
            }();
            break;
        case Opcode::testne:
            [this, &instruction]() {
                getRegister(instruction.getSrc1()).setOrdinal(ac_.conditionCodeIs<0b101>() ? 1 : 0);
            }();
            break;
        case Opcode::testle:
            [this, &instruction]() {
                getRegister(instruction.getSrc1()).setOrdinal(ac_.conditionCodeIs<0b110>() ? 1 : 0);
            }();
            break;
        case Opcode::testo:
            [this, &instruction]() {
                getRegister(instruction.getSrc1()).setOrdinal(ac_.conditionCodeIs<0b111>() ? 1 : 0);
            }();
            break;
        case Opcode::bbc:
            // branch if bit is clear
            [this, &instruction]() {
                auto bitpos = bitPositions[getRegister(instruction.getSrc1()).getOrdinal() & 0b11111];
                auto src = getRegister(instruction.getSrc2()).getOrdinal();
                advanceIPBy = 0;
                if ((bitpos & src) == 0) {
                    ac_.setConditionCode(0b010);
                    // while the docs show (displacement * 4), I am currently including the bottom two bits being forced to zero in displacement
                    // in the future (the HX uses those two bits as "S2" so that will be a fun future change...)
                    ip_.setInteger(ip_.getInteger() + 4 + instruction.getDisplacement());
                } else {
                    ac_.setConditionCode(0b000);
                    ip_.setOrdinal(ip_.getOrdinal() + 4);
                }
            }();
            break;
        case Opcode::bbs:
            [this, &instruction]() {

                auto bitpos = bitPositions[getRegister(instruction.getSrc1()).getOrdinal() & 0b11111];
                auto src = getRegister(instruction.getSrc2()).getOrdinal();
                advanceIPBy = 0;
                if ((bitpos & src) != 0) {
                    ac_.setConditionCode(0b010);
                    // while the docs show (displacement * 4), I am currently including the bottom two bits being forced to zero in displacement
                    // in the future (the HX uses those two bits as "S2" so that will be a fun future change...)
                    ip_.setInteger(ip_.getInteger() + 4 + instruction.getDisplacement());
                } else {
                    ac_.setConditionCode(0b000);
                    ip_.setOrdinal(ip_.getOrdinal() + 4);
                }
            }();
            break;
        case Opcode::cmpo:
            [this, &instruction]() {
                cmpo(getRegister(instruction.getSrc1()).getOrdinal(),
                     getRegister(instruction.getSrc2()).getOrdinal());
            }();
            break;
        case Opcode::cmpi:
            [this, &instruction]() {
                cmpi(getRegister(instruction.getSrc1()).getInteger(),
                     getRegister(instruction.getSrc2()).getInteger());
            }();
            break;
        case Opcode::cmpdeco:
            [this, &instruction]() {
                auto src2 = getRegister(instruction.getSrc2()).getOrdinal();
                cmpo(getRegister(instruction.getSrc1()).getOrdinal(), src2);
                getRegister(instruction.getSrcDest(false)).setOrdinal(src2 - 1);
            }();
            break;
        case Opcode::cmpdeci:
            [this, &instruction]() {
                auto src2 = getRegister(instruction.getSrc2()).getInteger();
                cmpi(getRegister(instruction.getSrc1()).getInteger(), src2);
                getRegister(instruction.getSrcDest(false)).setInteger(src2 - 1);
            }();
            break;
        case Opcode::cmpinco:
            [this, &instruction]() {
                auto src2 = getRegister(instruction.getSrc2()).getOrdinal();
                cmpo(getRegister(instruction.getSrc1()).getOrdinal(), src2);
                getRegister(instruction.getSrcDest(false)).setOrdinal(src2 + 1);
            }();
            break;
        case Opcode::cmpinci:
            [this, &instruction]() {
                auto src2 = getRegister(instruction.getSrc2()).getInteger();
                cmpi(getRegister(instruction.getSrc1()).getInteger(), src2);
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
                dest.setOrdinal(loadByte(computeMemoryAddress(instruction)));
            }();
            break;
        case Opcode::bx:
            [this, &instruction]() {
                advanceIPBy = 0;
                ip_.setOrdinal(computeMemoryAddress(instruction));
            }();
            break;
        case Opcode::balx:
            [this, &instruction]() {
                auto& g14 = getRegister(RegisterIndex::Global14);
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
                dest.setOrdinal(computeMemoryAddress(instruction));
            }();
            break;
        case Opcode::ld:
            [this, &instruction]() {
                auto& dest = getRegister(instruction.getSrcDest(false));
                dest.setOrdinal(load(computeMemoryAddress(instruction)));
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
                getRegister(instruction.getSrcDest(false)).setInteger(getRegister(instruction.getSrc2()).getInteger() +
                                                                      getRegister(instruction.getSrc1()).getInteger());
            }( );
            break;
        case Opcode::addo:
            [this, &instruction]() {
                getRegister(instruction.getSrcDest(false)).setOrdinal(getRegister(instruction.getSrc2()).getOrdinal() +
                                                                      getRegister(instruction.getSrc1()).getOrdinal());
            }();
            break;
        case Opcode::subi:
            [this, &instruction]() {
                getRegister(instruction.getSrcDest(false)).setInteger(
                        getRegister(instruction.getSrc2()).getInteger() - getRegister(instruction.getSrc1()).getInteger());
            }();
            break;
        case Opcode::subo:
            [this, &instruction]() {
                getRegister(instruction.getSrcDest(false)).setOrdinal(
                        getRegister(instruction.getSrc2()).getOrdinal() - getRegister(instruction.getSrc1()).getOrdinal());
            }();
            break;
        case Opcode::muli:
            [this, &instruction]() {
                getRegister(instruction.getSrcDest(false)).setInteger(
                        getRegister(instruction.getSrc2()).getInteger() * getRegister(instruction.getSrc1()).getInteger());
            }();
            break;
        case Opcode::mulo:
            [this, &instruction]() {
                getRegister(instruction.getSrcDest(false)).setOrdinal(
                        getRegister(instruction.getSrc2()).getOrdinal() * getRegister(instruction.getSrc1()).getOrdinal());
            }();
            break;
        case Opcode::divo:
            [this, &instruction]() {
                /// @todo check denominator and do proper handling
                getRegister(instruction.getSrcDest(false)).setOrdinal(
                        getRegister(instruction.getSrc2()).getOrdinal() / getRegister(instruction.getSrc1()).getOrdinal());
            }();
            break;
        case Opcode::divi:
            [this, &instruction]() {
                /// @todo check denominator and do proper handling
                getRegister(instruction.getSrcDest(false)).setInteger(
                        getRegister(instruction.getSrc2()).getInteger() / getRegister(instruction.getSrc1()).getInteger());
            }();
            break;
        case Opcode::notbit:
            [this, &instruction]() {
                auto bitpos = bitPositions[getRegister(instruction.getSrc1()).getOrdinal() & 0b11111];
                auto src = getRegister(instruction.getSrc2()).getOrdinal();
                auto& dest = getRegister(instruction.getSrcDest(false));
                dest.setOrdinal(src ^ bitpos);
            }();
            break;
        case Opcode::setbit:
            [this, &instruction]() {
            }();
            break;
        case Opcode::clrbit:
            [this, &instruction]() {
            }();
            break;
        case Opcode::logicalAnd: [this, &instruction]() { getRegister(instruction.getSrcDest(false)).setOrdinal(getRegister(instruction.getSrc2()).getOrdinal() & getRegister(instruction.getSrc1()).getOrdinal()); }(); break;
        case Opcode::logicalOr: [this, &instruction]() { getRegister(instruction.getSrcDest(false)).setOrdinal(getRegister(instruction.getSrc2()).getOrdinal() | getRegister(instruction.getSrc1()).getOrdinal()); }(); break;
        case Opcode::logicalXor: [this, &instruction]() { getRegister(instruction.getSrcDest(false)).setOrdinal(getRegister(instruction.getSrc2()).getOrdinal() ^ getRegister(instruction.getSrc1()).getOrdinal()); }(); break;
        case Opcode::logicalXnor: [this, &instruction]() { getRegister(instruction.getSrcDest(false)).setOrdinal(~(getRegister(instruction.getSrc2()).getOrdinal() ^ getRegister(instruction.getSrc1()).getOrdinal())); }(); break;
        case Opcode::logicalNor: [this, &instruction]() { getRegister(instruction.getSrcDest(false)).setOrdinal(~(getRegister(instruction.getSrc2()).getOrdinal() | getRegister(instruction.getSrc1()).getOrdinal())); }(); break;
        case Opcode::logicalNand: [this, &instruction]() { getRegister(instruction.getSrcDest(false)).setOrdinal(~(getRegister(instruction.getSrc2()).getOrdinal() & getRegister(instruction.getSrc1()).getOrdinal())); }(); break;
        case Opcode::logicalNot: [this, &instruction]() { getRegister(instruction.getSrcDest(false)).setOrdinal(~(getRegister(instruction.getSrc1()).getOrdinal())); }(); break;
        case Opcode::andnot: [this, &instruction]() { getRegister(instruction.getSrcDest(false)).setOrdinal(getRegister(instruction.getSrc2()).getOrdinal() & ~getRegister(instruction.getSrc1()).getOrdinal()); }(); break;
        case Opcode::notand: [this, &instruction]() { getRegister(instruction.getSrcDest(false)).setOrdinal(~getRegister(instruction.getSrc2()).getOrdinal() & getRegister(instruction.getSrc1()).getOrdinal()); }(); break;
        case Opcode::ornot: [this, &instruction]() { getRegister(instruction.getSrcDest(false)).setOrdinal(getRegister(instruction.getSrc2()).getOrdinal() | ~getRegister(instruction.getSrc1()).getOrdinal()); }(); break;
        case Opcode::notor: [this, &instruction]() { getRegister(instruction.getSrcDest(false)).setOrdinal(~getRegister(instruction.getSrc2()).getOrdinal() | getRegister(instruction.getSrc1()).getOrdinal()); }(); break;
        case Opcode::remi:
            [this, &instruction]() {
                auto& dest = getRegister(instruction.getSrcDest(false));
                auto src2 = getRegister(instruction.getSrc2()).getInteger();
                auto src1 = getRegister(instruction.getSrc1()).getInteger();
                // taken from the i960Sx manual
                dest.setInteger(src2 - ((src2 / src1) * src1));
            }();
            break;
        case Opcode::remo:
            [this, &instruction]() {
                auto& dest = getRegister(instruction.getSrcDest(false));
                auto src2 = getRegister(instruction.getSrc2()).getOrdinal();
                auto src1 = getRegister(instruction.getSrc1()).getOrdinal();
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
                auto src = getRegister(instruction.getSrc2()).getOrdinal();
                auto len = getRegister(instruction.getSrc1()).getOrdinal();
                dest.setOrdinal(rotateOperation(src, len));
            }();
            break;
        case Opcode::mov:
            [this, &instruction]() {
                auto& dest = getRegister(instruction.getSrcDest(false));
                dest.setOrdinal(getRegister(instruction.getSrc1()).getOrdinal());
            }();
            break;
        case Opcode::movl:
            [this, &instruction]() {
                auto& dest = getDoubleRegister(instruction.getSrcDest(false));
                dest.setLongOrdinal(getDoubleRegister(instruction.getSrc1()).getLongOrdinal());
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
                    generateFault(0, 0); /// @todo flesh this out
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
                auto src2 = static_cast<LongOrdinal>(getRegister(instruction.getSrc2()).getOrdinal());
                auto src1 = static_cast<LongOrdinal>(getRegister(instruction.getSrc2()).getOrdinal());
                auto& dest = getDoubleRegister(instruction.getSrcDest(false));
                // taken from the manual
                dest.setLongOrdinal(src2 * src1);
            }();
            break;
        case Opcode::extract:
            [this, &instruction]() {
                auto& dest = getRegister(instruction.getSrcDest(false));
                auto bitpos = getRegister(instruction.getSrc1()).getOrdinal();
                auto len = getRegister(instruction.getSrc2()).getOrdinal();
                // taken from the Hx manual as it isn't insane
                auto shiftAmount = bitpos > 32 ? 32 : bitpos;
                dest.setOrdinal((dest.getOrdinal() >> shiftAmount) & ~(0xFFFF'FFFF << len));
            }();
            break;
        case Opcode::flushreg:
            [this, &instruction]() {
                /// @todo expand this instruction to dump saved register sets to stack in the right places
                // currently this does nothing because I haven't implemented the register frame stack yet
            }();
            break;
        case Opcode::fmark:
            [this, &instruction]() {
                // Generates a breakpoint trace-event. This instruction causes a breakpoint trace-event to be generated, regardless of the
                // setting of the breakpoint trace mode flag (to be implemented), providing the trace-enable bit (bit 0) of the process
                // controls is set.

                // if pc.te == 1 then raiseFault(BreakpointTraceFault)
                /// @todo implement
            }();
            break;
        case Opcode::mark:
            [this, &instruction]() {
                // Generates a breakpoint trace-event if the breakpoint trace mode has been enabled.
                // The breakpoint trace mode is enabled if the trace-enable bit (bit 0) of the process
                // controls and the breakpoint-trace mode bit (bit 7) of the trace controls have been zet

                // if pc.te == 1 && breakpoint_trace_flag then raise trace breakpoint fault
                /// @todo implement
            }();
                break;

        case Opcode::modac:
            [this, &instruction]() {
                auto& dest = getRegister(instruction.getSrcDest(false));
                auto mask = getRegister(instruction.getSrc1()).getOrdinal();
                auto src = getRegister(instruction.getSrc2()).getOrdinal();
                dest.setOrdinal(ac_.modify(mask, src));
            }( );
            break;
        case Opcode::modi:
            [this, &instruction]() {
                auto denominator = getRegister(instruction.getSrc1()) .getInteger();
                if (denominator == 0) {
                    generateFault(0, 0); /// @todo make this an Arithmetic Zero Divide
                } else {
                    auto numerator = getRegister(instruction.getSrc2()).getInteger();
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
                auto temp = (getStackPointer().getOrdinal() + c_) & ~c_; // round to next boundary
                auto fp = getFramePointer().getOrdinal();
                getRIP().setOrdinal(ip_.getOrdinal());
                /// @todo implement support for caching register frames
                saveRegisterFrame(locals, getFramePointer().getOrdinal());
                ip_.setInteger(ip_.getInteger() + instruction.getDisplacement());
                /// @todo expand pfp and fp to accurately model how this works
                getPFP().setOrdinal(fp);
                getFramePointer().setOrdinal(temp);
                getStackPointer().setOrdinal(temp + 64);
            }();
            break;
        case Opcode::callx:
            [this, &instruction]() {
                // wait for any uncompleted instructions to finish
                auto temp = (getStackPointer().getOrdinal() + c_) & ~c_; // round to next boundary
                auto fp = getFramePointer().getOrdinal();
                getRIP().setOrdinal(ip_.getOrdinal());
                /// @todo implement support for caching register frames
                saveRegisterFrame(locals, getFramePointer().getOrdinal());
                ip_.setInteger(computeMemoryAddress(instruction));
                getPFP().setOrdinal(fp);
                getFramePointer().setOrdinal(temp);
                getStackPointer().setOrdinal(temp + 64);
            }();
            break;
        case Opcode::shlo:
            [this, &instruction]() {
                auto& dest = getRegister(instruction.getSrcDest(false));
                auto len = getRegister(instruction.getSrc1()).getOrdinal();
                if (len < 32) {
                    auto src = getRegister(instruction.getSrc2()).getOrdinal();
                    dest.setOrdinal(src << len);
                } else {
                    dest.setOrdinal(0);
                }
            }();
            break;
        case Opcode::shro:
            [this, &instruction]() {
                auto& dest = getRegister(instruction.getSrcDest(false));
                auto len = getRegister(instruction.getSrc1()).getOrdinal();
                if (len < 32) {
                    auto src = getRegister(instruction.getSrc2()).getOrdinal();
                    dest.setOrdinal(src >> len);
                } else {
                    dest.setOrdinal(0);
                }
            }();
            break;
        case Opcode::shli:
            [this, &instruction]() {
                auto& dest = getRegister(instruction.getSrcDest(false));
                auto len = getRegister(instruction.getSrc1()).getInteger();
                auto src = getRegister(instruction.getSrc2()).getInteger();
                dest.setInteger(src << len);
            }();
            break;
        case Opcode::scanbyte:
            [this, &instruction]() {
                auto& dest = getRegister(instruction.getSrcDest(false));
                auto& src1 = getRegister(instruction.getSrc1());
                auto& src2 = getRegister(instruction.getSrc2());
                auto bytesEqual = [&src1, &src2](int which) constexpr { return src1.getByteOrdinal(which) == src2.getByteOrdinal(which); };
                ac_.setConditionCode((bytesEqual(0) || bytesEqual(1) || bytesEqual(2) || bytesEqual(3)) ? 0b010 : 0b000);


            }();
            break;
    }
}



int main(int argc, char** argv) {
    return 0;
}