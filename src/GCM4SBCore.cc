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
#if 0
        memoryImage_.begin();
        Serial.println(F("SUCCESSFULLY OPENED \"live.bin\""));
        // now we copy from the pristine image over to the new one in blocks
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
            (void)memoryImage_.write(i, transferCache, numRead);
            // wait until we are ready to
            while (SD.card()->isBusy());
            Serial.print(F("."));
        }
        Serial.println(F("CONSTRUCTION COMPLETE!!!"));
        // make a new copy of this file
        theFile.close();
        while (SD.card()->isBusy());
#else
        pinMode(53, OUTPUT);
        digitalWrite(53, HIGH);
        delayMicroseconds(200);
        digitalWrite(53, LOW);
        SPI.transfer(0x66);
        digitalWrite(53, HIGH);
        digitalWrite(53, LOW);
        SPI.transfer(0x99);
        digitalWrite(53, HIGH);
        Address size = theFile.size();
        constexpr auto CacheSize = TransferCacheSize;
        Serial.println(F("CONSTRUCTING NEW MEMORY IMAGE IN \"live.bin\""));
        for (Address i = 0; i < size; i += CacheSize) {
            auto numRead = theFile.read(transferCache, CacheSize);
            if (numRead < 0) {
                SD.errorHalt();
            }
            while (SD.card()->isBusy());
            digitalWrite(53, LOW);
            SPI.transfer(0x02);
            SPI.transfer()
            // wait until we are ready to
            Serial.print(F("."));
        }
        Serial.println(F("CONSTRUCTION COMPLETE!!!"));
        // make a new copy of this file
        theFile.close();
        while (SD.card()->isBusy());
#endif
        // okay also clear out the cache lines since the transfer buffer is shared with the cache
        for (auto& line : lines_) {
            line.clear();
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
GCM4SBCore::doRAMLoad(Address address, TreatAsOrdinal thingy) {
    return getCacheLine(address).get(address, thingy);
}
ShortOrdinal
GCM4SBCore::doRAMLoad(Address address, TreatAsShortOrdinal thingy) {
    return getCacheLine(address).get(address, thingy);
}
ByteOrdinal
GCM4SBCore::doRAMLoad(Address address, TreatAsByteOrdinal thingy) {
    return getCacheLine(address).get(address, thingy);
}
void
GCM4SBCore::doRAMStore(Address address, ByteOrdinal value) {
    getCacheLine(address).set(address, value);
}
void
GCM4SBCore::doRAMStore(Address address, ShortOrdinal value) {
    getCacheLine(address).set(address, value);
}
void
GCM4SBCore::doRAMStore(Address address, Ordinal value) {
    getCacheLine(address).set(address, value);
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
GCM4SBCore::GCM4SBCore() : Parent(), memoryImage_(0,64_MB, 64_MB,"live.bin", FILE_WRITE) {}

GCM4SBCore::CacheLine&
GCM4SBCore::getCacheLine(Address target) noexcept {
    CacheAddress addr(target);
    // okay we need to find out which cache line this current address targets
    auto& targetLine = lines_[addr.getCacheIndex()];
    if (!targetLine.matches(target)) {
        // right now we only have one thing "mapped" to the file cache
        targetLine.reset(target, memoryImage_);
    }
    return targetLine;
}

void
GCM4SBCore::CacheLine::set(Address targetAddress, Ordinal value) {
    // assume aligned
    CacheAddress addr(targetAddress);
    dirty_ = true;
    storage_[addr.getCellIndex()].setOrdinalValue(value);
}

void
GCM4SBCore::CacheLine::set(Address targetAddress, ShortOrdinal value) {
    // assume aligned
    CacheAddress addr(targetAddress);
    dirty_ = true;
    // just shift by one since we can safely assume it is aligned as that is the only way
    // to call this method through the memory system gestalt
    storage_[addr.getCellIndex()].setShortOrdinal(value, addr.getCellOffset(TreatAsShortOrdinal{}));
}

void
GCM4SBCore::CacheLine::set(Address targetAddress, ByteOrdinal value) {
    CacheAddress addr(targetAddress);
    dirty_ = true;
    storage_[addr.getCellIndex()].setByteOrdinal(value, addr.getCellOffset(TreatAsByteOrdinal{}));
}

Ordinal
GCM4SBCore::CacheLine::get(Address targetAddress, TreatAsOrdinal) const noexcept {
    // assume aligned
    CacheAddress addr(targetAddress);
    return storage_[addr.getCellIndex()].getOrdinalValue();
}

ShortOrdinal
GCM4SBCore::CacheLine::get(Address targetAddress, TreatAsShortOrdinal thingy) const noexcept {
    CacheAddress addr(targetAddress);
    return storage_[addr.getCellIndex()].getShortOrdinal(addr.getCellOffset(thingy));
}

ByteOrdinal
GCM4SBCore::CacheLine::get(Address targetAddress, TreatAsByteOrdinal thingy) const noexcept {
    CacheAddress addr(targetAddress);
    return storage_[addr.getCellIndex()].getByteOrdinal(addr.getCellOffset(thingy));
}

void
GCM4SBCore::CacheLine::clear() noexcept {
    // calling clear means that you just want to do a hard clear without any saving of what is currently in the cache
    // We must do this after using part of the cacheline storage as a transfer buffer for setting up the live memory image
    // there will be garbage in memory here that can be interpreted as "legal" cache lines
    dirty_ = false;
    valid_ = false;
    backingStorage_ = nullptr;
    address_ = 0;
    for (auto& cell : storage_) {
        cell.setOrdinalValue(0);
    }
}
void
GCM4SBCore::CacheLine::reset(Address newAddress, MemoryThing &newThing) {
    // okay this is the most complex part of the implementation
    // we need to do the following:
    // 1. If the cache line is dirty (and also valid), then commit it back to its backing storage
    // 2. compute the new base address using the new address
    // 3. use the new backing storage to load a cache line's worth of data into storage from teh new backing storage
    // 4. mark the line as clean and valid
    if (valid() && dirty()) {
        // okay so we have a valid dirty cache line, lets save it back to the underlying storage
        (void)backingStorage_->write(address_, reinterpret_cast<byte*>(storage_), sizeof(storage_));
        /// @todo check and see if we were able to write everything back to the underlying storage
        // at this point we've written back to the old backing storage
    }
    CacheAddress newAddr(newAddress);
    valid_ = true;
    dirty_ = false;
    address_ = newAddr.getTagAddress();
    backingStorage_ = &newThing;
    (void)backingStorage_->read(address_, reinterpret_cast<byte*>(storage_), sizeof(storage_));
    /// @todo check and see if we were able to read a full cache line from underlying storage
}



#endif
