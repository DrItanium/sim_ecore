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
// Created by jwscoggins on 8/21/21.
//

#ifdef ARDUINO_AVR_ATmega1284
#include <Arduino.h>
#include <Wire.h>
#include <SdFat.h>
#include <SPI.h>
#include "Types.h"
#include "HitagiSBCore.h"

SdFat SD;
namespace {
    SPISettings fullSpeed(10_MHz, MSBFIRST, SPI_MODE0);
    SPISettings psramSettings(5_MHz, MSBFIRST, SPI_MODE0);
}
void
HitagiSBCore::begin() {
    // hold the i960 in reset for the rest of execution, we really don't care about anything else with respect to this processor now
    Serial.println(F("BRINGING UP HITAGI SBCORE EMULATOR"));
    Serial.println(F("PUTTING THE CONNECTED i960 INTO RESET PERMANENTLY!"));
    pinMode(HitagiChipsetPinout::RESET960_, OUTPUT);
    digitalWrite(HitagiChipsetPinout::RESET960_, LOW);
    pinMode(HitagiChipsetPinout::PSRAM_EN_, OUTPUT);
    digitalWrite(HitagiChipsetPinout::PSRAM_EN_, HIGH);
    pinMode(HitagiChipsetPinout::SPI_OFFSET0, OUTPUT);
    pinMode(HitagiChipsetPinout::SPI_OFFSET1, OUTPUT);
    pinMode(HitagiChipsetPinout::SPI_OFFSET2, OUTPUT);
    digitalWrite(HitagiChipsetPinout::SPI_OFFSET0, LOW);
    digitalWrite(HitagiChipsetPinout::SPI_OFFSET1, LOW);
    digitalWrite(HitagiChipsetPinout::SPI_OFFSET2, LOW);
    SPI.begin();
    while (!SD.begin(static_cast<int>(HitagiChipsetPinout::SD_EN_), 10_MHz)) {
        Serial.println(F("NO SDCARD...WILL TRY AGAIN!"));
        delay(1000);
    }
    Serial.println(F("SD CARD UP!"));
    if (auto theFile = SD.open("boot.sys", FILE_READ); !theFile) {
            Serial.println(F("Could not open \"boot.sys\"! SD CARD may be corrupt?")) ;
            while (true) { delay(1000); }
    } else {
        Serial.println(F("OPENED BOOT.SYS!"));
        Serial.println(F("SETTING UP THE PSRAM CHIPS"));
        setupPSRAMChips();
        Address size = theFile.size();
        Serial.println(F("COPYING \"boot.sys\" to PSRAM"));
        SPI.beginTransaction(SPISettings(5_MHz, MSBFIRST, SPI_MODE0));
        for (Address addr = 0; addr < size; addr += PSRAMCopyBufferSize) {
            auto numRead = theFile.readBytes(psramCopyBuffer, PSRAMCopyBufferSize) ;
            (void) psramBlockWrite<false>(addr, psramCopyBuffer, numRead);
            Serial.print(F("."));
        }
        SPI.endTransaction();
        Serial.println(F("TRANSFER COMPLETE!!!"));
        theFile.close();
    }
    /// @todo implement support for other features as well
    for (auto& a : lines_) {
        a.clear();
    }
}

void
HitagiSBCore::setupPSRAMChips() noexcept {
    delayMicroseconds(200); // give the psram enough time to come up regardless of when this called
    SPI.beginTransaction(psramSettings);
    for (int i = 0; i < 8; ++i) {
        setPSRAMId(i);
        digitalWrite<i960Pinout::PSRAM_EN_, LOW>();
        SPI.transfer(0x66);
        digitalWrite<i960Pinout::PSRAM_EN_, HIGH>();
        asm volatile ("nop");
        asm volatile ("nop");
        asm volatile ("nop");
        asm volatile ("nop");
        digitalWrite<i960Pinout::PSRAM_EN_, LOW>();
        SPI.transfer(0x99);
        digitalWrite<i960Pinout::PSRAM_EN_, HIGH>();

    }
    SPI.endTransaction();
}
void
HitagiSBCore::setPSRAMId(byte id) noexcept {
    if (id != chipId_) {
        chipId_ = id;
        digitalWrite<HitagiChipsetPinout::SPI_OFFSET0>(id & 0b001 ? HIGH : LOW);
        digitalWrite<HitagiChipsetPinout::SPI_OFFSET1>(id & 0b010 ? HIGH : LOW);
        digitalWrite<HitagiChipsetPinout::SPI_OFFSET2>(id & 0b100 ? HIGH : LOW);
    }
}

