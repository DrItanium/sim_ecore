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
        constexpr uint8_t scaleFactors[8] { 1, 2, 4, 8, 16, 0, 0, 0, };
        if (isMEMBFormat()) {
            return scaleFactors[memb.scale];
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
        Integer displacement: 13;
        Ordinal m1: 1;
        Ordinal src2: 5;
        Ordinal src1: 5;
        Ordinal opcode: 8;
    } cobr;
    struct {
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
public:
    Core() : ip_(0), ac_(0) { };
    virtual ~Core() = default;
    virtual void storeByte(Address destination, ByteOrdinal value) = 0;
    virtual void storeShort(Address destination, ShortOrdinal value) = 0;
    virtual void storeLong(Address destination, LongOrdinal value) = 0;
    virtual void storeWord(Address destination, Ordinal value) = 0;
    virtual void storeTriple(Address destination, Ordinal lowest, Ordinal middle, Ordinal highest) {
        storeWord(destination + 0, lowest);
        storeWord(destination + 4, middle);
        storeWord(destination + 8, highest);
    }
    virtual void storeQuad(Address destination, Ordinal a, Ordinal b, Ordinal c, Ordinal d) {
        storeWord(destination + 0, a);
        storeWord(destination + 4, b);
        storeWord(destination + 8, c);
        storeWord(destination + 12, d);
    }
    virtual Ordinal load(Address destination) = 0;
    virtual ByteOrdinal loadByte(Address destination) = 0;
    virtual ShortOrdinal loadShort(Address destination) = 0;
    virtual LongOrdinal loadLong(Address destination) = 0;
    Register& getRegister(RegisterIndex targetIndex);
    const Register& getRegister(RegisterIndex targetIndex) const;
private:
    Instruction loadInstruction(Address baseAddress) noexcept;
    void executeInstruction(const Instruction& instruction) noexcept;
private:
    void saveRegisterFrame(const RegisterFrame& theFrame, Address baseAddress) noexcept;
    inline void saveLocals(Address baseAddress) noexcept { saveRegisterFrame(locals, baseAddress); }
    void restoreRegisterFrame(RegisterFrame& theFrame, Address baseAddress) noexcept;
    inline void restoreLocals(Address baseAddress) noexcept { restoreRegisterFrame(locals, baseAddress); }
private:
    Register ip_; // start at address zero
    ArithmeticControls ac_;
    RegisterFrame locals;
    RegisterFrame globals;
#ifdef NUMERICS_ARCHITECTURE
    ExtendedReal fpRegs[4] = { 0 };
#endif
};

Register&
Core::getRegister(RegisterIndex targetIndex) {
    if (isLocalRegister(targetIndex)) {
        return locals.gprs[static_cast<uint8_t>(targetIndex) & 0b1111];
    } else if (isGlobalRegister(targetIndex)) {
        return globals.gprs[static_cast<uint8_t>(targetIndex) & 0b1111];
    } else if (isLiteral(targetIndex)) {
        throw "Literals cannot be modified";
    } else {
        throw "Illegal register requested";
    }
}

