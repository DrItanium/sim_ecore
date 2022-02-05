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
#include "Core.h"
#include <Arduino.h>
namespace
{
    constexpr bool EnableEmulatorTrace = false;
    Register BadRegister(-1);
    DoubleRegister BadRegisterDouble(-1);
    TripleRegister BadRegisterTriple(-1);
    QuadRegister BadRegisterQuad(-1);
    constexpr Ordinal bitPositions[32] {
#define Z(base, offset) static_cast<Ordinal>(1) << static_cast<Ordinal>(base + offset)
#define X(base) Z(base, 0), Z(base, 1), Z(base, 2), Z(base, 3)
            X(0), X(4), X(8), X(12),
            X(16), X(20), X(24), X(28)
#undef X
#undef Z
    };
    constexpr Ordinal reverseBitPositions[32] {
#define Z(base, offset) static_cast<Ordinal>(1) << static_cast<Ordinal>(base + offset)
#define X(base) Z(base, 3), Z(base, 2), Z(base, 1), Z(base, 0)
            X(28), X(24), X(20), X(16),
            X(12), X(8), X(4), X(0),
#undef X
#undef Z
    };
    constexpr Ordinal getBitPosition(Ordinal value) noexcept {
        return bitPositions[value & 0b11111];
    }
}
Register&
Core::destinationFromSrcDest(const Instruction& instruction) noexcept {
    return getRegister(instruction.getSrcDest(false));
}
void
Core::setDestinationFromSrcDest(const Instruction& instruction, Ordinal value, TreatAsOrdinal) {
    destinationFromSrcDest(instruction).set<Ordinal>(value);
}
void
Core::setDestinationFromSrcDest(const Instruction& instruction, Integer value, TreatAsInteger) {
    destinationFromSrcDest(instruction).set<Integer>(value);
}
void
Core::setDestinationFromSrcDest(const Instruction& instruction, LongOrdinal value, TreatAsLongOrdinal) {
    getDoubleRegister(instruction.getSrcDest(false)).set(value, TreatAsLongOrdinal{});
}
DoubleRegister&
Core::longDestinationFromSrcDest(const Instruction& instruction) noexcept {
    return getDoubleRegister(instruction.getSrcDest(false));
}
void
Core::syncf() noexcept {
    if (ac_.getNoImpreciseFaults()) {

    } else {
        /// @todo wait until no imprecise faults can occur associated with any uncompleted instructions
    }
}

void
Core::cycle() noexcept {
    if constexpr (EnableEmulatorTrace) {
        Serial.print(F("\trip(before): 0x"));
        Serial.println(getRIP().get<Ordinal>(), HEX);
    }
    advanceIPBy = 4;
    auto instruction = loadInstruction(ip_.get<Ordinal>());
    executeInstruction(instruction);
    //executeInstruction(loadInstruction(ip_.get<Ordinal>()));
    if (advanceIPBy > 0)  {
        ip_.set<Ordinal>(ip_.get<Ordinal>() + advanceIPBy);
    }
    if constexpr (EnableEmulatorTrace) {
        Serial.print(F("\trip(after): 0x"));
        Serial.println(getRIP().get<Ordinal>(), HEX);
    }
}

Register&
Core::getRegister(RegisterIndex targetIndex) {
    if (isLocalRegister(targetIndex)) {
        return getLocals().getRegister(static_cast<uint8_t>(targetIndex));
    } else if (isGlobalRegister(targetIndex)) {
        return globals.getRegister(static_cast<uint8_t>(targetIndex));
    } else {
        /// @todo figure out what to return on a fault failure?
        generateFault(FaultType::Operation_InvalidOperand);
        return BadRegister;
    }
}

DoubleRegister&
Core::getDoubleRegister(RegisterIndex targetIndex) {
    if (isLocalRegister(targetIndex)) {
        return getLocals().getDoubleRegister(static_cast<int>(targetIndex));
    } else if (isGlobalRegister(targetIndex)) {
        return globals.getDoubleRegister(static_cast<int>(targetIndex));
    } else {
        /// @todo figure out what to return on a fault failure?
        generateFault(FaultType::Operation_InvalidOperand);
        return BadRegisterDouble;
    }
}


TripleRegister&
Core::getTripleRegister(RegisterIndex targetIndex) {
    if (isLocalRegister(targetIndex)) {
        return getLocals().getTripleRegister(static_cast<int>(targetIndex));
    } else if (isGlobalRegister(targetIndex)) {
        return globals.getTripleRegister(static_cast<int>(targetIndex));
    } else {
        /// @todo figure out what to return on a fault failure?
        generateFault(FaultType::Operation_InvalidOperand);
        return BadRegisterTriple;
    }
}

QuadRegister&
Core::getQuadRegister(RegisterIndex targetIndex) {
    if (isLocalRegister(targetIndex)) {
        return getLocals().getQuadRegister(static_cast<int>(targetIndex));
    } else if (isGlobalRegister(targetIndex)) {
        return globals.getQuadRegister(static_cast<int>(targetIndex));
    } else {
        generateFault(FaultType::Operation_InvalidOperand);
        return BadRegisterQuad;
    }
}

Instruction
Core::loadInstruction(Address baseAddress) noexcept {
    // load words 64-bits at a time for simplicity, we increment by eight on double wide instructions and four on single wide
    auto targetAddress = baseAddress & ~(static_cast<Address>(0b11));
    auto theLong = loadLong(targetAddress);
    return Instruction(theLong);
}

void
Core::saveRegisterFrame(const RegisterFrame &theFrame, Address baseAddress) noexcept {
    for (byte i = 0; i < 16; ++i, baseAddress += 4) {
        store(baseAddress, theFrame.getRegister(i).get<Ordinal>());
    }
}

