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
//
// Created by jwscoggins on 8/18/21.
//

#ifndef SIM3_REGISTER_H
#define SIM3_REGISTER_H
#include "Types.h"
struct TreatAsWordAlignedOrdinal final {
    using UnderlyingType = Ordinal;
};
struct TreatAsQuadAlignedOrdinal final {
    using UnderlyingType = Ordinal;
};
struct TreatAsDoubleAlignedOrdinal final {
    using UnderlyingType = Ordinal;
};
union Register {
public:
    constexpr explicit Register(Ordinal value = 0) noexcept : ord_(value) { }
    [[nodiscard]] constexpr bool getMostSignificantBit() const noexcept { return ord_ & 0x8000'0000; }
    [[nodiscard]] constexpr auto get(TreatAsQuadAlignedOrdinal) const noexcept { return ord_ & 0xFFFF'FFF0; }
    [[nodiscard]] constexpr auto get(TreatAsDoubleAlignedOrdinal) const noexcept { return ord_ & 0xFFFF'FFF8; }
    [[nodiscard]] constexpr auto get(TreatAsWordAlignedOrdinal) const noexcept { return ord_ & 0xFFFF'FFFC; }
    [[nodiscard]] constexpr Integer get(TreatAsInteger) const noexcept { return integer_; }
    [[nodiscard]] constexpr Ordinal get(TreatAsOrdinal) const noexcept { return ord_; }
    [[nodiscard]] constexpr ByteInteger get(TreatAsByteInteger) const noexcept { return static_cast<ByteInteger>(integer_); }
    [[nodiscard]] constexpr ByteInteger get(int which, TreatAsByteInteger) const noexcept { return bints_[which&0b11]; }
    [[nodiscard]] constexpr ByteOrdinal get(TreatAsByteOrdinal) const noexcept { return static_cast<ByteOrdinal>(ord_); }
    [[nodiscard]] constexpr ByteOrdinal get(int which, TreatAsByteOrdinal) const noexcept { return bords_[which&0b11]; }
    [[nodiscard]] constexpr ShortInteger get(TreatAsShortInteger) const noexcept { return static_cast<ShortInteger>(integer_); }
    [[nodiscard]] constexpr auto get(int which, TreatAsShortInteger) const noexcept { return sints_[which&0b01]; }
    [[nodiscard]] constexpr ShortOrdinal get(TreatAsShortOrdinal) const noexcept { return static_cast<ShortOrdinal>(ord_); }
    [[nodiscard]] constexpr auto get(int which, TreatAsShortOrdinal) const noexcept { return sords_[which&0b01]; }
    [[nodiscard]] constexpr Real get(TreatAsReal) const noexcept { return real_; }
    template<typename T>
    [[nodiscard]] constexpr T get() const noexcept {
        return get(TreatAs<T>{});
    }
    void set(Integer value, TreatAsInteger) noexcept { integer_ = value; }
    void set(Ordinal value, TreatAsOrdinal) noexcept { ord_ = value; }
    void set(ByteInteger value, int which, TreatAsByteInteger) noexcept { bints_[which&0b11] = value; }
    void set(ByteOrdinal value, int which, TreatAsByteOrdinal) noexcept { bords_[which&0b11] = value; }
    void set(ShortInteger value, int which, TreatAsShortInteger) noexcept { sints_[which&0b1] = value; }
    void set(ShortOrdinal value, int which, TreatAsShortOrdinal) noexcept { sords_[which&0b1] = value; }
    void set(ByteInteger value, TreatAsByteInteger) noexcept { set(value, TreatAsInteger{}); }
    void set(ByteOrdinal value, TreatAsByteOrdinal) noexcept { set(value, TreatAsOrdinal{}); }
    void set(ShortInteger value, TreatAsShortInteger) noexcept { set(value, TreatAsInteger{}); }
    void set(ShortOrdinal value, TreatAsShortOrdinal) noexcept { set(value, TreatAsOrdinal{}); }
    void set(Real value, TreatAsReal) noexcept { real_ = value; }
    template<typename T>
    void set(T value) noexcept {
        set(value, TreatAs<T>{});
    }
    // not using C++ operators because I need to do tag dispatch to select appropriate type, otherwise I would totally do that.
    template<typename T>
    [[nodiscard]] constexpr T add(const Register& other, TreatAs<T>) const noexcept {
        using K = TreatAs<T>;
        return get(K{}) + other.get(K{});
    }
    template<typename T>
    [[nodiscard]] constexpr T subtract(const Register& other, TreatAs<T>) const noexcept {
        using K = TreatAs<T>;
        return get(K{}) - other.get(K{});
    }
    template<typename T>
    [[nodiscard]] constexpr T multiply(const Register& other, TreatAs<T>) const noexcept {
        using K = TreatAs<T>;
        return get(K{}) * other.get(K{});
    }
    template<typename T>
    [[nodiscard]] constexpr T divide(const Register& other, TreatAs<T>) const noexcept {
        using K = TreatAs<T>;
        return get(K{}) * other.get(K{});
    }
    template<typename T>
    [[nodiscard]] constexpr bool equals(const Register& other, TreatAs<T>) const noexcept {
        using K = TreatAs<T>;
        return get(K{}) == other.get(K{});
    }
    template<typename T>
    [[nodiscard]] constexpr bool notEquals(const Register& other, TreatAs<T>) const noexcept {
        using K = TreatAs<T>;
        return get(K{}) != other.get(K{});
    }

    template<typename T>
    [[nodiscard]] constexpr bool lessThan(const Register& other, TreatAs<T>) const noexcept {
        using K = TreatAs<T>;
        return get(K{}) < other.get(K{});
    }

    template<typename T>
    [[nodiscard]] constexpr bool greaterThan(const Register& other, TreatAs<T>) const noexcept {
        using K = TreatAs<T>;
        return get(K{}) > other.get(K{});
    }

    template<typename T>
    [[nodiscard]] constexpr bool lessThanOrEqual(const Register& other, TreatAs<T>) const noexcept {
        using K = TreatAs<T>;
        return get(K{}) <= other.get(K{});
    }
    template<typename T>
    [[nodiscard]] constexpr bool greaterThanOrEqual(const Register& other, TreatAs<T>) const noexcept {
        using K = TreatAs<T>;
        return get(K{}) >= other.get(K{});
    }
    template<typename T>
    [[nodiscard]] constexpr T remainder(const Register& other, TreatAs<T>) const noexcept {
        using K = TreatAs<T>;
        // taken from the i960Sx manual and adapted for the emulator core
        // auto src2 = get(K{});
        // auto src1 = other.get(K{});
        // return src2 - ((src2 / src1) * src1);
        return get(K{}) % other.get(K{});
    }

    template<typename T>
    [[nodiscard]] constexpr T shiftLeft(const Register& other, TreatAs<T>) const noexcept {
        using K = TreatAs<T>;
        return get(K{}) << other.get(K{});
    }

    template<typename T>
    [[nodiscard]] constexpr T shiftRight(const Register& other, TreatAs<T>) const noexcept {
        using K = TreatAs<T>;
        return get(K{}) >> other.get(K{});
    }

    template<typename T>
    [[nodiscard]] constexpr T rotate(const Register& other, TreatAs<T>) const noexcept {
        using K = TreatAs<T>;
        auto value = get(K{});
        auto len = other.get(K{});
        return ::rotate(value, len);
    }



private:
    Ordinal ord_ = 0;
    Integer integer_;
    ShortOrdinal sords_[sizeof(Ordinal)/sizeof(ShortOrdinal)];
    ShortInteger sints_[sizeof(Integer)/sizeof(ShortInteger)];
    ByteOrdinal bords_[sizeof(Ordinal)/sizeof(ByteOrdinal)];
    ByteInteger bints_[sizeof(Integer)/sizeof(ByteInteger)];
    Real real_;
};

/**
 * @brief Wrapper around a register to make it behave like the frame pointer should
 */
class FramePointer {
public:
    explicit FramePointer(Register& targetRegister, Address alignmentMask) : reg_(targetRegister), alignmentMask_(alignmentMask) {}
    explicit FramePointer(const Register& targetRegister, Address alignmentMask) : reg_(const_cast<Register&>(targetRegister)), alignmentMask_(alignmentMask) {}
    void setAddress(Address address) noexcept {
        reg_.set(address & alignmentMask_, TreatAsOrdinal{});
    }
    [[nodiscard]] Address getAddress() const noexcept {
        return reg_.get(TreatAsOrdinal{}) & alignmentMask_;
    }
private:
    Register& reg_;
    Address alignmentMask_;
};

/**
 * @brief Wrapper around a register to make it behave like the previous frame pointer register
 */
class PreviousFramePointer {
public:
    enum class ReturnType : byte {
        LocalReturn = 0b000,
        FaultReturn = 0b001,
        SupervisorReturnWithTraceFlagSet = 0b010,
        SupervisorReturnWithTraceFlagClear = 0b011,
        StoppedInterruptCall = 0b110, // only found in the i960KB manual..., will not be implemented right now
        InterruptReturn = 0b111,
    };
public:
    explicit PreviousFramePointer(Register& targetRegister) : reg_(targetRegister) {}
    explicit PreviousFramePointer(const Register& targetRegister) : reg_(const_cast<Register&>(targetRegister)) {}
    [[nodiscard]] constexpr bool getPrereturnTraceFlag() const noexcept { return (reg_.get<Ordinal>() & 0b1000); }
    void enablePrereturnTraceFlag() const noexcept { reg_.set<Ordinal>(reg_.get<Ordinal>() | 0b1000); }
    void disablePrereturnTraceFlag() const noexcept {
        reg_.set<Ordinal>(reg_.get<Ordinal>() & ~static_cast<Ordinal>(0b1000));
    }
    void setPrereturnTraceFlag(bool value) const noexcept {
        value ? enablePrereturnTraceFlag() : disablePrereturnTraceFlag();
    }
    [[nodiscard]] constexpr auto getReturnType() const noexcept { return static_cast<ReturnType>(reg_.get<Ordinal>() & 0b111); }
    [[nodiscard]] constexpr Ordinal getAddress() const noexcept {
        // according to the i960Sx manual, the lowest 6 bits are ignored but the lowest 4 bits are always confirmed
        return reg_.get<Ordinal>() & ~0b1'111;
    }
    [[nodiscard]] constexpr auto getWhole() const noexcept { return reg_.get<Ordinal>(); }
    void setWhole(Ordinal value) noexcept { reg_.set<Ordinal>(value); }
    void setReturnType(Ordinal value) noexcept { reg_.set<Ordinal>((reg_.get<Ordinal>() & ~0b111) | (value & 0b111)); }
    void setAddress(Ordinal value) noexcept {
        auto lowerBits = reg_.get<Ordinal>() & 0b1'111;
        reg_.set<Ordinal>((value & ~0b1'111) | lowerBits);
    }
private:
    Register& reg_;
};
union TraceControls {
public:
    constexpr explicit TraceControls(Ordinal value = 0) noexcept : ord_(value) { }
    [[nodiscard]] constexpr auto getValue() const noexcept { return ord_; }
    void setValue(Ordinal value) noexcept { ord_ = value; }
#define X(name, field, type) \
constexpr type get ## name () const noexcept { return field ; } \
void set ## name( type value) noexcept { field  = value ; }
#define Y(particleName, particleField)  \
X(particleName ## TraceMode, particleField ## TraceMode, bool); \
X(particleName ## TraceEvent, particleField ## TraceEvent, bool)
    Y(Instruction, instruction);
    Y(Branch, branch);
    Y(Call, call);
    Y(Return, return);
    Y(Prereturn, prereturn);
    Y(Supervisor, supervisor);
    Y(Breakpoint, breakpoint);
#undef Y
#undef X
    /**
     * Reads and modifies contents of this object
     */
    Ordinal modify(Ordinal mask, Ordinal src) noexcept;
private:
    Ordinal ord_ = 0;
    struct {
        Ordinal unused0 : 1; // 0
        Ordinal instructionTraceMode : 1; // 1
        Ordinal branchTraceMode : 1; // 2
        Ordinal callTraceMode : 1; // 3
        Ordinal returnTraceMode : 1; // 4
        Ordinal prereturnTraceMode : 1; // 5
        Ordinal supervisorTraceMode : 1; // 6
        Ordinal breakpointTraceMode : 1; // 7
        Ordinal unused1 : 9; // 8, 9, 10, 11, 12, 13, 14, 15, 16
        Ordinal instructionTraceEvent : 1; // 17
        Ordinal branchTraceEvent : 1; // 18
        Ordinal callTraceEvent : 1; // 19
        Ordinal returnTraceEvent : 1; // 20
        Ordinal prereturnTraceEvent : 1; // 21
        Ordinal supervisorTraceEvent : 1; // 22
        Ordinal breakpointTraceEvent : 1; // 23
        Ordinal unused2 : 8;
    };
};
static_assert (sizeof(TraceControls) == sizeof(Ordinal), "ArithmeticControls must be the width of a single Ordinal");
union ProcessControls {
public:
    constexpr explicit ProcessControls(Ordinal value = 0) noexcept : ord_(value) { }
    [[nodiscard]] constexpr auto getValue() const noexcept { return ord_; }
    [[nodiscard]] constexpr bool inSupervisorMode() const noexcept { return getExecutionMode(); }
    [[nodiscard]] constexpr bool inUserMode() const noexcept { return !getExecutionMode(); }
    void setValue(Ordinal value) noexcept { ord_ = value; }

#define X(name, field, type) \
constexpr type get ## name () const noexcept { return field ; } \
void set ## name( type value) noexcept { field  = value ; }
    X(TraceEnable, traceEnable, bool);
    X(ExecutionMode, executionMode, bool);
    X(Resume, resume, bool);
    X(TraceFaultPending, traceFaultPending, bool);
    X(State, state, bool);
    X(Priority, priority, Ordinal);
    X(InternalState, internalState, Ordinal);
#undef X
    /**
     * Reads and modifies contents of this object
     */
    Ordinal modify(Ordinal mask, Ordinal src) noexcept;
private:
    Ordinal ord_ = 0;
    struct {
        Ordinal traceEnable : 1; // 0
        Ordinal executionMode : 1; // 1
        Ordinal unused0 : 7;  // 2, 3, 4, 5, 6, 7, 8
        Ordinal resume : 1; // 9
        Ordinal traceFaultPending : 1; // 10
        Ordinal unused1 : 2; // 11, 12
        Ordinal state : 1; // 13
        Ordinal unused2 : 2; // 14, 15
        Ordinal priority : 5; // 16, 17, 18, 19, 20
        Ordinal internalState : 11; // 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
    };
};
static_assert (sizeof(ProcessControls) == sizeof(Ordinal), "ArithmeticControls must be the width of a single Ordinal");


union ArithmeticControls {
public:
    constexpr explicit ArithmeticControls(Ordinal value = 0) noexcept : ord_(value) { }
    [[nodiscard]] constexpr auto getValue() const noexcept { return ord_; }
    [[nodiscard]] constexpr bool getOverflowBit() const noexcept { return getConditionCode() & 0b001; }
    [[nodiscard]] constexpr bool getCarryBit() const noexcept { return getConditionCode() & 0b001; }
    void setCarryBit(bool value) noexcept {
        if (value) {
            setConditionCode(getConditionCode() | 0b010);
        } else {
            setConditionCode(getConditionCode() & 0b101);
        }
    }
    void setOverflowBit(bool value) noexcept {
        if (value) {
            setConditionCode(getConditionCode() | 0b001);
        } else {
            setConditionCode(getConditionCode() & 0b110);
        }
    }
    void setValue(Ordinal value) noexcept { ord_ = value; }
    [[nodiscard]] constexpr byte getConditionCode() const noexcept { return static_cast<byte>(conditionCode); }
    void setConditionCode(byte value) noexcept { conditionCode = value; }
#define X(name, field, type) \
constexpr type get ## name () const noexcept { return field ; } \
void set ## name( type value) noexcept { field  = value ; }
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
    [[nodiscard]] constexpr bool conditionCodeIs(uint8_t value) const noexcept {
        if ((value & 0b111) == 0b000) {
            return (getConditionCode() & (value & 0b111)) == 0;
        } else {
            return (getConditionCode() & (value & 0b111)) != 0;
        }
    }
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




union DoubleRegister {
public:
    constexpr explicit DoubleRegister(LongOrdinal value = 0) noexcept : lord_(value) { }
    constexpr DoubleRegister(Ordinal lower, Ordinal upper) noexcept : parts_{lower, upper} { }
    [[nodiscard]] constexpr auto get(TreatAsLongOrdinal) const noexcept { return lord_; }
    [[nodiscard]] constexpr auto get(TreatAsLongInteger) const noexcept { return lint_; }
    [[nodiscard]] constexpr auto get(int which, TreatAsOrdinal) const noexcept { return parts_[which & 0b01]; }
    [[nodiscard]] constexpr auto get(TreatAsLongReal) const noexcept { return lreal_; }
    void set(LongOrdinal value, TreatAsLongOrdinal) noexcept { lord_ = value; }
    void set(LongInteger value, TreatAsLongInteger) noexcept { lint_ = value; }
    void set(LongReal value, TreatAsLongReal) noexcept { lreal_ = value; }
    void set(Ordinal value, int which, TreatAsOrdinal) noexcept {parts_[which & 0b01] = value; }

    /**
     * @brief Copy the contents of another double register to this one
     * @param other The other double register to get contents from
     */
    void copy(const DoubleRegister& other) noexcept {
        lord_ = other.lord_;
    }
private:
    LongOrdinal lord_ = 0;
    LongInteger lint_;
    Ordinal parts_[sizeof(LongOrdinal)/ sizeof(Ordinal)];
    LongReal lreal_;
};
static_assert(sizeof(Register) * 2 == sizeof(DoubleRegister), "Register is not half the size of DoubleRegister ");


union TripleRegister {
public:
    constexpr explicit TripleRegister(Ordinal a = 0, Ordinal b = 0, Ordinal c = 0) noexcept : parts_{a, b, c, 0}{ }
    constexpr auto getOrdinal(byte which = 0) const noexcept { return parts_[which % 3]; } // very expensive!
    void setOrdinal(Ordinal value, byte which = 0) noexcept { parts_[which % 3] = value; }
    /**
     * @brief Copy the contents of another triple register into this one
     * @param src The source triple register to take from
     */
    void copy(const TripleRegister& src) noexcept  {
        for (byte i = 0;i < 3; ++i) {
            parts_[i] = src.parts_[i];
        }
    }
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
// not a false statement or a copy paste error
static_assert(sizeof(Register) * 4 == sizeof(TripleRegister), "Register is not a fourth the size of TripleRegister");

union QuadRegister {
public:
    constexpr explicit QuadRegister(Ordinal a = 0, Ordinal b = 0, Ordinal c = 0, Ordinal d = 0) noexcept : parts_{a, b, c, d}{ }
    constexpr explicit QuadRegister(LongOrdinal lower, LongOrdinal upper) noexcept : halves_{lower, upper} { }
    constexpr auto getOrdinal(int which = 0) const noexcept { return parts_[which & 0b11]; } // very expensive!
    void setOrdinal(Ordinal value, int which = 0) noexcept { parts_[which & 0b11] = value; }
    constexpr auto getHalf(int which = 0) const noexcept { return halves_[which & 0b01];}
    void setHalf(LongOrdinal value, int which = 0) noexcept { halves_[which & 0b01] = value; }
    [[nodiscard]] constexpr auto getLowerHalf() const noexcept { return halves_[0]; }
    [[nodiscard]] constexpr auto getUpperHalf() const noexcept { return halves_[1]; }
    void setLowerHalf(LongOrdinal value) noexcept {
        halves_[0] = value;
    }
    void setUpperHalf(LongOrdinal value) noexcept {
        halves_[1] = value;
    }
    /**
     * @brief Copy the contents of a provided quad register into this one
     * @param src The source to copy from
     */
    void copy(const QuadRegister& src) noexcept {
        for (byte i = 0;i < 4; ++i) {
            parts_[i] = src.parts_[i];
        }
    }
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

static_assert(sizeof(Register) * 4 == sizeof(QuadRegister), "Register is not a fourth the size of QuadRegister");

union RegisterFrame {
    RegisterFrame() noexcept : gprs { Register(), Register(), Register(), Register(),
                                      Register(), Register(), Register(), Register(),
                                      Register(), Register(), Register(), Register(),
                                      Register(), Register(), Register(), Register(),
    } {

    }
    [[nodiscard]] const Register& getRegister(int index) const noexcept { return gprs[index & 0b1111]; }
    Register& getRegister(int index) noexcept { return gprs[index & 0b1111]; }
    [[nodiscard]] const DoubleRegister& getDoubleRegister(int index) const noexcept { return dprs[(index >> 1) & 0b111]; }
    DoubleRegister& getDoubleRegister(int index) noexcept { return dprs[(index >> 1) & 0b111]; }
    [[nodiscard]] const TripleRegister& getTripleRegister(int index) const noexcept { return tprs[(index >> 2) & 0b11]; }
    TripleRegister& getTripleRegister(int index) noexcept { return tprs[(index >> 2) & 0b11]; }
    [[nodiscard]] const QuadRegister& getQuadRegister(int index) const noexcept { return qprs[(index >> 2) & 0b11]; }
    QuadRegister& getQuadRegister(int index) noexcept { return qprs[(index >> 2) & 0b11]; }

    [[nodiscard]] constexpr auto getNumRegisters() const noexcept { return 16; }
    [[nodiscard]] constexpr auto getNumDoubleRegisters() const noexcept { return 8; }
    [[nodiscard]] constexpr auto getNumTripleRegisters () const noexcept { return 4; }
    [[nodiscard]] constexpr auto getNumQuadRegisters () const noexcept { return 4; }

    Register gprs[16];
    DoubleRegister dprs[sizeof(gprs)/sizeof(DoubleRegister)];
    TripleRegister tprs[sizeof(gprs)/sizeof(TripleRegister)]; // this will have the same alignment as quad registers by ignoring the fourth ordinal
    QuadRegister qprs[sizeof(gprs)/sizeof(QuadRegister)];
    // we put the extended reals in a different location
};
#endif //SIM3_REGISTER_H
