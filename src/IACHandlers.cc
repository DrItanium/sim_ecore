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
//
// Created by jwscoggins on 2/3/22.
//

#include "Core.h"
void
Core::purgeInstructionCache(const IACMessage &message) noexcept {
// do nothing as we don't have an instruction cache
// this is not the register cache!
/// @todo implement
}

void
Core::reinitializeProcessor(const IACMessage &message) noexcept {
    boot0(message.getField3(), message.getField4(), message.getField5());
}
void
Core::setBreakpointRegister(const IACMessage &message) noexcept {
/// @todo implement
}
void
Core::storeSystemBase(const IACMessage &message) noexcept {
// stores the current locations of the system address table and the prcb in a specified location in memory.
// The address of the system address table is stored in the word starting at the byte specified in field 3,
// and the address of the PRCB is stored in the next word in memory (field 3 address plus 4)
    DoubleRegister pack(getSystemAddressTableBase(), getPRCBPtrBase());
    storeLong(message.getField3(), pack.get(TreatAsLongOrdinal{}));
}
void
Core::generateSystemInterrupt(const IACMessage &message) noexcept {
// Generates an interrupt request. The interrup vector is given in field 1 of the IAC message. The processor handles the
// interrupt request just as it does interrupts received from other sources. If the interrupt priority is higher than the prcessor's
// current priority, the processor services the interrupt immediately. Otherwise, it posts the interrup in the pending interrupts
// section of the interrupt table.
}
void
Core::testPendingInterrupts(const IACMessage &message) noexcept {
// tests for pending interrupts. The processor checks the pending interrupt section of the interrupt
// table for a pending interrupt with a priority higher than the prcoessor's current priority. If a higher
// priority interrupt is found, it is serviced immediately. Otherwise, no action is taken

/// @todo implement this
}
void
Core::processIACMessage(const IACMessage &message) noexcept {
    switch (message.getMessageType()) {
        case 0x89: // purge instruction cache
            purgeInstructionCache(message);
            break;
        case 0x93: // reinitialize processor
            reinitializeProcessor(message);
            break;
        case 0x8F: // set breakpoint register
            setBreakpointRegister(message);
            break;
        case 0x80: // store system base
            storeSystemBase(message);
            break;
        case 0x40: // interrupt
            generateSystemInterrupt(message);
            break;
        case 0x41: // Test pending interrupts
            testPendingInterrupts(message);
            break;
// Custom IAC messages start here
        case 0x00: // do normal boot startup
            Serial.println(F("BOOTING!"));
            boot();
            break;
        case 0x01: // checksum fail procedure
            Serial.println(F("CHECKSUM FAIL!"));
            checksumFail();
            break;
        default:
            break;

    }
}