void
Core::restoreRegisterFrame(RegisterFrame &theFrame, Address baseAddress) noexcept {
    for (auto& reg : theFrame.gprs) {
        reg.set<Ordinal>(load(baseAddress));
        baseAddress += 4;
    }
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
            return instruction.getOffset() + valueFromAbaseRegister<Ordinal>(instruction);
        case MEMFormatMode::MEMB_RegisterIndirect:
            return valueFromAbaseRegister<Ordinal>(instruction);
        case MEMFormatMode::MEMB_RegisterIndirectWithIndex:
            return valueFromAbaseRegister<Ordinal>(instruction) + scaledValueFromIndexRegister<Ordinal>(instruction);
        case MEMFormatMode::MEMB_IPWithDisplacement:
            return static_cast<Ordinal>(ip_.get<Integer>() + instruction.getDisplacement() + 8);
        case MEMFormatMode::MEMB_AbsoluteDisplacement:
            return instruction.getDisplacement(); // this will return the optional displacement
        case MEMFormatMode::MEMB_RegisterIndirectWithDisplacement:
            return static_cast<Ordinal>(valueFromAbaseRegister<Integer>(instruction) + instruction.getDisplacement());
        case MEMFormatMode::MEMB_IndexWithDisplacement:
            return static_cast<Ordinal>(scaledValueFromIndexRegister<Integer>(instruction) + instruction.getDisplacement());
        case MEMFormatMode::MEMB_RegisterIndirectWithIndexAndDisplacement:
            return static_cast<Ordinal>(
                    valueFromAbaseRegister<Integer>(instruction) +
                    scaledValueFromIndexRegister<Integer>(instruction) +
                    instruction.getDisplacement());
        default:
            return -1;
    }
}
void
Core::lda(const Instruction &inst) noexcept {
    // compute the effective address (memory address) and store it in destination
    setDestinationFromSrcDest(inst, computeMemoryAddress(inst), TreatAsOrdinal{});
}
void
Core::cmpobx(const Instruction &instruction, uint8_t mask) noexcept {
    cmpo(instruction);
    if ((mask & ac_.getConditionCode()) != 0) {
        // while the docs show (displacement * 4), I am currently including the bottom two bits being forced to zero in displacement
        // in the future (the HX uses those two bits as "S2" so that will be a fun future change...).
        // I do not know why the Sx manual shows adding four while the hx manual does not
        // because of this, I'm going to drop the +4  from both paths and only disable automatic incrementation if we are successful
        ipRelativeBranch(instruction);
    }
};
void
Core::bbc(const Instruction& instruction) noexcept {
    auto src = valueFromSrc2Register<Ordinal>(instruction);
    auto bitpos = getBitPosition(valueFromSrc1Register<Ordinal>(instruction));
    if ((bitpos & src) == 0) {
        // another lie in the i960Sx manual, when this bit is clear we assign 0b000 otherwise it is 0b010
        ac_.setConditionCode(0b000);
        // while the docs show (displacement * 4), I am currently including the bottom two bits being forced to zero in displacement
        // in the future (the HX uses those two bits as "S2" so that will be a fun future change...)
        ipRelativeBranch(instruction);
    } else {
        ac_.setConditionCode(0b010);
    }
}
void
Core::bbs(const Instruction& instruction) noexcept {
    auto src = valueFromSrc2Register<Ordinal>(instruction);
    auto bitpos = getBitPosition(valueFromSrc1Register<Ordinal>(instruction));
    if ((bitpos & src) != 0) {
        ac_.setConditionCode(0b010);
        // while the docs show (displacement * 4), I am currently including the bottom two bits being forced to zero in displacement
        // in the future (the HX uses those two bits as "S2" so that will be a fun future change...)
        ipRelativeBranch(instruction);
    } else {
        ac_.setConditionCode(0b000);
    }

}
void
Core::condBranch(const Instruction& inst) noexcept {
    if (auto mask = inst.getEmbeddedMask(); mask != 0b000) {
        if ((ac_.getConditionCode() & mask) != 0) {
            ipRelativeBranch(inst);
        }
    } else {
        if (ac_.getConditionCode() == 0) {
            ipRelativeBranch(inst);
        }
    }
}
void
Core::condFault(const Instruction &inst) noexcept {
    if (auto mask = inst.getEmbeddedMask(); mask != 0b000) {
       if ((ac_.getConditionCode() & mask) != 0) {
           generateFault(FaultType::Constraint_Range);
       }
    } else {
        if (ac_.getConditionCode() == 0) {
            generateFault(FaultType::Constraint_Range);
        }
    }
}
void
Core::testOp(const Instruction &inst) noexcept {
    getRegister(inst.getSrc1(true)).set<Ordinal>(ac_.conditionCodeIs(inst.getEmbeddedMask()) ? 1 : 0);
}

Ordinal
Core::getSystemProcedureTableBase() noexcept {
    return load(getSystemAddressTableBase() + 120);
}
Ordinal
Core::getFaultProcedureTableBase() noexcept {
    return load(getSystemAddressTableBase() + 152);

}
Ordinal
Core::getTraceTablePointer() noexcept {
    return load(getSystemAddressTableBase() + 168);
}
Ordinal
Core::getInterruptTableBase() noexcept {
    return load(getPRCBPtrBase() + 20);
}
Ordinal
Core::getFaultTableBase() noexcept {
    return load(getPRCBPtrBase() + 40);
}

Ordinal
Core::getInterruptStackPointer() noexcept {
    return load(getPRCBPtrBase() + 24);
}

