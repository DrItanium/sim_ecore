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

template<typename T>
inline volatile T& memory(const size_t address) noexcept {
    return *reinterpret_cast<T*>(address);
}

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
class IACMessage {
public:
    explicit IACMessage(const QuadRegister& qr) noexcept :
            field0_(qr.getOrdinal(0)),
            field3_(qr.getOrdinal(1)),
            field4_(qr.getOrdinal(2)),
            field5_(qr.getOrdinal(3)) { }
    [[nodiscard]] constexpr uint8_t getMessageType() const noexcept { return messageType_; }
    [[nodiscard]] constexpr uint8_t getField1() const noexcept { return field1_; }
    [[nodiscard]] constexpr uint16_t getField2() const noexcept { return field2_; }
    [[nodiscard]] constexpr uint32_t getField3() const noexcept { return field3_; }
    [[nodiscard]] constexpr uint32_t getField4() const noexcept { return field4_; }
    [[nodiscard]] constexpr uint32_t getField5() const noexcept { return field5_; }
    [[nodiscard]] constexpr uint32_t getField0() const noexcept { return field0_; }
private:
    union {
        /// @todo revert this to do bit manipulation as the bitfield assumes little endian
        uint32_t field0_;
        struct {
            uint16_t field2_;
            uint8_t field1_;
            uint8_t messageType_;
        };
    };
    uint32_t field3_;
    uint32_t field4_;
    uint32_t field5_;
};
class Core {
public:
    static constexpr auto NumRegisterFrames = 4;
    /**
     * @brief The main node in a circular queue used to keep track of the on chip register entries
     */
    class LocalRegisterPack {
    public:
        LocalRegisterPack() = default;
        [[nodiscard]] constexpr auto valid() const noexcept { return valid_; }
        RegisterFrame& getUnderlyingFrame() noexcept { return underlyingFrame; }
        [[nodiscard]] const RegisterFrame& getUnderlyingFrame() const noexcept { return underlyingFrame; }
        /**
         * @brief Relinquish ownership of the current register pack without saving the contents
         */
        void relinquishOwnership() noexcept {
            valid_ = false;
            framePointerAddress_ = 0;
            // the following code does something the original i960 spec does not do, clear registers out
            //for (auto& a : underlyingFrame.gprs) {
            //    a.setOrdinal(0);
            //}
        }
        /**
         * @brief Relinquish ownership of the current register pack after saving its contents to memory if valid
         * @tparam T The function signature used to save registers to the stack
         * @param saveRegisters the function used to save a valid register frame
         */
        template<typename T>
        void relinquishOwnership(T saveRegisters) noexcept {
            if (valid_) {
                saveRegisters(underlyingFrame, framePointerAddress_);
            }
            relinquishOwnership();
        }
        /**
         * @brief Take ownership of this pack and save whatever is currently there (if valid); No analysis of old vs new FP is done;
         * Generally this function is meant for generating new calls into the stack
         * @tparam T
         * @param saveRegisters
         */
        template<typename T>
        void takeOwnership(Address newFP, T saveRegisters) noexcept {
            if (valid_) {
                // we do not analyze to see if we got a match because that should never happen
                saveRegisters(underlyingFrame, framePointerAddress_);
            }
            valid_ = true;
            framePointerAddress_ = newFP;
            // don't clear out the registers
            //for (auto& a : underlyingFrame.dprs) {
            //    // clear out storage two registers at a time
            //    a.setLongOrdinal(0);
            //}
            // the instruction contents will be responsible for populating this back up
        }

