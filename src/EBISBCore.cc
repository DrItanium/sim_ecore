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
#include <EEPROM.h>
#include "Types.h"
#include "Core.h"

void
pinMode(Pinout pin, decltype(OUTPUT) direction) noexcept {
    ::pinMode(static_cast<int>(pin), direction);
}
void
digitalWrite(Pinout pin, decltype(LOW) value) noexcept {
    ::digitalWrite(static_cast<byte>(pin), value);
}
byte
digitalRead(Pinout p) noexcept {
    return ::digitalRead(static_cast<byte>(p));
}
namespace
{
    void
    setupEBI() noexcept {
        Serial.print(F("Enabling EBI..."));
        // full 64k space without bus keeper
        XMCRB = 0b0000'0000;
        // turn on the EBI
        XMCRA |= _BV(SRE);
        Serial.println(F("DONE!"));
        Serial.print(F("Setting up Extended EBI..."));
        for (const auto &ebiPin: EBIExtendedPins) {
            pinMode(ebiPin, OUTPUT);
            digitalWrite(ebiPin, LOW);
        }
        Serial.println(F("DONE!"));
    }
    void
    setupInternalConfigurationSpace() noexcept {
        Serial.print(F("BRINGING UP INTERNAL CONFIGURATION SPACE EEPROM..."));
        EEPROM.begin();
        // okay we need to check to make sure that the configuration space contains the values in question that we expect.
        // The 4k of EEPROM we have onboard is meant to hold onto internal device addresses
        // Like the hard i960 devices, we are going to set the addresses of all devices into the 0xFF00'0000 address space.
        // I am planning to use the upper most 4k to hold onto this special configuration space

        // we want to check out the configuration space and install any important base addresses there
        for (int i = 0, addr = 0; i < static_cast<int>(Builtin::Devices::Count); ++i, addr += sizeof(Address)) {
            EEPROM.put(addr, Builtin::computeBaseAddress(static_cast<Builtin::Devices>(i)));
        }
        Serial.println(F("DONE!"));
    }
    void
    setupInterruptPins() noexcept {
        Serial.print(F("Setting up Interrupt Pins..."));
        pinMode(Pinout::Int0_, INPUT);
        pinMode(Pinout::Int1_, INPUT);
        pinMode(Pinout::Int2_, INPUT);
        pinMode(Pinout::Int3_, INPUT);
        /// @todo attach to interrupt handlers
        Serial.println(F("DONE!"));

    }
    void
    bringUpSPI() noexcept {
        Serial.print(F("BRINGING SPI UP..."));
        SPI.begin();
        Serial.println(F("DONE!"));
    }
    void
    bringUpI2C() noexcept {

        Serial.print(F("BRINGING I2C UP..."));
        Wire.begin();
        Serial.println(F("DONE!"));
    }
    void
    bringUpSerial() noexcept {
        Serial.begin(115200);
        while (!Serial);
        Serial.println(F("STARTING UP EAVR2e i960 Processor"));
    }
    void
    configureLED() noexcept {
        pinMode(LED_BUILTIN, OUTPUT);
        digitalWrite(LED_BUILTIN, LOW);
    }
}
void
haltExecution(const __FlashStringHelper* message) noexcept {
    Serial.print(F("HALTING EXECUTION: "));
    Serial.println(message);
    while (true) {
        delay(1000);
    }
}
void
Core::begin() noexcept {
    bringUpSerial();
    setupEBI();
    setupInterruptPins();
    setupInternalConfigurationSpace();
    configureLED();
    // these peripherals are special because I'm not sure that it makes complete sense to expose the raw details to the emulation
    bringUpSPI();
    bringUpI2C();
    /// @todo setup all of the mega2560 peripherals here
    boot(Builtin::InternalBootProgramBase);
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
    Serial.print(F("\tInterrupt Stack Pointer: 0x"));
    Serial.println(thePointer, HEX);
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
Core::boot(Address base) {
    auto q = loadQuad(base);
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
Core::purgeInstructionCache(const IACMessage &message) noexcept {
    // do nothing as we don't have an instruction cache
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
    storeLong(message.getField3(), pack.getLongOrdinal());
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
        case 0x00: // do normal boot startup
            boot();
            break;
        case 0x01: // checksum fail procedure
            checksumFail();
            break;
        default:
            break;

    }
}

void
Core::checksumFail() noexcept {
    digitalWrite(LED_BUILTIN, HIGH);
    while (true) {
        delay(1000);
    }
}