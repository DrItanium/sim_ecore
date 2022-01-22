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

#include <Arduino.h>

#include <SPI.h>
#include <Wire.h>
#include "Types.h"
#include "Core.h"

void pinMode(Core::Pinout pin, decltype(OUTPUT) direction) noexcept {
    ::pinMode(static_cast<int>(pin), direction);
}
void
Core::begin() noexcept {
    Serial.begin(115200);
    while (!Serial);
    Serial.println(F("STARTING UP EAVR2e i960 Processor"));
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    Serial.print(F("BRINGING SPI UP..."));
    SPI.begin();
    Serial.println(F("DONE!"));
    Serial.print(F("BRINGING I2C UP..."));
    Wire.begin();
    Serial.println(F("DONE!"));
    for (const auto& ebiPin : Core::EBIExtendedPins) {
        pinMode(ebiPin, OUTPUT);
    }

    pinMode(Pinout::Int0_, INPUT);
    pinMode(Pinout::Int1_, INPUT);
    pinMode(Pinout::Int2_, INPUT);
    pinMode(Pinout::Int3_, INPUT);

    /// @todo setup all of the mega2560 peripherals here

}

void
Core::generateFault(FaultType faultKind) {
    Serial.print(F("FAULT GENERATED AT 0x"));
    Serial.print(ip_.getOrdinal(), HEX);
    Serial.println(F("! HALTING!!"));
    while (true) {
        delay(1000);
    }
}
void
Core::boot0(Ordinal sat, Ordinal pcb, Ordinal startIP) {
    systemAddressTableBase_ = sat;
    prcbBase_ = pcb;
    // skip the check words
    absoluteBranch(startIP);
    pc_.setPriority(31);
    pc_.setState(true); // needs to be set as interrupted
    auto thePointer = getInterruptStackPointer();
    // also make sure that we set the target pack to zero
    currentFrameIndex_ = 0;
    // invalidate all cache entries forcefully
    for (auto& a : frames) {
        a.relinquishOwnership();
        // at this point we want all of the locals to be cleared, this is the only time
        for (auto& reg : a.getUnderlyingFrame().dprs) {
            reg.setLongOrdinal(0);
        }
    }
    setFramePointer(thePointer);
    // we need to take ownership of the target frame on startup
    // we want to take ownership and throw anything out just in case so make the lambda do nothing
    getCurrentPack().takeOwnership(thePointer, [](const auto&, auto) noexcept { });
    // THE MANUAL DOESN'T STATE THAT YOU NEED TO SETUP SP and PFP as well, but you do!
    getStackPointer().setOrdinal(thePointer + 64);
    getPFP().setOrdinal(thePointer);
}
void
Core::boot() {
    auto q = loadQuad(0);
    boot0(q.getOrdinal(0), q.getOrdinal(1), q.getOrdinal(3));
}
Ordinal
Core::getSystemAddressTableBase() const noexcept {
    return systemAddressTableBase_;
}
Ordinal
Core::getPRCBPtrBase() const noexcept {
    return prcbBase_;
}

void
Core::processIACMessage(const IACMessage &message) noexcept {
    switch (message.getMessageType()) {
        case 0x89: // purge instruction cache
            // do nothing as we don't have an instruction cache
            break;
        case 0x93: // reinitialize processor
            boot0(message.getField3(), message.getField4(), message.getField5());
            break;
        case 0x8F:
            // set breakpoint register
            break;
        case 0x80:
            // store system base
            [this, &message]() {
                // stores the current locations of the system address table and the prcb in a specified location in memory.
                // The address of the system address table is stored in the word starting at the byte specified in field 3,
                // and the address of the PRCB is stored in the next word in memory (field 3 address plus 4)
                DoubleRegister pack(getSystemAddressTableBase(), getPRCBPtrBase());
                storeLong(message.getField3(), pack.getLongOrdinal());
            }();
            break;
        case 0x40: // interrupt
            // Generates an interrupt request. The interrup vector is given in field 1 of the IAC message. The processor handles the
            // interrupt request just as it does interrupts received from other sources. If the interrupt priority is higher than the prcessor's
            // current priority, the processor services the interrupt immediately. Otherwise, it posts the interrup in the pending interrupts
            // section of the interrupt table.
            break;
        case 0x41: // Test pending interrupts
            // tests for pending interrupts. The processor checks the pending interrupt section of the interrupt
            // table for a pending interrupt with a priority higher than the prcoessor's current priority. If a higher
            // priority interrupt is found, it is serviced immediately. Otherwise, no action is taken

            /// @todo implement this
            break;
        default:
            break;

    }
}