        /**
         * @brief Use this pack to try and reclaim ownership to a given register frame, if the pack is currently valid and matches the frame pointer address then we are safe to use it as is
         * @tparam RestoreFunction The function type used for restoring registers from the stack
         * @tparam SaveFunction The function type used for saving registers to the stack
         * @param newFP The new frame pointer address that this pack will reflect
         * @param saveRegisters the function to use when saving registers to the stack
         * @param restoreRegisters the function to use when restoring registers from the stack
         */
        template<typename SaveFunction, typename RestoreFunction>
        void restoreOwnership(Address newFP, SaveFunction saveRegisters, RestoreFunction restoreRegisters) {
            if (valid_) {
                // okay we have something valid in there right now, so we need to determine if
                // it is currently valid or not
                if (newFP == framePointerAddress_) {
                    // okay so we got a match, great!
                    // just leave early since the frame is already setup
                    return;
                }
                // okay we got a mismatch, the goal is to now save the current frame contents to memory
                saveRegisters(underlyingFrame, framePointerAddress_);
                // now we continue on as though this pack was initially invalid
            }
            // either is not valid
            // now do the restore operation since it doesn't matter how we got here
            valid_ = true;
            framePointerAddress_ = newFP;
            restoreRegisters(underlyingFrame, framePointerAddress_);
        }
    private:
        RegisterFrame underlyingFrame;
        Address framePointerAddress_ = 0;
        bool valid_ = false;
    };
public:
    /// @todo figure out a better way to handle literals instead of having very large lookup tables
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
    // Mega2560 pins
public:
    explicit Core(Ordinal salign = 4);
    ~Core() = default;
    void begin() noexcept;
    void boot(Ordinal baseAddress = 0);
    void cycle() noexcept;
private:
    [[nodiscard]] Ordinal getSystemAddressTableBase() const noexcept;
    [[nodiscard]] Ordinal getPRCBPtrBase() const noexcept;
    [[nodiscard]] Ordinal getSystemProcedureTableBase() noexcept;
    [[nodiscard]] Ordinal getFaultProcedureTableBase() noexcept;
    [[nodiscard]] Ordinal getTraceTablePointer() noexcept;
    [[nodiscard]] Ordinal getInterruptTableBase() noexcept;
    [[nodiscard]] Ordinal getFaultTableBase() noexcept;
    [[nodiscard]] Ordinal getInterruptStackPointer() noexcept;
    void generateFault(FaultType fault) noexcept;
    void storeLong(Address destination, LongOrdinal value) noexcept;
    void store(Address destination, const TripleRegister& reg) noexcept;
    void store(Address destination, const QuadRegister& reg) noexcept;
    void storeShortInteger(Address destination, ShortInteger value) noexcept;
    void storeByteInteger(Address destination, ByteInteger value) noexcept;
    [[nodiscard]] LongOrdinal loadLong(Address destination) noexcept;
    void load(Address destination, TripleRegister& reg) noexcept;
    void load(Address destination, QuadRegister& reg) noexcept;
    [[nodiscard]] QuadRegister loadQuad(Address destination) noexcept;
    void synchronizedStore(Address destination, const DoubleRegister& value) noexcept;
    void synchronizedStore(Address destination, const QuadRegister& value) noexcept;
    void synchronizedStore(Address destination, const Register& value) noexcept;
    [[nodiscard]] ByteOrdinal loadByte(Address destination) noexcept;
    void storeByte(Address destination, ByteOrdinal value) noexcept;
    [[nodiscard]] ShortOrdinal loadShort(Address destination) noexcept;
    void storeShort(Address destination, ShortOrdinal value) noexcept;
    [[nodiscard]] Ordinal load(Address destination) noexcept;
    void store(Address destination, Ordinal value) noexcept;
    constexpr bool inInternalSpace(Address destination) const noexcept {
        return static_cast<byte>(destination >> 24) == 0xFF;
    }
    [[nodiscard]] ByteOrdinal readFromInternalSpace(Address destination) noexcept;
    void writeToInternalSpace(Address destination, byte value) noexcept;
    template<typename T>
    void storeToBus(Address destination, T value, TreatAs<T>) noexcept {
        setEBIUpper(destination);
        memory<T>(static_cast<size_t>(destination)) = value;
    }
    template<typename T>
    typename TreatAs<T>::UnderlyingType loadFromBus(Address destination, TreatAs<T>) noexcept {
        setEBIUpper(destination);
        return memory<T>(static_cast<size_t>(destination));
    }

    template<typename T>
    typename TreatAs<T>::UnderlyingType load(Address destination, TreatAs<T>) noexcept {
        using K = TreatAs<T>;
        if (inInternalSpace(destination)) {
            union {
                byte bytes[sizeof(T)] ;
                T value;
            } container;
            // okay we are in internal space. Do byte by byte transfers
            for (size_t i = 0; i < sizeof(T); ++i, ++destination) {
                container.bytes[i] = readFromInternalSpace(destination);
            }
            return container.value;
        } else {
            // we are not in internal space so force the matter
            setEBIUpper(destination);
            return loadFromBus(destination, K{});
        }
    }
    template<typename T>
    void store(Address destination, T value, TreatAs<T>) noexcept {
        using K = TreatAs<T>;
            if (inInternalSpace(destination)) {
                union {
                    byte bytes[sizeof(T)] ;
                    T value;
                } container;
                container.value = value;
                // okay we are in internal space. Do byte by byte transfers
                for (size_t i = 0; i < sizeof(T); ++i, ++destination) {
                    writeToInternalSpace(destination, container.bytes[i]);
                }
            } else {
                // we are not in internal space so force the matter
                setEBIUpper(destination);
                storeToBus(destination, value, K{});
            }
    }

