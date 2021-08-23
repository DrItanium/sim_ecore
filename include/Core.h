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

#ifndef SIM3_CORE_H
#define SIM3_CORE_H
#include "Types.h"
#include "Instruction.h"
#include "Register.h"

enum class FaultType : Ordinal {
    Trace = 0x0001'0000,
    Instruction_Trace = Trace | 0x02,
    Branch_Trace = Trace | 0x04,
    Call_Trace = Trace | 0x08,
    Return_Trace = Trace | 0x10,
    Prereturn_Trace = Trace | 0x20,
    Supervisor_Trace = Trace | 0x40,
    Breakpoint_Trace = Trace | 0x80,
    Operation = 0x0002'0000,
    Operation_InvalidOpcode = Operation | 0x01,
    Operation_InvalidOperand = Operation | 0x04,
    Arithmetic = 0x0003'0000,
    Arithmetic_IntegerOverflow = Arithmetic | 0x01,
    Arithmetic_ArithmeticZeroDivide = Arithmetic | 0x02,
#ifdef NUMERICS_ARCHITECTURE
    FloatingPoint = 0x0004'0000,
/// @todo implement floating point faults
#endif
    Constraint = 0x0005'0000,
    Constraint_Range = Constraint | 0x01,
    Constraint_Privileged = Constraint | 0x02,

    Protection = 0x0007'0000,
    Protection_Length = Protection | 0x02,
    Type = 0x000A'0000,
    Type_Mismatch = Type | 0x01,
};
class Core {
public:
    /**
     * @brief The main node in a circular queue used to keep track of the on chip register entries
     */
    class LocalRegisterPack {
    public:
        LocalRegisterPack() = default;
        RegisterFrame& getUnderlyingFrame() noexcept { return underlyingFrame; }
        const RegisterFrame& getUnderlyingFrame() const noexcept { return underlyingFrame; }
        constexpr auto isValid() const noexcept { return valid_; }
        auto* getNext() const noexcept { return next_; }
        auto* getPrevious() const noexcept { return prev_; }
        void setNext(LocalRegisterPack& next) noexcept { next_ = &next; }
        void setPrev(LocalRegisterPack& prev) noexcept { prev_ = &prev; }
    private:
        RegisterFrame underlyingFrame;
        LocalRegisterPack* next_ = nullptr;
        LocalRegisterPack* prev_ = nullptr;
        bool valid_ = false;
    };
public:
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
    explicit Core(Ordinal salign = 4);
    virtual ~Core() = default;
public:
    void run();
protected:
    virtual void boot() = 0;
    virtual Ordinal getSystemAddressTableBase() const noexcept = 0;
    virtual Ordinal getPRCBPtrBase() const noexcept = 0;
    virtual bool continueToExecute() const noexcept = 0;
    virtual Ordinal getSystemProcedureTableBase() ;
    virtual Ordinal getFaultProcedureTableBase() ;
    virtual Ordinal getTraceTablePointer() ;
    virtual Ordinal getInterruptTableBase() ;
    virtual Ordinal getFaultTableBase() ;
    virtual Ordinal getInterruptStackPointer();
    virtual void generateFault(FaultType fault);
    virtual void storeLong(Address destination, LongOrdinal value) {
        DoubleRegister wrapper(value);
        store(destination + 0, wrapper.getOrdinal(0));
        store(destination + 4, wrapper.getOrdinal(1));
    }
    virtual void atomicStore(Address destination, Ordinal value) {
        store(destination, value);
    }
    virtual void store(Address destination, const TripleRegister& reg) {
        store(destination + 0, reg.getOrdinal(0));
        store(destination + 4, reg.getOrdinal(1));
        store(destination + 8, reg.getOrdinal(2));
    }
    virtual void store(Address destination, const QuadRegister& reg) {
        storeLong(destination + 0, reg.getHalf(0));
        storeLong(destination + 8, reg.getHalf(1));
    }
    virtual void storeShortInteger(Address destination, ShortInteger value) {
        union {
            ShortInteger in;
            ShortOrdinal out;
        } thing;
        thing.in = value;
        storeShort(destination, thing.out);
    }
    virtual void storeByteInteger(Address destination, ByteInteger value) {
        union {
            ByteInteger in;
            ByteOrdinal out;
        } thing;
        thing.in = value;
        storeByte(destination, thing.out);
    }
    virtual Ordinal atomicLoad(Address destination) {
        return load(destination);
    }
    virtual LongOrdinal loadLong(Address destination) {
        auto lower = load(destination + 0);
        auto upper = load(destination + 4);
        auto outcome = DoubleRegister(lower, upper).getLongOrdinal();
        return outcome;
    }
    virtual void load(Address destination, TripleRegister& reg) noexcept {
        reg.setOrdinal(load(destination + 0), 0);
        reg.setOrdinal(load(destination + 4), 1);
        reg.setOrdinal(load(destination + 8), 2);
    }
    virtual void load(Address destination, QuadRegister& reg) noexcept {
        reg.setHalf(loadLong(destination + 0), 0);
        reg.setHalf(loadLong(destination + 8), 1);
    }
    QuadRegister loadQuad(Address destination) noexcept {
        QuadRegister tmp;
        load(destination, tmp);
        return tmp;
    }
    virtual void synchronizedStore(Address destination, const DoubleRegister& value) noexcept = 0;
    virtual void synchronizedStore(Address destination, const QuadRegister& value) noexcept = 0;
    virtual void synchronizedStore(Address destination, const Register& value) noexcept = 0;
    virtual ByteOrdinal loadByte(Address destination) = 0;
    virtual void storeByte(Address destination, ByteOrdinal value) = 0;
    virtual ShortOrdinal loadShortAligned(Address destination) = 0;
    virtual void storeShortAligned(Address destination, ShortOrdinal value) = 0;
    virtual Ordinal loadAligned(Address destination) = 0;
    virtual void storeAligned(Address destination, Ordinal value) = 0;
    ShortOrdinal loadShort(Address destination) noexcept;
    virtual void storeShort(Address destination, ShortOrdinal value);
    Ordinal load(Address destination);
    void store(Address destination, Ordinal value);

