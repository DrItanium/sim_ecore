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

#ifdef ARDUINO_GRAND_CENTRAL_M4
#include <Arduino.h>

#include <SPI.h>
#include "Types.h"
#include "GCM4SBCore.h"
#include <SdFat.h>
SdFat SD(&SDCARD_SPI);

void
GCM4SBCore::begin() {
    Serial.println(F("BRINGING UP HITAGI SBCORE EMULATOR!"));
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    SPI.begin();
    while (!SD.begin(SDCARD_SS_PIN)) {
        Serial.println(F("NO SDCARD...WILL TRY AGAIN!"));
        delay(1000);
    }
    Serial.println(F("SDCARD UP!"));
    if (auto theFile = SD.open("boot.sys", FILE_READ); !theFile) {
        Serial.println(F("Could not open \"boot.sys\"! SD CARD may be corrupt?"));
        while (true) { delay(1000); }
    } else {

        memoryImage_ = SD.open("live.bin", FILE_WRITE);
        if (!memoryImage_) {
            Serial.println(F("UNABLE TO OPEN \"live.bin\" FOR RW! HALTING!"));
            while(true) { delay(1000); }
        }
        Serial.println(F("SUCCESSFULLY OPENED \"live.bin\""));
        // now we copy from the pristine image over to the new one in blocks
        memoryImage_.seekSet(0); // jump to address zero
        Address size = theFile.size();
        constexpr auto CacheSize = TransferCacheSize;
        Serial.println(F("CONSTRUCTING NEW MEMORY IMAGE IN \"live.bin\""));
        for (Address i = 0; i < size; i += CacheSize) {
            auto numRead = theFile.read(transferCache, CacheSize);
            if (numRead < 0) {
                SD.errorHalt();
            }
            while (SD.card()->isBusy());
            // wait until the sd card is ready again to transfer
            (void)memoryImage_.write(transferCache, numRead);
            // wait until we are ready to
            while (SD.card()->isBusy());
            Serial.print(F("."));
        }
        Serial.println(F("CONSTRUCTION COMPLETE!!!"));
        memoryImage_.flush();
        // make a new copy of this file
        theFile.close();
        memoryImage_.close();
        while (SD.card()->isBusy());
        memoryImage_ = SD.open("live.bin", FILE_WRITE);
        if (!memoryImage_) {
            Serial.println(F("UNABLE TO OPEN \"live.bin\" FOR RW! HALTING!"));
            while(true) { delay(1000); }
        }

    }
}



ByteOrdinal
GCM4SBCore::ioSpaceLoad(Address address, TreatAsByteOrdinal) {
    return 0;
}
void
GCM4SBCore::ioSpaceStore(Address address, ByteOrdinal value) {
    // nothing to do here right now
}
Ordinal
GCM4SBCore::ioSpaceLoad(Address address, TreatAsOrdinal ) {
    // right now there is nothing to do here
    return 0;
}
ShortOrdinal
GCM4SBCore::ioSpaceLoad(Address address, TreatAsShortOrdinal) {
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
GCM4SBCore::ioSpaceStore(Address address, ShortOrdinal value) {
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
GCM4SBCore::ioSpaceStore(Address address, Ordinal value) {
    // nothing to do right now
}
ByteOrdinal
GCM4SBCore::doIACLoad(Address address, TreatAsByteOrdinal ordinal) {
    return 0;
}
ShortOrdinal
GCM4SBCore::doIACLoad(Address address, TreatAsShortOrdinal ordinal) {
    return 0;
}
void
GCM4SBCore::doIACStore(Address address, ByteOrdinal value) {
    // do nothing
}
void
GCM4SBCore::doIACStore(Address address, ShortOrdinal value) {
    // do nothing
}
Ordinal
GCM4SBCore::doIACLoad(Address address, TreatAsOrdinal) {
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
GCM4SBCore::doIACStore(Address address, Ordinal value) {
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
GCM4SBCore::doRAMLoad(Address address, TreatAsOrdinal) {
    Ordinal value = 0;
    memoryImage_.seekSet(address);
    memoryImage_.read(reinterpret_cast<byte*>(value), sizeof(Ordinal));
    return value;
}
void
GCM4SBCore::doRAMStore(Address address, ByteOrdinal value) {
    memoryImage_.seek(address);
    memoryImage_.write(value);
}
void
GCM4SBCore::doRAMStore(Address address, ShortOrdinal value) {
    memoryImage_.seek(address);
    memoryImage_.write(reinterpret_cast<byte*>(value), sizeof(value));
}
void
GCM4SBCore::doRAMStore(Address address, Ordinal value) {
    memoryImage_.seek(address);
    memoryImage_.write(reinterpret_cast<byte*>(value), sizeof(value));
}
bool
GCM4SBCore::inRAMArea(Address target) noexcept{
    // since the ram starts at address zero, there is no need to worry about shifting the offset
    return target >= RamStart && target < RamSize;
}
Address
GCM4SBCore::toRAMOffset(Address target) noexcept{
    return target & RamMask;
}
GCM4SBCore::~GCM4SBCore() noexcept {}
GCM4SBCore::GCM4SBCore() : Parent() {}
#if 0
void
GCM4SBCore::CacheLine::clear() noexcept {
    valid_ = false;
    valid_ = false;
    address_ = 0;
    for (int i = 0; i < 32; ++i) {
        storage_[i] = 0;
    }
}
#endif
#endif
