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
const Register& Core::getSourceRegister(RegisterIndex targetIndex) const noexcept { return getRegister(targetIndex); }
Ordinal Core::valueFromSrc1Register(const Instruction& instruction, TreatAsOrdinal) const noexcept { return sourceFromSrc1(instruction).get<Ordinal>(); }
Integer Core::valueFromSrc1Register(const Instruction& instruction, TreatAsInteger) const noexcept { return sourceFromSrc1(instruction).get<Integer>(); }
Ordinal Core::valueFromSrc2Register(const Instruction& instruction, TreatAsOrdinal) const noexcept { return sourceFromSrc2(instruction).get<Ordinal>(); }
Integer Core::valueFromSrc2Register(const Instruction& instruction, TreatAsInteger) const noexcept { return sourceFromSrc2(instruction).get<Integer>(); }
LongOrdinal  Core::valueFromSrc2Register(const Instruction &instruction, TreatAsLongOrdinal) const noexcept { return getDoubleRegister(instruction.getSrc2()).getLongOrdinal(); }
const Register&
Core::sourceFromSrc1(const Instruction& instruction) const noexcept {
    return getSourceRegister(instruction.getSrc1());
}
const Register&
Core::sourceFromSrc2(const Instruction& instruction) const noexcept {
    return getSourceRegister(instruction.getSrc2());
}
Register&
Core::destinationFromSrcDest(const Instruction& instruction) noexcept {
    return getRegister(instruction.getSrcDest(false));
}
const Register&
Core::sourceFromSrcDest(const Instruction& instruction) const noexcept {
    return getRegister(instruction.getSrcDest(true));
}
void
Core::setDestinationFromSrcDest(const Instruction& instruction, Ordinal value, TreatAsOrdinal) {
    destinationFromSrcDest(instruction).set<Ordinal>(value);
}
void
Core::setDestinationFromSrcDest(const Instruction& instruction, Integer value, TreatAsInteger) {
    destinationFromSrcDest(instruction).setInteger(value);
}
void
Core::setDestinationFromSrcDest(const Instruction& instruction, LongOrdinal value, TreatAsLongOrdinal) {
    getDoubleRegister(instruction.getSrcDest(false)).setLongOrdinal(value);
}
DoubleRegister&
Core::longDestinationFromSrcDest(const Instruction& instruction) noexcept {
    return getDoubleRegister(instruction.getSrcDest(false));
}
const DoubleRegister&
Core::longSourceFromSrcDest(const Instruction &instruction) const noexcept {
    return getDoubleRegister(instruction.getSrcDest(true));
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
        Serial.println(getRIP().getOrdinal(), HEX);
    }
    advanceIPBy = 4;
    auto instruction = loadInstruction(ip_.getOrdinal());
    executeInstruction(instruction);
    //executeInstruction(loadInstruction(ip_.getOrdinal()));
    if (advanceIPBy > 0)  {
        ip_.set<Ordinal>(ip_.getOrdinal() + advanceIPBy);
    }
    if constexpr (EnableEmulatorTrace) {
        Serial.print(F("\trip(after): 0x"));
        Serial.println(getRIP().getOrdinal(), HEX);
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

const Register&
Core::getRegister(RegisterIndex targetIndex) const noexcept {
    if (isLocalRegister(targetIndex)) {
        return getLocals().getRegister(static_cast<uint8_t>(targetIndex));
    } else if (isGlobalRegister(targetIndex)) {
        return globals.getRegister(static_cast<uint8_t>(targetIndex));
    } else if (isLiteral(targetIndex)) {
        return OrdinalLiterals[static_cast<uint8_t>(targetIndex) & 0b11111];
    } else {
        //generateFault(FaultType::Operation_InvalidOperand);
        return BadRegister;
    }
}

const DoubleRegister&
Core::getDoubleRegister(RegisterIndex targetIndex) const noexcept {
    if (isLocalRegister(targetIndex)) {
        return getLocals().getDoubleRegister(static_cast<uint8_t>(targetIndex));
    } else if (isGlobalRegister(targetIndex)) {
        return globals.getDoubleRegister(static_cast<uint8_t>(targetIndex));
    } else if (isLiteral(targetIndex)) {
        /// @todo implement double register literal support, according to the docs it is allowed
        return LongOrdinalLiterals[static_cast<uint8_t>(targetIndex) & 0b11111];
    } else {
        //generateFault(FaultType::Operation_InvalidOperand);
        return BadRegisterDouble;
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
        store(baseAddress, theFrame.getRegister(i).getOrdinal());
    }
}

void
Core::restoreRegisterFrame(RegisterFrame &theFrame, Address baseAddress) noexcept {
    for (auto& reg : theFrame.gprs) {
        reg.set<Ordinal>(load(baseAddress));
        baseAddress += 4;
    }
}

const Register&
Core::getAbaseRegister(const Instruction& inst) const noexcept {
    return getSourceRegister(inst.getABase());
}
const Register&
Core::getIndexRegister(const Instruction& inst) const noexcept {
    return getSourceRegister(inst.getIndex());
}
Ordinal
Core::valueFromAbaseRegister(const Instruction& inst, TreatAsOrdinal) const noexcept {
    return getAbaseRegister(inst).get<Ordinal>();
}
Integer
Core::valueFromAbaseRegister(const Instruction& inst, TreatAsInteger) const noexcept {
    return getAbaseRegister(inst).get<Integer>();
}
Ordinal
Core::valueFromIndexRegister(const Instruction& inst, TreatAsOrdinal) const noexcept {
    return getIndexRegister(inst).get<Ordinal>();
}
Integer
Core::valueFromIndexRegister(const Instruction& inst, TreatAsInteger) const noexcept {
    return getIndexRegister(inst).get<Integer>();
}
Ordinal
Core::scaledValueFromIndexRegister(const Instruction& inst, TreatAsOrdinal) const noexcept {
    return valueFromIndexRegister(inst, TreatAsOrdinal{}) << inst.getScale();
}
Integer
Core::scaledValueFromIndexRegister(const Instruction& inst, TreatAsInteger) const noexcept {
    return valueFromIndexRegister(inst, TreatAsInteger{}) << inst.getScale();
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
            return instruction.getOffset() + valueFromAbaseRegister(instruction, TreatAsOrdinal{});
        case MEMFormatMode::MEMB_RegisterIndirect:
            return valueFromAbaseRegister(instruction, TreatAsOrdinal{});
        case MEMFormatMode::MEMB_RegisterIndirectWithIndex:
            return valueFromAbaseRegister(instruction, TreatAsOrdinal{}) + scaledValueFromIndexRegister(instruction, TreatAsOrdinal{});
        case MEMFormatMode::MEMB_IPWithDisplacement:
            return static_cast<Ordinal>(ip_.getInteger() + instruction.getDisplacement() + 8);
        case MEMFormatMode::MEMB_AbsoluteDisplacement:
            return instruction.getDisplacement(); // this will return the optional displacement
        case MEMFormatMode::MEMB_RegisterIndirectWithDisplacement: {
            return static_cast<Ordinal>(valueFromAbaseRegister(instruction, TreatAsInteger{}) + instruction.getDisplacement());
        }
        case MEMFormatMode::MEMB_IndexWithDisplacement:
            return static_cast<Ordinal>(scaledValueFromIndexRegister(instruction, TreatAsInteger{}) + instruction.getDisplacement());
        case MEMFormatMode::MEMB_RegisterIndirectWithIndexAndDisplacement:
            return static_cast<Ordinal>(
                    valueFromAbaseRegister(instruction, TreatAsInteger{}) +
                    scaledValueFromIndexRegister(instruction, TreatAsInteger{}) +
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
    auto src = valueFromSrc2Register(instruction, TreatAsOrdinal{});
    auto bitpos = getBitPosition(valueFromSrc1Register(instruction, TreatAsOrdinal{}));
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
    auto src = valueFromSrc2Register(instruction, TreatAsOrdinal{});
    auto bitpos = getBitPosition(valueFromSrc1Register(instruction, TreatAsOrdinal{}));
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

void
Core::setDestination(RegisterIndex index, Ordinal value, TreatAsOrdinal) {
    getRegister(index).set<Ordinal>(value);
}
void
Core::setDestination(RegisterIndex index, Integer value, TreatAsInteger) {
    getRegister(index).set<Integer>(value);
}

Core::Core(Ordinal salign) : ip_(0), ac_(0), pc_(0), tc_(0), salign_(salign), c_((salign * 16) - 1), stackAlignMask_(c_ - 1), frameAlignmentMask_(~stackAlignMask_) {
}

void
Core::flushreg() noexcept {
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
    return getStackPointer().get<Ordinal>();
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
    ip_.setInteger(ip_.getInteger() + instruction.getDisplacement());
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
    if (auto targ = valueFromSrc1Register(instruction, TreatAsOrdinal{}); targ > 259) {
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
Core::ret() noexcept {
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
    return getFramePointer().getAddress();
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
    constexpr LongOrdinal addWithCarry(Ordinal src2, Ordinal src1, Ordinal carry) noexcept {
        return static_cast<LongOrdinal>(src2) + static_cast<LongOrdinal>(src1) + carry;
    }
    constexpr LongOrdinal subtractWithCarry(Ordinal src2, Ordinal src1, Ordinal carry) noexcept {
        return static_cast<LongOrdinal>(src2) - static_cast<LongOrdinal>(src1) - 1 + carry;
    }
    bool overflowDetected(const Register& src2, const Register& src1, const Register& dest) noexcept {
        return (src2.getMostSignificantBit() == src1.getMostSignificantBit()) && (src2.getMostSignificantBit() != dest.getMostSignificantBit());
    }
}
void
Core::withCarryOperationGeneric(const Instruction &instruction, ArithmeticWithCarryOperation op) noexcept {
    auto& src1 = sourceFromSrc1(instruction);
    auto& src2 = sourceFromSrc2(instruction);
    auto carry = ac_.getCarryBit() ? 1 : 0;
    DoubleRegister result((op == ArithmeticWithCarryOperation::Add) ? addWithCarry(src2.get<Ordinal>(), src1.get<Ordinal>(), carry) : subtractWithCarry(src2.get<Ordinal>(), src1.get<Ordinal>(), carry));
    auto& dest = destinationFromSrcDest(instruction);
    dest.set<Ordinal>(result.getOrdinal(0));
    ac_.setConditionCode(0);
    if (overflowDetected(src2, src1, dest)) {
        // set the overflow bit in ac
        ac_.setOverflowBit(true);
    }
    ac_.setCarryBit(result.getOrdinal(1) != 0);
    // set the carry out bit
}
void Core::synld(const Instruction& instruction) noexcept {
    // wait until another execution unit sets the condition codes to continue after requesting a load.
    // In the case of this emulator, it really doesn't mean anything but I can see this being a synld followed by a wait
    // for synchronization. It also allows access to internal memory mapped items.
    // So I'm not sure how to implement this yet, however I think at this point I'm just going to treat is as a special kind of load
    // with condition code assignments and forced alignments
    auto address = sourceFromSrc1(instruction).getWordAligned(); // force word alignment
    // load basically takes care of accessing different registers and such even memory mapped ones
    setDestinationFromSrcDest(instruction, load(address), TreatAsOrdinal{});
    // there is a _fail_ condition where a bad access condition will result in 0b000
    /// @todo implement support for bad access conditions
    ac_.setConditionCode(0b010);

}

void
Core::synmov(const Instruction &instruction) noexcept {
    // load from memory and then store to another address in a synchronous fashion
    auto src = valueFromSrc2Register(instruction, TreatAsOrdinal{});
    auto addr = sourceFromSrc1(instruction).getWordAligned(); // align
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
    auto src = valueFromSrc2Register(instruction, TreatAsOrdinal{}); // source address
    auto addr = sourceFromSrc1(instruction).getDoubleWordAligned(); // align
    DoubleRegister temp(loadLong(src));
    synchronizedStore(addr, temp);
    /// @todo figure out how to support bad access conditions
    ac_.setConditionCode(0b010);
}

void
Core::synmovq(const Instruction &instruction) noexcept {
    auto src = valueFromSrc2Register(instruction, TreatAsOrdinal{}); // source address
    auto addr = sourceFromSrc1(instruction).getQuadWordAligned(); // align
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
    ip_.setInteger(ip_.getInteger() + inst.getDisplacement());
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
    auto bitpos = getBitPosition(valueFromSrc1Register(instruction, TreatAsOrdinal{}));
    auto src = valueFromSrc2Register(instruction, TreatAsOrdinal{});
    setDestinationFromSrcDest(instruction, src ^ bitpos, TreatAsOrdinal{});
}

void
Core::shrdi(const Instruction &instruction) noexcept {
    // according to the manual, equivalent to divi value, 2 so that is what we're going to do for correctness sake
    auto len = valueFromSrc1Register(instruction, TreatAsInteger{});
    if (auto& dest = destinationFromSrcDest(instruction); len < 32) {
        auto src = valueFromSrc2Register(instruction, TreatAsInteger{});
        /// @todo verify that this does what we expect
        dest.setInteger(src / static_cast<Integer>(bitPositions[len]));
    } else {
        dest.setInteger(0);
    }
}
void
Core::checkPendingInterrupts() noexcept {
    /// @todo implement if it makes sense
}
void
Core::extract(const Instruction &instruction) noexcept {
    auto& dest = destinationFromSrcDest(instruction);
    auto bitpos = valueFromSrc1Register(instruction, TreatAsOrdinal{});
    auto len = valueFromSrc2Register(instruction, TreatAsOrdinal{});
    // taken from the Hx manual as it isn't insane
    auto shiftAmount = bitpos > 32 ? 32 : bitpos;
    dest.set<Ordinal>((dest.get<Ordinal>() >> shiftAmount) & ~(0xFFFF'FFFF << len));
}
/// @todo figure out how to reduce code size on this
void
Core::mov(const Instruction &inst) noexcept {
    setDestinationFromSrcDest(inst,
                              valueFromSrc1Register(inst, TreatAsOrdinal{}),
                              TreatAsOrdinal {});
}
void
Core::movl(const Instruction &instruction) noexcept {
    const auto& src = getSourceDoubleRegister(instruction.getSrc1());
    longDestinationFromSrcDest(instruction).copy(src);
}
void
Core::movt(const Instruction &instruction) noexcept {
    auto& dest = getTripleRegister(instruction.getSrcDest(false));
    const auto& src = getTripleRegister(instruction.getSrc1());
    dest.copy(src);
}
void
Core::movq(const Instruction &instruction) noexcept {
    auto& dest = getQuadRegister(instruction.getSrcDest(false));
    const auto& src = getQuadRegister(instruction.getSrc1());
    dest.copy(src);
}
void
Core::scanbyte(const Instruction &instruction) noexcept {
    const auto& src1 = sourceFromSrc1(instruction);
    const auto& src2 = sourceFromSrc2(instruction);
    auto bytesEqual = [&src1, &src2](int which) constexpr { return src1.getByteOrdinal(which) == src2.getByteOrdinal(which); };
    ac_.setConditionCode((bytesEqual(0) ||
                          bytesEqual(1) ||
                          bytesEqual(2) ||
                          bytesEqual(3)) ? 0b010 : 0b000);
}
void
Core::scanbit(const Instruction &instruction) noexcept {
    // perform a sanity check
    auto& dest = destinationFromSrcDest(instruction);
    auto src = valueFromSrc1Register(instruction, TreatAsOrdinal{});
    dest.set<Ordinal>(0xFFFF'FFFF);
    ac_.setConditionCode(0);
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
    auto src = valueFromSrc1Register(instruction, TreatAsOrdinal{});
    dest.set<Ordinal>(0xFFFF'FFFF);
    ac_.setConditionCode(0);
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
    auto bitpos = bitPositions[valueFromSrc1Register(instruction, TreatAsOrdinal{}) & 0b11111];
    auto src = valueFromSrc2Register(instruction, TreatAsOrdinal{});
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
    auto mask = valueFromSrc1Register(instruction, TreatAsOrdinal{});
    auto src = valueFromSrc2Register(instruction, TreatAsOrdinal{});
    dest.set<Ordinal>(::modify(mask, src, dest.get<Ordinal>()));
}
void
Core::modi(const Instruction &instruction) noexcept {
    if (auto denominator = valueFromSrc1Register(instruction, TreatAsInteger{}); denominator == 0) {
        generateFault(FaultType::Arithmetic_ArithmeticZeroDivide);
    } else {
        auto numerator = valueFromSrc2Register(instruction, TreatAsInteger{});
        auto result = numerator - ((numerator / denominator) * denominator);
        if (((numerator * denominator) < 0) && (result != 0)) {
            result += denominator;
        }
        setDestinationFromSrcDest(instruction, result, TreatAsInteger{});
    }
}
void
Core::modac(const Instruction &instruction) noexcept {
    auto& dest = destinationFromSrcDest(instruction);
    auto mask = valueFromSrc1Register(instruction, TreatAsOrdinal{});
    auto src = valueFromSrc2Register(instruction, TreatAsOrdinal{});
    dest.set<Ordinal>(ac_.modify(mask, src));
}
void
Core::ediv(const Instruction &instruction) noexcept {
    if (auto denomord = valueFromSrc1Register(instruction, TreatAsOrdinal{}); denomord == 0) {
        // raise an arithmetic zero divide fault
        generateFault(FaultType::Arithmetic_ArithmeticZeroDivide);
    } else {
        auto numerator = valueFromSrc2Register(instruction, TreatAsLongOrdinal{});
        auto denominator = static_cast<LongOrdinal>(denomord);
        auto& dest = longDestinationFromSrcDest(instruction);
        // taken from the manual
        auto remainder = static_cast<Ordinal>(numerator - (numerator / denominator) * denominator);
        auto quotient = static_cast<Ordinal>(numerator / denominator);
        dest.setOrdinal(remainder, 0);
        dest.setOrdinal(quotient, 1);
    }
}
void
Core::emul(const Instruction &instruction) noexcept {
    auto src2 = static_cast<LongOrdinal>(valueFromSrc2Register(instruction, TreatAsOrdinal{}));
    auto src1 = static_cast<LongOrdinal>(valueFromSrc1Register(instruction, TreatAsOrdinal{}));
    // taken from the manual
    setDestinationFromSrcDest(instruction, src2 * src1, TreatAsLongOrdinal{});
}

void
Core::atadd(const Instruction& instruction) noexcept {
    // adds the src (src2 internally) value to the value in memory location specified with the addr (src1 in this case) operand.
    // The initial value from memory is stored in dst (internally src/dst).
    syncf();
    auto addr = sourceFromSrc1(instruction).getWordAligned(); // force alignment to word boundary
    lockBus();
    auto temp = load(addr);
    auto src = valueFromSrc2Register(instruction, TreatAsOrdinal{});
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

    auto addr = sourceFromSrc1(instruction).getWordAligned(); // force alignment to word boundary
    lockBus();
    auto temp = load(addr);
    auto& dest = destinationFromSrcDest(instruction);
    auto mask = valueFromSrc2Register(instruction, TreatAsOrdinal{});
    store(addr, (dest.get<Ordinal>() & mask) | (temp & ~mask));
    unlockBus();
    dest.set<Ordinal>(temp);
}
void
Core::chkbit(const Instruction &instruction) noexcept {
    auto src = valueFromSrc2Register(instruction, TreatAsOrdinal{});
    auto bitpos = getBitPosition(valueFromSrc1Register(instruction, TreatAsOrdinal{}));
    ac_.setConditionCode((src & bitpos) == 0 ? 0b000 : 0b010);
}

void
Core::setbit(const Instruction &instruction) noexcept {
    auto& dest = destinationFromSrcDest(instruction);
    auto bitpos = getBitPosition(valueFromSrc1Register(instruction, TreatAsOrdinal{}));
    auto src = valueFromSrc2Register(instruction, TreatAsOrdinal{});
    dest.set<Ordinal>(src | bitpos);
}
void
Core::clrbit(const Instruction &instruction) noexcept {
    auto& dest = destinationFromSrcDest(instruction);
    auto bitpos = getBitPosition(valueFromSrc1Register(instruction, TreatAsOrdinal{}));
    auto src = valueFromSrc2Register(instruction, TreatAsOrdinal{});
    dest.set<Ordinal>(src & ~bitpos);
}

void
Core::modpc(const Instruction &instruction) noexcept {
    auto mask = valueFromSrc1Register(instruction, TreatAsOrdinal{});
    auto& dest = destinationFromSrcDest(instruction);
    if (mask != 0) {
        if (!pc_.inSupervisorMode()) {
            generateFault(FaultType::Type_Mismatch); /// @todo TYPE.MISMATCH
        } else {
            auto src = valueFromSrc2Register(instruction, TreatAsOrdinal{});
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
Core::modtc(const Instruction &instruction) noexcept {
    auto mask = valueFromSrc1Register(instruction, TreatAsOrdinal{});
    auto src = valueFromSrc2Register(instruction, TreatAsOrdinal{});
    setDestinationFromSrcDest(instruction, tc_.modify(mask, src), TreatAsOrdinal {});
}
void
Core::bal(const Instruction &inst) noexcept {
    setDestination(RegisterIndex::Global14, ip_.get<Ordinal>() + 4, TreatAsOrdinal{});
    ipRelativeBranch(inst);
}