Core::Core(Ordinal salign) : ip_(0), ac_(0), pc_(0), tc_(0), salign_(salign), c_((salign * 16) - 1), stackAlignMask_(c_ - 1), frameAlignmentMask_(~stackAlignMask_) {
}

void
Core::flushreg(const Instruction&) noexcept {
    // clear all registers except the current one
    for (Ordinal curr = currentFrameIndex_ + 1; curr != currentFrameIndex_; curr = ((curr + 1) % NumRegisterFrames)) {
        frames[curr].relinquishOwnership([this](const RegisterFrame& frame, Address dest) noexcept {
            saveRegisterFrame(frame, dest);
        });
    }
}
void
Core::cmpibx(const Instruction &instruction, uint8_t mask) noexcept {
    cmpi(instruction);
    if ((mask & ac_.getConditionCode()) != 0) {
        // while the docs show (displacement * 4), I am currently including the bottom two bits being forced to zero in displacement
        // in the future (the HX uses those two bits as "S2" so that will be a fun future change...)

        // I do not know why the Sx manual shows adding four while the hx manual does not
        // because of this, I'm going to drop the +4  from both paths and only disable automatic incrementation if we are successful
        // this will fix an off by four problem I'm currently encountering
        ipRelativeBranch(instruction);
    }
}

RegisterFrame&
Core::getLocals() noexcept {
    return frames[currentFrameIndex_].getUnderlyingFrame();
}
const RegisterFrame&
Core::getLocals() const noexcept {
    return frames[currentFrameIndex_].getUnderlyingFrame();
}
void
Core::setFramePointer(Ordinal value) noexcept {
    getFramePointer().setAddress(value);
}

Ordinal
Core::getNextCallFrameStart() noexcept {
    return (getStackPointerValue() + c_) & ~c_; // round to next boundary
}
void
Core::setRIP() noexcept {
    getRIP().set<Ordinal>(ip_.get<Ordinal>() + advanceIPBy);
}
void
Core::setStackPointer(Ordinal value) noexcept {
    getStackPointer().set<Ordinal>(value);
}
Ordinal
Core::getStackPointerValue() const noexcept {
    return getOperand<Ordinal>(RegisterIndex::SP960).getValue();
}
void
Core::call(const Instruction& instruction) noexcept {
    if constexpr (EnableEmulatorTrace) {
        Serial.println(F("CALL!"));
    }
    // wait for any uncompleted instructions to finish
    auto temp = getNextCallFrameStart();
    auto fp = getFramePointerValue();
    setRIP();
    enterCall(temp);
    ip_.set<Integer>(ip_.get<Integer>() + instruction.getDisplacement());
    /// @todo expand pfp and fp to accurately model how this works
    getPFP().setAddress(fp);
    setFramePointer(temp);
    setStackPointer(temp + 64);
    advanceIPBy = 0; // we already know where we are going so do not jump ahead
}
void
Core::callx(const Instruction& instruction) noexcept {
    if constexpr (EnableEmulatorTrace) {
        Serial.println(F("CALLX!"));
    }
// wait for any uncompleted instructions to finish
    auto temp = getNextCallFrameStart();
    auto fp = getFramePointerValue();
    auto memAddr = computeMemoryAddress(instruction);
    if constexpr (EnableEmulatorTrace) {
        Serial.print(F("\tMEM ADDR: 0x"));
        Serial.println(memAddr, HEX);
    }
    setRIP();
/// @todo implement support for caching register frames
    enterCall(temp);
    absoluteBranch(memAddr);
    getPFP().setAddress(fp);
    setFramePointer(temp);
    setStackPointer(temp + 64);
}