    Register& getRegister(RegisterIndex targetIndex);
    const Register& getRegister(RegisterIndex targetIndex) const;
    const Register& getSourceRegister(RegisterIndex targetIndex) const { return getRegister(targetIndex); }
    DoubleRegister& getDoubleRegister(RegisterIndex targetIndex);
    const DoubleRegister& getDoubleRegister(RegisterIndex targetIndex) const;
    const DoubleRegister& getSourceDoubleRegister(RegisterIndex targetIndex) const { return getDoubleRegister(targetIndex); }
    TripleRegister& getTripleRegister(RegisterIndex targetIndex);
    const TripleRegister& getTripleRegister(RegisterIndex targetIndex) const;
    QuadRegister& getQuadRegister(RegisterIndex targetIndex);
    const QuadRegister& getQuadRegister(RegisterIndex targetIndex) const;
    Register& getStackPointer() noexcept {
        return getRegister(RegisterIndex::SP960);
    }
    const Register& getStackPointer() const noexcept {
        return getRegister(RegisterIndex::SP960);
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
protected:
    inline Ordinal getSupervisorStackPointer() noexcept { return load((getSystemProcedureTableBase() + 12)); }
    virtual void resetExecutionStatus() noexcept = 0;
private:
    void lda(const Instruction& inst) noexcept;
    void shro(const Instruction& inst) noexcept;
    void shlo(const Instruction& inst) noexcept;
    void ipRelativeBranch(Integer displacement) noexcept {
        advanceIPBy = 0;
        ip_.setInteger(ip_.getInteger() + displacement);
    }
    Instruction loadInstruction(Address baseAddress) noexcept;
    void executeInstruction(const Instruction& instruction) noexcept;
    void cmpi(Integer src1, Integer src2) noexcept;
    void cmpo(Ordinal src1, Ordinal src2) noexcept;
    void syncf() noexcept;
    void cycle() noexcept;
    void setDestination(RegisterIndex index, Ordinal value, TreatAsOrdinal);
    void setDestination(RegisterIndex index, Integer value, TreatAsInteger);
    Integer getSourceRegisterValue(RegisterIndex index, TreatAsInteger) const;
    Ordinal getSourceRegisterValue(RegisterIndex index, TreatAsOrdinal) const;
    void cmpobx(const Instruction& instruction, uint8_t mask) noexcept;
private:
    void saveRegisterFrame(const RegisterFrame& theFrame, Address baseAddress) noexcept;
    void restoreRegisterFrame(RegisterFrame& theFrame, Address baseAddress) noexcept;
    Ordinal computeMemoryAddress(const Instruction& instruction) noexcept;
protected:
    void clearLocalRegisters() noexcept;
private:
protected:
    Register ip_; // start at address zero
    ArithmeticControls ac_;
    ProcessControls pc_;
    TraceControls tc_;
    LocalRegisterPack* currentFrame = nullptr;
    // use a circular queue to store the last n local registers "on-chip"
    RegisterFrame globals;
#ifdef NUMERICS_ARCHITECTURE
    ExtendedReal fpRegs[4] = { 0 };
#endif
    Ordinal advanceIPBy = 0;
    Ordinal salign_;
    Ordinal c_;
    LocalRegisterPack frames[4];
};
#endif //SIM3_CORE_H
