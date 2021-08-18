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
    explicit Core(Ordinal salign = 4) : ip_(0), ac_(0), pc_(0), tc_(0), salign_(salign), c_((salign * 16) - 1) { };
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
        store(destination, reg.getOrdinal(0));
        store(destination+4, reg.getOrdinal(1));
        store(destination+8, reg.getOrdinal(2));
    }
    virtual void store(Address destination, const QuadRegister& reg) {
        storeLong(destination, reg.getHalf(0));
        storeLong(destination, reg.getHalf(1));
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
        auto lower = load(destination);
        auto upper = load(destination + 4);
        return DoubleRegister(lower, upper).getLongOrdinal();
    }
    virtual void load(Address destination, TripleRegister& reg) noexcept {
        reg.setOrdinal(load(destination), 0);
        reg.setOrdinal(load(destination + 4), 1);
        reg.setOrdinal(load(destination + 8), 2);
    }
    virtual void load(Address destination, QuadRegister& reg) noexcept {
        reg.setHalf(loadLong(destination), 0);
        reg.setHalf(loadLong(destination + 8), 1);
    }
    QuadRegister loadQuad(Address destination) noexcept {
        QuadRegister tmp;
        load(destination, tmp);
        return tmp;
    }
    virtual ByteOrdinal loadByte(Address destination) {
        // force the unaligned access to be handled in load
        return static_cast<ByteOrdinal>(load(destination));
    }
    virtual ShortOrdinal loadShort(Address destination) {
        // just like with
        return static_cast<ShortOrdinal>(load(destination));
    }
    virtual void storeByte(Address destination, ByteOrdinal value) {
        // terrible implementation but it will work
        // load a full word, modify it and then commit it back
        // This method is virtual so we can easily override this train wreck at any time
        Register memory(load(destination));
        memory.setByteOrdinal(value, 0);
        store(destination, memory.getOrdinal());
    }
    virtual void storeShort(Address destination, ShortOrdinal value) {
        // terrible implementation but it will work
        // load a full word, modify it and then commit it back
        // This method is virtual so we can easily override this train wreck at any time
        Register memory(load(destination));
        memory.setShortOrdinal(value, 0);
        store(destination, memory.getOrdinal());
    }
    virtual Ordinal load(Address destination) = 0;
    virtual void store(Address destination, Ordinal value) = 0;
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
protected:
    inline Ordinal getSupervisorStackPointer() noexcept { return load((getSystemProcedureTableBase() + 12)); }
private:
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
private:
    void saveRegisterFrame(const RegisterFrame& theFrame, Address baseAddress) noexcept;
    void restoreRegisterFrame(RegisterFrame& theFrame, Address baseAddress) noexcept;
    Ordinal computeMemoryAddress(const Instruction& instruction) noexcept;
protected:
    Register ip_; // start at address zero
    ArithmeticControls ac_;
    ProcessControls pc_;
    TraceControls tc_;
    RegisterFrame locals;
    RegisterFrame globals;
#ifdef NUMERICS_ARCHITECTURE
    ExtendedReal fpRegs[4] = { 0 };
#endif
    Ordinal advanceIPBy = 0;
    Ordinal salign_;
    Ordinal c_;
};
#endif //SIM3_CORE_H
