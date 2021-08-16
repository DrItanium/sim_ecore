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

using Real = float;
using LongReal = double;
using ExtendedReal = long double;

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

constexpr Ordinal getLiteralOrdinal(RegisterIndex index) noexcept {
    if (isLiteral(index)) {
        return static_cast<Ordinal>(static_cast<uint8_t>(index) & 0b11111);
    } else {
        return 0xFFFF'FFFF;
    }
}


static_assert(toFloatingPointForm(RegisterIndex::Bad) == RegisterIndex::Bad, "toFloatingPointForm is not passing through correctly!");

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
        constexpr explicit Instruction(Ordinal lower, Ordinal upper = 0) noexcept : parts{lower, upper} { }
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
        constexpr auto getReal() const noexcept { return real_; }
        constexpr auto getShortOrdinal(int which = 0) const noexcept { return sords_[which&0b01]; }
        constexpr auto getShortInteger(int which = 0) const noexcept { return sints_[which&0b01]; }
        constexpr auto getByteOrdinal(int which = 0) const noexcept { return bords_[which&0b11]; }
        constexpr auto getByteInteger(int which = 0) const noexcept { return bints_[which&0b11]; }
        void setOrdinal(Ordinal value) noexcept { ord_ = value; }
        void setInteger(Integer value) noexcept { integer_ = value; }
        void setReal(Real value) noexcept { real_ = value; }
        void setShortOrdinal(ShortOrdinal value) noexcept { setOrdinal(value); }
        void setShortInteger(ShortInteger value) noexcept { setInteger(value); }
        void setByteOrdinal(ByteOrdinal value) noexcept { setOrdinal(value); }
        void setByteInteger(ByteInteger value) noexcept { setInteger(value); }
        void setShortOrdinal(ShortOrdinal value, int which) noexcept { sords_[which&0b01] = value; }
        void setShortInteger(ShortInteger value, int which) noexcept { sints_[which&0b01] = value; }
        void setByteOrdinal(ByteOrdinal value, int which) noexcept { bords_[which&0b11] = value; }
        void setByteInteger(ByteInteger value, int which) noexcept { bints_[which&0b11] = value; }
    private:
        Ordinal ord_ = 0;
        Integer integer_;
        Real real_;
        ShortOrdinal sords_[sizeof(Ordinal)/sizeof(ShortOrdinal)];
        ShortInteger sints_[sizeof(Integer)/sizeof(ShortInteger)];
        ByteOrdinal bords_[sizeof(Ordinal)/sizeof(ByteOrdinal)];
        ByteInteger bints_[sizeof(Integer)/sizeof(ByteInteger)];
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
        X(ArithmeticStatus, arithmeticStatus, Ordinal);
        X(FloatingPointRoundingControl, floatingPointRoundingControl, Ordinal);
        X(NoImpreciseFaults, noImpreciseFaults, bool);
        X(FloatingPointNormalizingMode, floatingPointNormalizingMode, bool);
#define Y(name, field) \
        X(name ## Flag , field ## Flag, bool); \
        X(name ## Mask , field ## Mask, bool);
        Y(IntegerOverflow, integerOverflow);
        Y(FloatingOverflow, floatingOverflow);
        Y(FloatingUnderflow, floatingUnderflow);
        Y(FloatingInvalidOp, floatingInvalidOp);
        Y(FloatingZeroDivide, floatingZeroDivide);
        Y(FloatingInexact, floatingInexact);
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
            Ordinal arithmeticStatus : 4;
            Ordinal unused0 : 1;
            Ordinal integerOverflowFlag : 1;
            Ordinal unused1 : 3;
            Ordinal integerOverflowMask : 1;
            Ordinal unused2 : 2;
            Ordinal noImpreciseFaults : 1;
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
        constexpr auto getLongReal() const noexcept { return lreal_; }
        constexpr auto getOrdinal(int which = 0) const noexcept { return parts_[which & 0b01]; }
        void setLongOrdinal(LongOrdinal value) noexcept { lord_ = value; }
        void setLongInteger(LongInteger value) noexcept { lint_ = value; }
        void setLongReal(LongReal value) noexcept { lreal_ = value; }
        void setOrdinal(Ordinal value, int which = 0) noexcept { parts_[which & 0b01] = value; }
    private:
        LongOrdinal lord_ = 0;
        LongInteger lint_;
        LongReal lreal_;
        Ordinal parts_[sizeof(LongOrdinal)/ sizeof(Ordinal)];
};

union RegisterFrame {
    RegisterFrame() noexcept : gprs { Register(), Register(), Register(), Register(),
                                         Register(), Register(), Register(), Register(),
                                         Register(), Register(), Register(), Register(),
                                         Register(), Register(), Register(), Register(),
                                       } {

                                       }
    Register gprs[16];
    DoubleRegister dprs[sizeof(gprs)/sizeof(DoubleRegister)];
};


class Core {
public:
    Core() : ip_(0), ac_(0) { };
protected:
    Register ip_; // start at address zero
    ArithmeticControls ac_;
    RegisterFrame locals;
    RegisterFrame globals;
    ExtendedReal fpRegs[4];
};


int main(int argc, char** argv) {
    Core c;
    return 0;
}