    [[nodiscard]] Register& getRegister(RegisterIndex targetIndex);
    [[nodiscard]] const Register& getRegister(RegisterIndex targetIndex) const;
    [[nodiscard]] const Register& getSourceRegister(RegisterIndex targetIndex) const noexcept;
    [[nodiscard]] DoubleRegister& getDoubleRegister(RegisterIndex targetIndex);
    [[nodiscard]] const DoubleRegister& getDoubleRegister(RegisterIndex targetIndex) const;
    [[nodiscard]] const DoubleRegister& getSourceDoubleRegister(RegisterIndex targetIndex) const { return getDoubleRegister(targetIndex); }
    [[nodiscard]] TripleRegister& getTripleRegister(RegisterIndex targetIndex);
    [[nodiscard]] const TripleRegister& getTripleRegister(RegisterIndex targetIndex) const;
    [[nodiscard]] QuadRegister& getQuadRegister(RegisterIndex targetIndex);
    [[nodiscard]] const QuadRegister& getQuadRegister(RegisterIndex targetIndex) const;
    [[nodiscard]] Register& getStackPointer() noexcept { return getRegister(RegisterIndex::SP960); }
    [[nodiscard]] const Register& getStackPointer() const noexcept { return getRegister(RegisterIndex::SP960); }
    [[nodiscard]] Register& getFramePointer() noexcept { return getRegister(RegisterIndex::FP); }
    [[nodiscard]] const Register& getFramePointer() const noexcept { return getRegister(RegisterIndex::FP); }
    [[nodiscard]] Register& getPFP() noexcept { return getRegister(RegisterIndex::PFP); }
    [[nodiscard]] const Register& getPFP() const noexcept { return getRegister(RegisterIndex::PFP); }
    [[nodiscard]] Register& getRIP() noexcept { return getRegister(RegisterIndex::RIP); }
    [[nodiscard]] const Register& getRIP() const noexcept { return getRegister(RegisterIndex::RIP); }
private:
    /**
     * @brief Compute the next instruction location and store it in RIP
     */
    void setRIP() noexcept;
    inline Ordinal getSupervisorStackPointer() noexcept { return load((getSystemProcedureTableBase() + 12)); }
    LocalRegisterPack& getCurrentPack() noexcept { return frames[currentFrameIndex_]; }
    void setFramePointer(Ordinal value) noexcept;
    [[nodiscard]] Ordinal getFramePointerValue() const noexcept;
    void lda(const Instruction& inst) noexcept;
    void shro(const Instruction& inst) noexcept;
    void shlo(const Instruction& inst) noexcept;
    void flushreg() noexcept;
    void ipRelativeBranch(const Instruction& inst) noexcept;
    [[nodiscard]] Instruction loadInstruction(Address baseAddress) noexcept;
    void executeInstruction(const Instruction& instruction) noexcept;
    void cmpi(const Instruction& instruction) noexcept;
    void cmpo(const Instruction& instruction) noexcept;
    void syncf() noexcept;
    void setDestination(RegisterIndex index, Ordinal value, TreatAsOrdinal);
    void setDestination(RegisterIndex index, Integer value, TreatAsInteger);
    [[nodiscard]] Integer getSourceRegisterValue(RegisterIndex index, TreatAsInteger) const;
    [[nodiscard]] Ordinal getSourceRegisterValue(RegisterIndex index, TreatAsOrdinal) const;
    void cmpobx(const Instruction& instruction, uint8_t mask) noexcept;
    void cmpobx(const Instruction& instruction) noexcept { cmpobx(instruction, instruction.getEmbeddedMask()); }
    void cmpibx(const Instruction& instruction, uint8_t mask) noexcept;
    void cmpibx(const Instruction& instruction) noexcept { cmpibx(instruction, instruction.getEmbeddedMask()); }