size_t
HitagiSBCore::psramBlockRead(Address address, byte *buf, size_t count) {
    SPI.beginTransaction(psramSettings);
    MemoryCell32 translated(address);
    auto singleOperation = [&translated](byte* buf, size_t count) {
        digitalWrite<i960Pinout::PSRAM_EN_, LOW>();
        SPI.transfer(0x03);
        SPI.transfer(translated.getByteOrdinal(2));
        SPI.transfer(translated.getByteOrdinal(1));
        SPI.transfer(translated.getByteOrdinal(0));
        SPI.transfer(buf, count);
        digitalWrite<i960Pinout::PSRAM_EN_, HIGH>();
    };
    Address26 curr(address);
    Address26 end(address + count);
    if (curr.getIndex() == end.getAddress()) {
        // okay they are part of the same chip so we can safely just do a single operation
        setPSRAMId(curr.getIndex());
        singleOperation(buf, count);
    } else {
        // okay we are going to span multiple addresses
        // we need to compute how much will go into each area.
        // This isn't as much of a problem because a size_t is 16-bits on the 1284p
        // We will never have to span more than two psram chips which is very welcome.
        // We can easily compute the number of bytes that are going to be transferred
        // from the second chip and subtract that from the total count. That will be
        // the number of bytes to be transferred from the first chip

        // note that on any other target, this assumption would be wrong as size_t could be 4 or 8 bytes.
        // In that case, we would need to do some full chip spanning
        auto numBytesToSecondChip = end.getOffset();
        auto numBytesToFirstChip = count - numBytesToSecondChip;
        setPSRAMId(curr.getIndex());
        singleOperation(buf, numBytesToFirstChip);
        setPSRAMId(end.getIndex());
        // jump ahead to the left overs and retrieve them
        singleOperation(buf + numBytesToFirstChip, numBytesToSecondChip);
    }
    SPI.endTransaction();
    return count;
}



ByteOrdinal
HitagiSBCore::ioSpaceLoad(Address address, TreatAsByteOrdinal) {
    return 0;
}
void
HitagiSBCore::ioSpaceStore(Address address, ByteOrdinal value) {
    // nothing to do here right now
}
Ordinal
HitagiSBCore::ioSpaceLoad(Address address, TreatAsOrdinal ) {
    // right now there is nothing to do here
    return 0;
}
ShortOrdinal
HitagiSBCore::ioSpaceLoad(Address address, TreatAsShortOrdinal) {
    switch (address) {
        case 0:
            Serial.flush();
            break;
        case 2:
            return Serial.available();
        case 4:
            return Serial.availableForWrite();
        case 6:
            return Serial.read();
        default:
            break;
    }
    return 0;
}

void
HitagiSBCore::ioSpaceStore(Address address, ShortOrdinal value) {
    switch (address) {
        case 0:
            Serial.flush();
            break;
        case 6:
            Serial.write(static_cast<char>(value));
            break;
        default:
            break;
    }
}
void
HitagiSBCore::ioSpaceStore(Address address, Ordinal value) {
    // nothing to do right now
}
ByteOrdinal
HitagiSBCore::doIACLoad(Address address, TreatAsByteOrdinal ordinal) {
    return 0;
}
ShortOrdinal
HitagiSBCore::doIACLoad(Address address, TreatAsShortOrdinal ordinal) {
    return 0;
}
void
HitagiSBCore::doIACStore(Address address, ByteOrdinal value) {
    // do nothing
}
void
HitagiSBCore::doIACStore(Address address, ShortOrdinal value) {
    // do nothing
}
Ordinal
HitagiSBCore::doIACLoad(Address address, TreatAsOrdinal) {
    switch (address) {
        case HaltRegisterOffset:
            haltExecution();
            break;
        case ConsoleRegisterOffset:
            return static_cast<Ordinal>(Serial.read());
        case ConsoleFlushOffset:
            Serial.flush();
            break;
        default:
            break;
    }
    return 0;
}
void
HitagiSBCore::doIACStore(Address address, Ordinal value) {
    switch (address) {
        case HaltRegisterOffset:
            haltExecution();
            break;
        case ConsoleRegisterOffset:
            Serial.write(static_cast<char>(value));
        case ConsoleFlushOffset:
            Serial.flush();
            break;
        default:
            break;
    }
}
Ordinal
HitagiSBCore::doRAMLoad(Address address, TreatAsOrdinal) {
    Ordinal value = 0;
    (void)psramBlockRead(address, reinterpret_cast<byte*>(value), sizeof(value));
    return value;
}
void
HitagiSBCore::doRAMStore(Address address, ByteOrdinal value) {
    psramBlockWrite(address, reinterpret_cast<byte*>(value), sizeof(value));
}
void
HitagiSBCore::doRAMStore(Address address, ShortOrdinal value) {
    psramBlockWrite(address, reinterpret_cast<byte*>(value), sizeof(value));
}
void
HitagiSBCore::doRAMStore(Address address, Ordinal value) {
    psramBlockWrite(address, reinterpret_cast<byte*>(value), sizeof(value));
}
bool
HitagiSBCore::inRAMArea(Address target) noexcept{
    // since the ram starts at address zero, there is no need to worry about shifting the offset
    return target >= RamStart && target < RamSize;
}
Core::Address
HitagiSBCore::toRAMOffset(Address target) noexcept{
    return target & RamMask;
}
HitagiSBCore::~HitagiSBCore() noexcept {}
HitagiSBCore::HitagiSBCore() : Parent() {

}
void
HitagiSBCore::CacheLine::clear() noexcept {
    valid_ = false;
    valid_ = false;
    address_ = 0;
    for (int i = 0; i < 32; ++i) {
        storage_[i] = 0;
    }
}
#endif