void
Core::calls(const Instruction& instruction) noexcept {
    if constexpr (EnableEmulatorTrace) {
        Serial.println(F("CALLS!"));
    }
    if (auto targ = valueFromSrc1Register<Ordinal>(instruction); targ > 259) {
        generateFault(FaultType::Protection_Length);
    } else {
        syncf();
        auto tempPE = load(getSystemProcedureTableBase() + 48 + (4 * targ));
        auto type = tempPE & 0b11;
        auto procedureAddress = tempPE & ~0b11;
        // read entry from system-procedure table, where sptbase is address of system-procedure table from IMI
        setRIP();
        absoluteBranch(procedureAddress);
        Ordinal temp = 0;
        Ordinal tempRRR = 0;
        if ((type == 0b00) || pc_.inSupervisorMode()) {
            temp = getNextCallFrameStart();
            tempRRR = 0;
        } else {
            temp = getSupervisorStackPointer();
            tempRRR = pc_.getTraceEnable() ? 0b011 : 0b010;
            pc_.setExecutionMode(true);
            pc_.setTraceEnable(temp & 0b1);
        }
        enterCall(temp);
        /// @todo expand pfp and fp to accurately model how this works
        auto pfp = getPFP();
        pfp.setAddress(getFramePointerValue());
        pfp.setReturnType(tempRRR);
        setFramePointer(temp);
        setStackPointer(temp + 64);
        // we do not want to jump ahead on calls
    }
}
void
Core::restoreStandardFrame() noexcept {
    // load the previous frame and get RIP out of that
    exitCall();
    absoluteBranch(getRIP().get<Ordinal>());
}
void
Core::handleFaultReturn() noexcept {
    if constexpr (EnableEmulatorTrace) {
        Serial.println(F("FAULT RETURN!"));
    }
    auto fpOrd = getFramePointerValue();
    if constexpr (EnableEmulatorTrace) {
        Serial.print(F("0b001: fpOrd: 0x"));
        Serial.println(fpOrd, HEX);
    }
    auto x = load(fpOrd - 16);
    auto y = load(fpOrd - 12);
    restoreStandardFrame();
    ac_.setValue(y);
    if (pc_.inSupervisorMode()) {
        pc_.setValue(x);
    }
}
void
Core::handleSupervisorReturnWithTraceSet() noexcept {
    if constexpr (EnableEmulatorTrace) {
        Serial.println(F("Supervisor return, with the trace enable flag in the process controls set to 1 and exec mode set to 0"));
    }
    if (pc_.inSupervisorMode()) {
        pc_.setTraceEnable(false);
        pc_.setExecutionMode(false);
    }
    restoreStandardFrame();
}
void
Core::handleSupervisorReturnWithTraceClear() noexcept {
    if constexpr (EnableEmulatorTrace) {
        Serial.println(F("Supervisor return, with the trace enable flag in the process controls set to 0 and exec mode set to 0"));
    }
    if (pc_.inSupervisorMode())  {
        pc_.setTraceEnable(true);
        pc_.setExecutionMode(false);
    }
    restoreStandardFrame();
}
void
Core::handleInterruptReturn() noexcept {
    if constexpr (EnableEmulatorTrace) {
        Serial.println(F("INTERRUPT RETURN!"));
    }

    auto fpOrd = getFramePointerValue();
    if constexpr (EnableEmulatorTrace) {
        Serial.print(F("0b111: fpOrd: 0x"));
        Serial.println(fpOrd, HEX);
    }
    auto x = load(fpOrd - 16);
    auto y = load(fpOrd - 12);
    restoreStandardFrame();
    ac_.setValue(y);
    if (pc_.inSupervisorMode()) {
        pc_.setValue(x);
        checkPendingInterrupts();
    }
}
void
Core::handleLocalReturn() noexcept {
    if constexpr (EnableEmulatorTrace) {
        Serial.println(F("LOCAL RETURN"));
    }
    restoreStandardFrame();
}
void
Core::ret(const Instruction&) noexcept {
    if constexpr (EnableEmulatorTrace) {
        Serial.println(F("RET!"));
    }
    syncf();
    auto pfp = getPFP();
    switch (pfp.getReturnType()) {
        case PreviousFramePointer::ReturnType::LocalReturn:
            handleLocalReturn();
            break;
        case PreviousFramePointer::ReturnType::FaultReturn:
            handleFaultReturn();
            break;
        case PreviousFramePointer::ReturnType::SupervisorReturnWithTraceFlagSet:
            handleSupervisorReturnWithTraceSet();
            break;
        case PreviousFramePointer::ReturnType::SupervisorReturnWithTraceFlagClear:
            handleSupervisorReturnWithTraceClear();
            break;
        case PreviousFramePointer::ReturnType::InterruptReturn:
            handleInterruptReturn();
            break;
        default: // reserved entries
            break;
    }
}
Ordinal
Core::getFramePointerValue() const noexcept {
    return getOperand<Ordinal>(RegisterIndex::FP).getValue() & frameAlignmentMask_;
}
Ordinal
Core::properFramePointerAddress() const noexcept {
    // we have to remember that a given number of bits needs to be ignored when dealing with the frame pointer
    // we have to use the "c_" parameter for this
    return getFramePointerValue() & (~c_);
}
Core::LocalRegisterPack&
Core::getNextPack() noexcept {
    return frames[(currentFrameIndex_ + 1) % NumRegisterFrames];
}
Core::LocalRegisterPack&
Core::getPreviousPack() noexcept {
    return frames[(currentFrameIndex_ - 1) % NumRegisterFrames];
}
void
Core::exitCall() noexcept {
    if constexpr (EnableEmulatorTrace) {
        Serial.print("ENTERING ");
        Serial.println(__PRETTY_FUNCTION__);
        Serial.print("OLD FP: 0x");
        Serial.println(properFramePointerAddress(), HEX);
    }
    setFramePointer(getPFP().getAddress());
    if constexpr (EnableEmulatorTrace) {
        Serial.print("NEW FP: 0x");
        Serial.println(properFramePointerAddress(), HEX);
        Serial.print(F("Old Frame Index: 0x"));
        Serial.println(currentFrameIndex_, HEX);
        if (getCurrentPack().valid()) {
            Serial.println(F("CURRENT PACK IS VALID"));
            Serial.print(F("RIP: 0x"));
            Serial.println(getRIP().get<Ordinal>(), HEX);
        } else {
            Serial.println(F("CURRENT PACK IS INVALID"));
        }
    }
    // compute the new frame pointer address
    auto targetAddress = properFramePointerAddress();
    if constexpr (EnableEmulatorTrace) {
        Serial.print(F("targetAddress: 0x"));
        Serial.println(targetAddress, HEX);
    }
    // okay we are done with the current frame so relinquish ownership
    frames[currentFrameIndex_].relinquishOwnership();
    getPreviousPack().restoreOwnership(targetAddress,
                                       [this](const RegisterFrame& frame, Address targetAddress) noexcept { saveRegisterFrame(frame, targetAddress); },
                                       [this](RegisterFrame& frame, Address targetAddress) noexcept { restoreRegisterFrame(frame, targetAddress); });
    // okay the restoration is complete so just decrement the address
    --currentFrameIndex_;
    currentFrameIndex_ %= NumRegisterFrames;
    if constexpr (EnableEmulatorTrace) {
        Serial.print(F("New Frame Index: 0x"));
        Serial.println(currentFrameIndex_, HEX);
        Serial.print("EXITING ");
        Serial.println(__PRETTY_FUNCTION__);
    }
}
void
Core::enterCall(Address newFP) noexcept {
    if constexpr (EnableEmulatorTrace) {
        Serial.print("ENTERING ");
        Serial.println(__PRETTY_FUNCTION__);
        Serial.print("OLD FP: 0x");
        Serial.println(properFramePointerAddress(), HEX);
        Serial.print("NEW FP: 0x");
        Serial.println(newFP, HEX);
        Serial.print(F("Old Frame Index: 0x"));
        Serial.println(currentFrameIndex_, HEX);
        if (getCurrentPack().valid()) {
            Serial.println(F("CURRENT PACK IS VALID"));
            Serial.print(F("RIP: 0x"));
            Serial.println(getRIP().get<Ordinal>(), HEX);
        } else {
            Serial.println(F("CURRENT PACK IS INVALID"));
        }
        Serial.print(F("newFP: 0x"));
        Serial.println(newFP, HEX);
    }
    // this is much simpler than exiting, we just need to take control of the next register frame in the set
    getNextPack().takeOwnership(newFP, [this](const RegisterFrame& frame, Address address) noexcept { saveRegisterFrame(frame, address); });
    // then increment the frame index
    ++currentFrameIndex_;
    currentFrameIndex_ %= NumRegisterFrames;
    if constexpr (EnableEmulatorTrace) {
        Serial.print(F("New Frame Index: 0x"));
        Serial.println(currentFrameIndex_, HEX);
        Serial.print("EXITING ");
        Serial.println(__PRETTY_FUNCTION__);
    }
}