    [[nodiscard]] Register& destinationFromSrcDest(const Instruction& instruction) noexcept;
    [[nodiscard]] const Register& sourceFromSrcDest(const Instruction& instruction) const noexcept;
    void setDestinationFromSrcDest(const Instruction& instruction, Ordinal value, TreatAsOrdinal);
    void setDestinationFromSrcDest(const Instruction& instruction, Integer value, TreatAsInteger);
    [[nodiscard]] const Register& sourceFromSrc1(const Instruction& instruction) const noexcept;
    [[nodiscard]] const Register& sourceFromSrc2(const Instruction& instruction) const noexcept;
    [[nodiscard]] Ordinal valueFromSrc1Register(const Instruction& instruction, TreatAsOrdinal) const noexcept;
    [[nodiscard]] Integer valueFromSrc1Register(const Instruction& instruction, TreatAsInteger) const noexcept;
    [[nodiscard]] Ordinal valueFromSrc2Register(const Instruction& instruction, TreatAsOrdinal) const noexcept;
    [[nodiscard]] Integer valueFromSrc2Register(const Instruction& instruction, TreatAsInteger) const noexcept;
private:
    void saveRegisterFrame(const RegisterFrame& theFrame, Address baseAddress) noexcept;
    void restoreRegisterFrame(RegisterFrame& theFrame, Address baseAddress) noexcept;
    Ordinal computeMemoryAddress(const Instruction& instruction) noexcept;
private:
    /**
     * @brief Number of register frames allocated "on-chip", shouldn't be too large as performance will suffer
     */
    [[nodiscard]] RegisterFrame& getLocals() noexcept;
    [[nodiscard]] const RegisterFrame& getLocals() const noexcept;
    [[nodiscard]] LocalRegisterPack& getNextPack() noexcept;
    [[nodiscard]] LocalRegisterPack& getPreviousPack() noexcept;
    /// @todo finish implementing these two functions
    void enterCall(Address newAddress) noexcept;
    void exitCall() noexcept;
    void call(const Instruction& instruction) noexcept;
    void callx(const Instruction& instruction) noexcept;
    void calls(const Instruction& instruction) noexcept;
    void ret() noexcept;
    [[nodiscard]] Ordinal properFramePointerAddress() const noexcept;
private:
    enum class ArithmeticOperation : byte {
        Add,
        Subtract,
        Multiply,
        Divide,
        Remainder,
        Rotate,
        ShiftLeft,
        ShiftRight,
    };
    template<ArithmeticOperation op, typename T>
    void arithmeticGeneric(const Instruction& instruction) noexcept {
        using K = typename T::UnderlyingType;
        K result = static_cast<K>(0);
        auto src2 = valueFromSrc2Register(instruction, T{});
        auto src1 = valueFromSrc1Register(instruction, T{});
        switch (op) {
            case ArithmeticOperation::Add:
                result = src2 + src1;
                break;
            case ArithmeticOperation::Subtract:
                result = src2 - src1;
                break;
            case ArithmeticOperation::Multiply:
                result = src2 * src1;
                break;
            case ArithmeticOperation::Divide:
                /// @todo check denominator and do proper handling
                result = src2 / src1;
                break;
            case ArithmeticOperation::Remainder:
                // taken from the i960Sx manual
                //auto result = src2 - ((src2 / src1) * src1);
                result = src2 % src1;
                break;
            case ArithmeticOperation::Rotate:
                result = rotate(src2, src1);
                break;
            case ArithmeticOperation::ShiftLeft:
                result = src2 << src1;
                break;
            case ArithmeticOperation::ShiftRight:
                /*
                 * // shift right integer implementation from the i960 manual
                 * if (src >= 0) {
                 *  if (len < 32) {
                 *      dest <- src/2^len
                 *  } else {
                 *      dest <- 0
                 *  }
                 * }else {
                 *  if (len < 32) {
                 *      dest <- (src - 2^len + 1)/2^len;
                 *  } else {
                 *      dest <- -1;
                 *   }
                 * }
                 *
                 */
                /// @todo perhaps implement the extra logic if necessary on shri
                result = src2 >> src1;
                break;
        }
        setDestinationFromSrcDest(instruction, result, T{});
    }
    enum class LogicalOp : byte {
        And,
        Or,
        Not,
        Xor,
        Xnor,
        Nor,
        Nand,
        AndNot,
        NotAnd,
        OrNot,
        NotOr,
    };
    template<LogicalOp op>
    void logicalOpGeneric(const Instruction& inst) noexcept {
        Ordinal result = 0;
        auto src2 = valueFromSrc2Register(inst, TreatAsOrdinal{});
        auto src1 = valueFromSrc1Register(inst, TreatAsOrdinal{});
        switch (op) {
            case LogicalOp::And: result = src2 & src1; break;
            case LogicalOp::Or: result = src2 | src1; break;
            case LogicalOp::Not: result = ~src1; break;
            case LogicalOp::Xor: result = src2 ^ src1; break;
            case LogicalOp::Xnor: result = ~(src2 ^ src1); break;
            case LogicalOp::Nor: result = ~(src2 | src1); break;
            case LogicalOp::Nand: result = ~(src2 & src1); break;
            case LogicalOp::AndNot:
                result = src2 & ~src1;
                break;
            case LogicalOp::NotAnd:
                result = ~src2 & src1;
                break;
            case LogicalOp::OrNot:
                result = src2 | ~src1;
                break;
            case LogicalOp::NotOr:
                result = ~src2 | src1;
                break;
        }
        setDestinationFromSrcDest(inst, result, TreatAsOrdinal{});
    }
private:
    void bbc(const Instruction& inst) noexcept;
    void bbs(const Instruction& inst) noexcept;
    [[nodiscard]] const Register& getAbaseRegister(const Instruction& inst) const noexcept;
    [[nodiscard]] const Register& getIndexRegister(const Instruction& inst) const noexcept;
    [[nodiscard]] Ordinal valueFromAbaseRegister(const Instruction& inst, TreatAsOrdinal) const noexcept;
    [[nodiscard]] Integer valueFromAbaseRegister(const Instruction& inst, TreatAsInteger) const noexcept;
    [[nodiscard]] Ordinal valueFromIndexRegister(const Instruction& inst, TreatAsOrdinal) const noexcept;
    [[nodiscard]] Integer valueFromIndexRegister(const Instruction& inst, TreatAsInteger) const noexcept;
    [[nodiscard]] Ordinal scaledValueFromIndexRegister(const Instruction& inst, TreatAsOrdinal) const noexcept;
    [[nodiscard]] Integer scaledValueFromIndexRegister(const Instruction& inst, TreatAsInteger) const noexcept;
    void restoreStandardFrame() noexcept;
    void fmark(const Instruction& inst) noexcept;
    void mark(const Instruction& inst) noexcept;
    enum class ArithmeticWithCarryOperation : byte {
        Add,
        Subtract,
    };
    void withCarryOperationGeneric(const Instruction& inst, ArithmeticWithCarryOperation op) noexcept;
    void addc(const Instruction& inst) noexcept { withCarryOperationGeneric(inst, ArithmeticWithCarryOperation::Add); }
    void subc(const Instruction& inst) noexcept { withCarryOperationGeneric(inst, ArithmeticWithCarryOperation::Subtract); }
    template<typename T>
    void concmpGeneric(const Instruction& instruction) noexcept {
        if ((ac_.getConditionCode() & 0b100) == 0) {
            auto src1 = valueFromSrc1Register(instruction, T{});
            auto src2 = valueFromSrc2Register(instruction, T{});
            ac_.setConditionCode((src1 <= src2) ? 0b010 : 0b001);
        }
    }
    void synld(const Instruction& inst) noexcept;
    void synmov(const Instruction& inst) noexcept;
    void synmovl(const Instruction& inst) noexcept;
    void synmovq(const Instruction& inst) noexcept;
    void lockBus() noexcept;
    void unlockBus() noexcept;
    void synchronizeMemoryRequests() noexcept;
    Ordinal getNextCallFrameStart() noexcept;
    void balx(const Instruction& inst) noexcept;
    void bx(const Instruction& inst) noexcept;
    void notbit(const Instruction& inst) noexcept;
    void condFault(const Instruction& inst) noexcept;
private:
    void condBranch(const Instruction& inst) noexcept;
    void absoluteBranch(Ordinal value) noexcept;
    void processIACMessage(const IACMessage& message) noexcept;
    void storeSystemBase(const IACMessage& message) noexcept;
    void generateSystemInterrupt(const IACMessage& message) noexcept;
    void reinitializeProcessor(const IACMessage& message) noexcept;
    void setBreakpointRegister(const IACMessage& message) noexcept;
    void testPendingInterrupts(const IACMessage& message) noexcept;
    void purgeInstructionCache(const IACMessage& message) noexcept;
    void boot0(Ordinal sat, Ordinal pcb, Ordinal startIP);
    void shrdi(const Instruction& inst) noexcept;
    void setEBIUpper(Address address) noexcept;
    [[noreturn]] void checksumFail() noexcept;
    void setStackPointer(Ordinal value) noexcept;
    [[nodiscard]] Ordinal getStackPointerValue() const noexcept;
public:
    static constexpr size_t NumSRAMBytesMapped = 2048;
    static_assert(NumSRAMBytesMapped < 4096 && NumSRAMBytesMapped >= 1024);
private:
    Register ip_; // start at address zero
    ArithmeticControls ac_;
    ProcessControls pc_;
    TraceControls tc_;
    // use a circular queue to store the last n local registers "on-chip"
    RegisterFrame globals;
#ifdef NUMERICS_ARCHITECTURE
    ExtendedReal fpRegs[4] = { 0 };
#endif
    Ordinal advanceIPBy = 0;
    Ordinal salign_;
    Ordinal c_;
    Ordinal currentFrameIndex_ = 0;
    LocalRegisterPack frames[NumRegisterFrames];
    Ordinal systemAddressTableBase_ = 0;
    Ordinal prcbBase_ = 0;
    byte internalSRAM_[NumSRAMBytesMapped] = { 0 };
    Address ebiUpper_ = 0xFFFF'FFFF;
};
enum class Pinout {
    // expose four controllable interrupts
    Int0_ = 19,
    Int1_ = 18,
    Int2_ = 2,
    Int3_ = 3,
    /**
     * @brief The real EBI_A15 is used to select which 32k window to write to in the EBI space. Thus we take A15 into our own hands when dealing with the external bus
     */
    EBI_A15 = 38,
    EBI_A16 = 49,
    EBI_A17 = 48,
    EBI_A18 = 47,
    EBI_A19 = 46,
    EBI_A20 = 45,
    EBI_A21 = 44,
    EBI_A22 = 43,
    EBI_A23 = 42,
    EBI_A24 = A8,
    EBI_A25 = A9,
    EBI_A26 = A10,
    EBI_A27 = A11,
    EBI_A28 = A12,
    EBI_A29 = A13,
    EBI_A30 = A14,
    EBI_A31 = A15,
};
static constexpr Pinout EBIExtendedPins[] {
        Pinout::EBI_A15 ,
        Pinout::EBI_A16 ,
        Pinout::EBI_A17 ,
        Pinout::EBI_A18 ,
        Pinout::EBI_A19 ,
        Pinout::EBI_A20 ,
        Pinout::EBI_A21 ,
        Pinout::EBI_A22 ,
        Pinout::EBI_A23 ,
        Pinout::EBI_A24 ,
        Pinout::EBI_A25 ,
        Pinout::EBI_A26 ,
        Pinout::EBI_A27 ,
        Pinout::EBI_A28 ,
        Pinout::EBI_A29 ,
        Pinout::EBI_A30 ,
        Pinout::EBI_A31 ,
};
namespace Builtin
{
    enum class Devices
    {
        Query,
        SerialConsole,
        IO,
        SPI,
        I2C,
        AnalogComparator,
        Interrupts,
        Timers,
        AnalogToDigitalConverters,
        JTAG,
        Count,
        Error = Count,
    };
    constexpr Address ConfigurationSpaceBaseAddress = 0xFFFF'F000;
    constexpr Address InternalBaseAddress = 0xFFFF'0000;
    constexpr Address InternalMemorySpaceBase = 0xFF00'0000;
    constexpr Address InternalBootProgramBase = 0xFFFD'0000;
    constexpr Address InternalSRAMBase = 0xFFFE'0000;
    constexpr Address InternalSRAMEnd = InternalSRAMBase + Core::NumSRAMBytesMapped;
    constexpr Address InternalPeripheralBase  = 0xFFFF'0000;
    constexpr Address computeBaseAddress(Devices dev) noexcept {
        return ((static_cast<Address>(dev) << 8) + InternalBaseAddress);
    }
    constexpr bool mapsToTargetPeripheral(Address address, Devices targetDevice) noexcept {
        return (computeBaseAddress(targetDevice) == (address & 0xFFFFFF00));
    }
    constexpr Devices addressToTargetPeripheral(Address address) noexcept {
        return static_cast<Devices>(static_cast<byte>(address >> 8));
    }
}
void pinMode(Pinout p, decltype(OUTPUT) direction) noexcept;
void digitalWrite(Pinout p, decltype(HIGH) value) noexcept;
byte digitalRead(Pinout p) noexcept;
[[noreturn]] void haltExecution(const __FlashStringHelper* message) noexcept;
#endif //SIM3_CORE_H