const Register&
Core::getRegister(RegisterIndex targetIndex) const {
    if (isLocalRegister(targetIndex)) {
        return locals.gprs[static_cast<uint8_t>(targetIndex) & 0b1111];
    } else if (isGlobalRegister(targetIndex)) {
        return globals.gprs[static_cast<uint8_t>(targetIndex) & 0b1111];
    } else if (isLiteral(targetIndex)) {
        return OrdinalLiterals[static_cast<uint8_t>(targetIndex) & 0b11111];
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
Core::executeInstruction(const Instruction &instruction) noexcept {
    switch (instruction.identifyOpcode()) {
        // REG format
        case Opcode::addi:
            [this, &instruction]() {
                getRegister(instruction.getSrcDest(false)).setInteger(getRegister(instruction.getSrc2()).getInteger() +
                                                                      getRegister(instruction.getSrc1()).getInteger());
            }();
        case Opcode::addo:
            [this, &instruction]() {
                getRegister(instruction.getSrcDest(false)).setOrdinal(getRegister(instruction.getSrc2()).getOrdinal() +
                                                                      getRegister(instruction.getSrc1()).getOrdinal());
            }();
            break;
            // CTRL Format opcodes
        case Opcode::b:
            [this, &instruction]() {

            }();
            break;
        case Opcode::call:
            [this, &instruction]() {

            }();
            break;
        case Opcode::ret:
            [this, &instruction]() {

            }();
            break;
        case Opcode::bal:
            [this, &instruction]() {

            }();
            break;
        case Opcode::bno:
            [this, &instruction]() {

            }();
            break;
        case Opcode::bg:
            [this, &instruction]() {

            }();
            break;
        case Opcode::be:
            [this, &instruction]() {

            }();
            break;
        case Opcode::bge:
            [this, &instruction]() {

            }();
            break;
        case Opcode::bl:
            [this, &instruction]() {

            }();
            break;
        case Opcode::bne:
            [this, &instruction]() {

            }();
            break;
        case Opcode::ble:
            [this, &instruction]() {

            }();
            break;
        case Opcode::bo:
            [this, &instruction]() {

            }();
            break;
        case Opcode::faultno:
            [this, &instruction]() {

            }();
            break;
        case Opcode::faultg:
            [this, &instruction]() {

            }();
            break;
        case Opcode::faulte:
            [this, &instruction]() {

            }();
            break;
        case Opcode::faultge:
            [this, &instruction]() {

            }();
            break;
        case Opcode::faultl:
            [this, &instruction]() {

            }();
            break;
        case Opcode::faultne:
            [this, &instruction]() {

            }();
            break;
        case Opcode::faultle:
            [this, &instruction]() {

            }();
            break;
        case Opcode::faulto:
            [this, &instruction]() {

            }();
            break;
            // COBR Format
        case Opcode::testno:
            [this, &instruction]() {

            }();
            break;
        case Opcode::testg:
            [this, &instruction]() {

            }();
            break;
        case Opcode::teste:
            [this, &instruction]() {

            }();
            break;
        case Opcode::testge:
            [this, &instruction]() {

            }();
            break;
        case Opcode::testl:
            [this, &instruction]() {

            }();
            break;
        case Opcode::testne:
            [this, &instruction]() {

            }();
            break;
        case Opcode::testle:
            [this, &instruction]() {

            }();
            break;
        case Opcode::testo:
            [this, &instruction]() {

            }();
            break;
        case Opcode::bbc:
            [this, &instruction]() {

            }();
            break;
        case Opcode::bbs:
            [this, &instruction]() {

            }();
            break;
        case Opcode::cmpobg:
            [this, &instruction]() {

            }();
            break;
        case Opcode::cmpobe:
            [this, &instruction]() {

            }();
            break;
        case Opcode::cmpobge:
            [this, &instruction]() {

            }();
            break;
        case Opcode::cmpobl:
            [this, &instruction]() {

            }();
            break;
        case Opcode::cmpobne:
            [this, &instruction]() {

            }();
            break;
        case Opcode::cmpoble:
            [this, &instruction]() {

            }();
            break;
        case Opcode::cmpibno:
            [this, &instruction]() {

            }();
            break;
        case Opcode::cmpibg:
            [this, &instruction]() {

            }();
            break;
        case Opcode::cmpibe:
            [this, &instruction]() {

            }();
            break;
        case Opcode::cmpibge:
            [this, &instruction]() {

            }();
            break;
        case Opcode::cmpibl:
            [this, &instruction]() {

            }();
            break;
        case Opcode::cmpibne:
            [this, &instruction]() {

            }();
            break;
        case Opcode::cmpible:
            [this, &instruction]() {

            }();
            break;
        case Opcode::cmpibo:
            [this, &instruction]() {

            }();
            break;
            // MEM Format
        case Opcode::ldob:
            [this, &instruction]() {

            }();
            break;
        case Opcode::stob:
            [this, &instruction]() {

            }();
            break;
        case Opcode::bx:
            [this, &instruction]() {

            }();
            break;
        case Opcode::balx:
            [this, &instruction]() {

            }();
            break;
        case Opcode::callx:
            [this, &instruction]() {

            }();
            break;
        case Opcode::ldos:
            [this, &instruction]() {

            }();
            break;
        case Opcode::stos:
            [this, &instruction]() {

            }();
            break;
        case Opcode::lda:
            [this, &instruction]() {

            }();
            break;
        case Opcode::ld:
            [this, &instruction]() {

            }();
            break;
        case Opcode::st:
            [this, &instruction]() {

            }();
            break;
        case Opcode::ldl:
            [this, &instruction]() {

            }();
            break;
        case Opcode::stl:
            [this, &instruction]() {

            }();
            break;
        case Opcode::ldt:
            [this, &instruction]() {

            }();
            break;
        case Opcode::stt:
            [this, &instruction]() {

            }();
            break;
        case Opcode::ldq:
            [this, &instruction]() {

            }();
            break;
        case Opcode::stq:
            [this, &instruction]() {

            }();
            break;
        case Opcode::ldib:
            [this, &instruction]() {

            }();
            break;
        case Opcode::stib:
            [this, &instruction]() {

            }();
            break;
        case Opcode::ldis:
            [this, &instruction]() {

            }();
            break;
        case Opcode::stis:
            [this, &instruction]() {

            }();
            break;
        default:
            /// @todo implement fault invocation
            break;
    }
}



int main(int argc, char** argv) {
    return 0;
}