void
Core::mark(const Instruction& inst) noexcept {
// Generates a breakpoint trace-event if the breakpoint trace mode has been enabled.
// The breakpoint trace mode is enabled if the trace-enable bit (bit 0) of the process
// controls and the breakpoint-trace mode bit (bit 7) of the trace controls have been zet
    if (pc_.getTraceEnable() && tc_.getBreakpointTraceMode()) {
        generateFault(FaultType::Breakpoint_Trace); /// @todo raise trace breakpoint fault
    }
// if pc.te == 1 && breakpoint_trace_flag then raise trace breakpoint fault
/// @todo implement
}
void
Core::fmark(const Instruction &) noexcept {

// Generates a breakpoint trace-event. This instruction causes a breakpoint trace-event to be generated, regardless of the
// setting of the breakpoint trace mode flag (to be implemented), providing the trace-enable bit (bit 0) of the process
// controls is set.

// if pc.te == 1 then raiseFault(BreakpointTraceFault)
/// @todo implement
    if (pc_.getTraceEnable()) {
        generateFault(FaultType::Breakpoint_Trace); /// @todo raise trace breakpoint fault
    }
}
namespace {
    constexpr DoubleRegister doAddc(const Operand<Ordinal>& src2, const Operand<Ordinal>& src1, byte carry) noexcept {
        return DoubleRegister{
            static_cast<LongOrdinal>(src2.getValue()) +
            static_cast<LongOrdinal>(src1.getValue()) +
            carry
        };
    }
    constexpr DoubleRegister doSubc(const Operand<Ordinal>& src2, const Operand<Ordinal>& src1, byte carry) noexcept {
        return DoubleRegister{
                static_cast<LongOrdinal>(src2.getValue()) - (static_cast<LongOrdinal>(src1.getValue()) - 1) + carry
        };
    }
    constexpr Ordinal getMostSignificantBit(const Operand<Ordinal>& value) noexcept { return value.getValue() & 0x8000'0000; }

    bool overflowDetected(const Operand<Ordinal>& src2, const Operand<Ordinal>& src1, const Register& dest) noexcept {
        return (getMostSignificantBit(src2) == getMostSignificantBit(src1)) && (getMostSignificantBit(src2) != dest.getMostSignificantBit());
    }
}
void
Core::withCarryOperationGeneric(const Instruction &instruction, ArithmeticWithCarryOperation op) noexcept {
    auto src1 = sourceFromSrc1<Ordinal>(instruction);
    auto src2 = sourceFromSrc2<Ordinal>(instruction);
    auto& dest = destinationFromSrcDest(instruction);
    byte carry = ac_.getCarryBit() ? 1 : 0;
    DoubleRegister result(op == ArithmeticWithCarryOperation::Add ? doAddc(src2, src1, carry) : doSubc(src2, src1, carry));
    dest.set<Ordinal>(result.get(0, TreatAsOrdinal{}));
    ac_.clearConditionCode();
    // look for overflow
    if (overflowDetected(src2, src1, dest)) {
        ac_.setOverflowBit(true);
    }
    // There are other ways to do this, see https://devblogs.microsoft.com/oldnewthing/20170822-00/?p=96865 for more information
    ac_.setCarryBit(result.get(1, TreatAsOrdinal{}) != 0);
}
namespace {
    [[nodiscard]] constexpr Ordinal wordAlign(Ordinal value) noexcept { return value & 0xFFFF'FFF0; }
    [[nodiscard]] constexpr Ordinal doubleWordAlign(Ordinal value) noexcept { return value & 0xFFFF'FFF8; }
    [[nodiscard]] constexpr Ordinal quadWordAlign(Ordinal value) noexcept { return value & 0xFFFF'FFFC; }
}
void Core::synld(const Instruction& instruction) noexcept {
    // wait until another execution unit sets the condition codes to continue after requesting a load.
    // In the case of this emulator, it really doesn't mean anything but I can see this being a synld followed by a wait
    // for synchronization. It also allows access to internal memory mapped items.
    // So I'm not sure how to implement this yet, however I think at this point I'm just going to treat is as a special kind of load
    // with condition code assignments and forced alignments
    auto address = wordAlign(valueFromSrc1Register<Ordinal>(instruction));
    // load basically takes care of accessing different registers and such even memory mapped ones
    setDestinationFromSrcDest(instruction, load(address), TreatAsOrdinal{});
    // there is a _fail_ condition where a bad access condition will result in 0b000
    /// @todo implement support for bad access conditions
    ac_.setConditionCode(0b010);

}

void
Core::synmov(const Instruction &instruction) noexcept {
    // load from memory and then store to another address in a synchronous fashion
    auto src = valueFromSrc2Register<Ordinal>(instruction);
    auto addr = wordAlign(valueFromSrc1Register<Ordinal>(instruction));
#if 0
    Serial.print(F("synmov(0x"));
    Serial.print(addr, HEX);
    Serial.print(F(", 0x"));
    Serial.print(src, HEX);
    Serial.println(F(")"));
#endif
    auto result = load(src);
    Register temp(result);
#if 0
    Serial.print(F("result: 0x"));
    Serial.println(result, HEX);
#endif
    synchronizedStore(addr, temp);
    /// @todo figure out how to support bad access conditions
    ac_.setConditionCode(0b010);
}
void
Core::synmovl(const Instruction &instruction) noexcept {
    auto src = valueFromSrc2Register<Ordinal>(instruction);
    auto addr = doubleWordAlign(valueFromSrc1Register<Ordinal>(instruction));
    DoubleRegister temp(loadLong(src));
    synchronizedStore(addr, temp);
    /// @todo figure out how to support bad access conditions
    ac_.setConditionCode(0b010);
}

void
Core::synmovq(const Instruction &instruction) noexcept {
    auto src = valueFromSrc2Register<Ordinal>(instruction);
    auto addr = quadWordAlign(valueFromSrc1Register<Ordinal>(instruction));
    QuadRegister temp = loadQuad(src);
    synchronizedStore(addr, temp);
    /// @todo figure out how to support bad access conditions
    ac_.setConditionCode(0b010);
}

void
Core::lockBus() noexcept {
    /// @todo implement this
}

void
Core::unlockBus() noexcept {
    /// @todo implement this
}

void
Core::ipRelativeBranch(const Instruction& inst) noexcept {
    advanceIPBy = 0;
    ip_.set<Integer>(ip_.get<Integer>() + inst.getDisplacement());
}
void
Core::absoluteBranch(Ordinal value) noexcept {
    advanceIPBy = 0; // we want to go to the exact position specified so do not advance
    ip_.set<Ordinal>(value);
}
void
Core::balx(const Instruction& inst) noexcept {
    auto address = computeMemoryAddress(inst);
    setDestinationFromSrcDest(inst, ip_.get<Ordinal>() + advanceIPBy, TreatAsOrdinal{});
    absoluteBranch(address);
}
void
Core::bx(const Instruction& inst) noexcept {
    absoluteBranch(computeMemoryAddress(inst));
}
void
Core::synchronizeMemoryRequests() noexcept {

}
void
Core::notbit(const Instruction& instruction) noexcept {
    auto bitpos = getBitPosition(valueFromSrc1Register<Ordinal>(instruction));
    auto src = valueFromSrc2Register<Ordinal>(instruction);
    setDestinationFromSrcDest(instruction, src ^ bitpos, TreatAsOrdinal{});
}

void
Core::shrdi(const Instruction &instruction) noexcept {
    // according to the manual, equivalent to divi value, 2 so that is what we're going to do for correctness sake
    auto len = valueFromSrc1Register<Integer>(instruction);
    if (auto& dest = destinationFromSrcDest(instruction); len < 32) {
        auto src = valueFromSrc2Register<Integer>(instruction);
        /// @todo verify that this does what we expect
        dest.set<Integer>(src / static_cast<Integer>(bitPositions[len]));
    } else {
        dest.set<Integer>(0);
    }
}
void
Core::checkPendingInterrupts() noexcept {
    /// @todo implement if it makes sense
}
void
Core::extract(const Instruction &instruction) noexcept {
    auto& dest = destinationFromSrcDest(instruction);
    auto bitpos = valueFromSrc1Register<Ordinal>(instruction);
    auto len = valueFromSrc2Register<Ordinal>(instruction);
    // taken from the Hx manual as it isn't insane
    auto shiftAmount = bitpos > 32 ? 32 : bitpos;
    dest.set<Ordinal>((dest.get<Ordinal>() >> shiftAmount) & ~(0xFFFF'FFFF << len));
}
/// @todo figure out how to reduce code size on this
void
Core::mov(const Instruction &inst) noexcept {
    setDestinationFromSrcDest(inst,
                              valueFromSrc1Register<Ordinal>(inst),
                              TreatAsOrdinal {});
}
void
Core::movl(const Instruction &instruction) noexcept {
    longDestinationFromSrcDest(instruction).set(valueFromSrc1Register<LongOrdinal>(instruction), TreatAsLongOrdinal {});
}
void
Core::movt(const Instruction &instruction) noexcept {
    destinationFromSrcDest(instruction, TreatAsTripleRegister{}).copy(sourceFromSrc1<TripleRegister>(instruction));
}
void
Core::movq(const Instruction &instruction) noexcept {
    destinationFromSrcDest(instruction, TreatAsQuadRegister{}).copy(sourceFromSrc1<QuadRegister>(instruction));
}
namespace {
    template<Ordinal mask>
    constexpr bool scanByte0(Ordinal a, Ordinal b) noexcept {
        return (a & mask) == (b & mask);
    }
}
void
Core::scanbyte(const Instruction &instruction) noexcept {
    if (auto src1 = valueFromSrc1Register<Ordinal>(instruction),
                src2 = valueFromSrc2Register<Ordinal>(instruction);
            scanByte0<0x0000'00FF>(src1, src2) ||
            scanByte0<0x0000'FF00>(src1, src2) ||
            scanByte0<0x00FF'0000>(src1, src2) ||
            scanByte0<0xFF00'0000>(src1, src2)) {
        ac_.setConditionCode(0b010);
    } else {
        ac_.clearConditionCode();
    }
}
void
Core::scanbit(const Instruction &instruction) noexcept {
    // perform a sanity check
    auto& dest = destinationFromSrcDest(instruction);
    auto src = valueFromSrc1Register<Ordinal>(instruction);
    dest.set<Ordinal>(0xFFFF'FFFF);
    ac_.clearConditionCode();
    Ordinal index = 31;
    for (auto mask : reverseBitPositions) {
        if ((src & mask) != 0) {
            dest.set<Ordinal>(index);
            ac_.setConditionCode(0b010);
            return;
        }
        --index;
    }
}

void
Core::spanbit(const Instruction &instruction) noexcept {
    auto& dest = destinationFromSrcDest(instruction);
    auto src = valueFromSrc1Register<Ordinal>(instruction);
    dest.set<Ordinal>(0xFFFF'FFFF);
    ac_.clearConditionCode();
    Ordinal index = 31;
    for (auto mask : reverseBitPositions) {
        if ((src & mask) == 0) {
            dest.set<Ordinal>(index);
            ac_.setConditionCode(0b010);
            return;
        }
        --index;
    }
}

void
Core::alterbit(const Instruction &instruction) noexcept {
    auto bitpos = bitPositions[valueFromSrc1Register<Ordinal>(instruction) & 0b11111];
    auto src = valueFromSrc2Register<Ordinal>(instruction);
    Ordinal result = src;
    if (; ac_.getConditionCode() & 0b010) {
        result |= bitpos;
    } else {
        result &= ~bitpos;
    }
    setDestinationFromSrcDest(instruction, result, TreatAsOrdinal{});
}
void
Core::modify(const Instruction &instruction) noexcept {
    // this is my encode operation but expanded out
    auto& dest = destinationFromSrcDest(instruction);
    auto mask = valueFromSrc1Register<Ordinal>(instruction);
    auto src = valueFromSrc2Register<Ordinal>(instruction);
    dest.set<Ordinal>(::modify(mask, src, dest.get<Ordinal>()));
}
void
Core::modi(const Instruction &instruction) noexcept {
    if (auto denominator = valueFromSrc1Register<Integer>(instruction); denominator == 0) {
        generateFault(FaultType::Arithmetic_ArithmeticZeroDivide);
    } else {
        auto numerator = valueFromSrc2Register<Integer>(instruction);
        auto result = numerator - ((numerator / denominator) * denominator);
        if (((numerator * denominator) < 0) && (result != 0)) {
            result += denominator;
        }
        setDestinationFromSrcDest(instruction, result, TreatAsInteger{});
    }
}
void
Core::ediv(const Instruction &instruction) noexcept {
    if (auto denomord = valueFromSrc1Register<Ordinal>(instruction); denomord == 0) {
        // raise an arithmetic zero divide fault
        generateFault(FaultType::Arithmetic_ArithmeticZeroDivide);
    } else {
        auto numerator = valueFromSrc2Register<LongOrdinal>(instruction);
        auto denominator = static_cast<LongOrdinal>(denomord);
        auto& dest = longDestinationFromSrcDest(instruction);
        // taken from the manual
        auto remainder = static_cast<Ordinal>(numerator - (numerator / denominator) * denominator);
        auto quotient = static_cast<Ordinal>(numerator / denominator);
        dest.set(remainder, 0, TreatAsOrdinal{});
        dest.set(quotient, 1, TreatAsOrdinal{});
    }
}
void
Core::emul(const Instruction &instruction) noexcept {
    auto src2 = static_cast<LongOrdinal>(valueFromSrc2Register<Ordinal>(instruction));
    auto src1 = static_cast<LongOrdinal>(valueFromSrc1Register<Ordinal>(instruction));
    // taken from the manual
    setDestinationFromSrcDest(instruction, src2 * src1, TreatAsLongOrdinal{});
}

void
Core::atadd(const Instruction& instruction) noexcept {
    // adds the src (src2 internally) value to the value in memory location specified with the addr (src1 in this case) operand.
    // The initial value from memory is stored in dst (internally src/dst).
    syncf();
    auto addr = wordAlign(valueFromSrc1Register<Ordinal>(instruction));
    lockBus();
    auto temp = load(addr);
    auto src = valueFromSrc2Register<Ordinal>(instruction);
    store(addr, temp + src);
    unlockBus();
    setDestinationFromSrcDest(instruction, temp, TreatAsOrdinal{});

}

void
Core::atmod(const Instruction &instruction) noexcept {
    // copies the src/dest value (logical version) into the memory location specifeid by src1.
    // The bits set in the mask (src2) operand select the bits to be modified in memory. The initial
    // value from memory is stored in src/dest
    syncf();
    auto addr = wordAlign(valueFromSrc1Register<Ordinal>(instruction));
    lockBus();
    auto temp = load(addr);
    auto& dest = destinationFromSrcDest(instruction);
    auto mask = valueFromSrc2Register<Ordinal>(instruction);
    store(addr, (dest.get<Ordinal>() & mask) | (temp & ~mask));
    unlockBus();
    dest.set<Ordinal>(temp);
}
void
Core::chkbit(const Instruction &instruction) noexcept {
    auto src = valueFromSrc2Register<Ordinal>(instruction);
    auto bitpos = getBitPosition(valueFromSrc1Register<Ordinal>(instruction));
    ac_.setConditionCode((src & bitpos) == 0 ? 0b000 : 0b010);
}

void
Core::setbit(const Instruction &instruction) noexcept {
    auto& dest = destinationFromSrcDest(instruction);
    auto bitpos = getBitPosition(valueFromSrc1Register<Ordinal>(instruction));
    auto src = valueFromSrc2Register<Ordinal>(instruction);
    dest.set<Ordinal>(src | bitpos);
}
void
Core::clrbit(const Instruction &instruction) noexcept {
    auto& dest = destinationFromSrcDest(instruction);
    auto bitpos = getBitPosition(valueFromSrc1Register<Ordinal>(instruction));
    auto src = valueFromSrc2Register<Ordinal>(instruction);
    dest.set<Ordinal>(src & ~bitpos);
}

void
Core::modpc(const Instruction &instruction) noexcept {
    auto mask = valueFromSrc1Register<Ordinal>(instruction);
    auto& dest = destinationFromSrcDest(instruction);
    if (mask != 0) {
        if (!pc_.inSupervisorMode()) {
            generateFault(FaultType::Type_Mismatch); /// @todo TYPE.MISMATCH
        } else {
            auto src = valueFromSrc2Register<Ordinal>(instruction);
            dest.set<Ordinal>(pc_.modify(mask, src));
            ProcessControls tmp(dest.get<Ordinal>());
            if (tmp.getPriority() > pc_.getPriority()) {
                checkPendingInterrupts();
            }
        }
    } else {
        dest.set<Ordinal>(pc_.getValue());
    }
}

void
Core::bal(const Instruction &inst) noexcept {
    getRegister(RegisterIndex::Global14).set<Ordinal>(ip_.get<Ordinal>() + 4);
    ipRelativeBranch(inst);
}

void
Core::illegalInstruction(const Instruction &inst) noexcept {
    generateFault(FaultType::Operation_InvalidOpcode) ;
}
void
Core::stob(const Instruction &instruction) noexcept {
    storeByte(computeMemoryAddress(instruction), valueFromSrcDestRegister<ByteOrdinal>(instruction));
}
void
Core::stib(const Instruction &instruction) noexcept {
    storeByteInteger(computeMemoryAddress(instruction), valueFromSrcDestRegister<ByteInteger>(instruction));
}
void
Core::stis(const Instruction &instruction) noexcept {
    storeShortInteger(computeMemoryAddress(instruction), valueFromSrcDestRegister<ShortInteger>(instruction));
}

void
Core::st(const Instruction &instruction) noexcept {
    store(computeMemoryAddress(instruction), valueFromSrcDestRegister<Ordinal>(instruction));
}
void
Core::stos(const Instruction &instruction) noexcept {
    storeShort(computeMemoryAddress(instruction), valueFromSrcDestRegister<ShortOrdinal>(instruction));
}
void
Core::stl(const Instruction &instruction) noexcept {
    storeLong(computeMemoryAddress(instruction), valueFromSrcDestRegister<LongOrdinal>(instruction));
}
void
Core::stt(const Instruction &instruction) noexcept {
    store(computeMemoryAddress(instruction), sourceFromSrcDest(instruction, TreatAsTripleRegister{}));
}
void
Core::stq(const Instruction &instruction) noexcept {
    store(computeMemoryAddress(instruction), sourceFromSrcDest(instruction, TreatAsQuadRegister{}));
}
void
Core::ldis(const Instruction &instruction) noexcept {
    setDestinationFromSrcDest(instruction, loadShort(computeMemoryAddress(instruction)), TreatAsInteger{});
}
void
Core::ldib(const Instruction &instruction) noexcept {
    setDestinationFromSrcDest(instruction, loadByte(computeMemoryAddress(instruction)), TreatAsInteger{});
}
void
Core::ldob(const Instruction &instruction) noexcept {
    setDestinationFromSrcDest(instruction, loadByte(computeMemoryAddress(instruction)), TreatAsOrdinal{});
}
void
Core::ldos(const Instruction &inst) noexcept {
    setDestinationFromSrcDest(inst, loadShort(computeMemoryAddress(inst)), TreatAsOrdinal{});
}
void
Core::ld(const Instruction &inst) noexcept {
    setDestinationFromSrcDest(inst, load(computeMemoryAddress(inst)), TreatAsOrdinal{});
}
void
Core::ldl(const Instruction &inst) noexcept {
    setDestinationFromSrcDest(inst, loadLong(computeMemoryAddress(inst)), TreatAsLongOrdinal{});
}
void
Core::ldt(const Instruction &inst) noexcept {
    load(computeMemoryAddress(inst), destinationFromSrcDest(inst, TreatAsTripleRegister{}));
}
void
Core::ldq(const Instruction &inst) noexcept {
    load(computeMemoryAddress(inst), destinationFromSrcDest(inst, TreatAsQuadRegister{}));
}
void
Core::modac(const Instruction &instruction) noexcept {
    // modac is different than most other i960 instructions... dest is src1... wow
    auto& dest = getRegister(instruction.getSrc1());
    auto mask = valueFromSrcDestRegister<Ordinal>(instruction);
    auto src = valueFromSrc2Register<Ordinal>(instruction);
    dest.set<Ordinal>(ac_.modify(mask, src));
}

void
Core::modtc(const Instruction &instruction) noexcept {
    // modtc is different than most other i960 instructions... dest is src1... wow
    auto& dest = getRegister(instruction.getSrc1());
    auto mask = valueFromSrcDestRegister<Ordinal>(instruction);
    auto src = valueFromSrc2Register<Ordinal>(instruction);
    dest.set<Ordinal>(tc_.modify(mask, src));
}
