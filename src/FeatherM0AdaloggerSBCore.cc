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

#ifdef ARDUINO_FEATHER_M0_ADALOGGER
#include <Arduino.h>

#include <SPI.h>
#include "Types.h"
#include "FeatherM0AdaloggerSBCore.h"
#include <SdFat.h>
SdFat SD;

void
FeatherM0AdaloggerSBCore::begin() {
    pixels.begin();
    pixels.setBrightness(10);
    pixels.setPixelColor(0, pixels.Color(255, 0, 255));
    pixels.show();
    delay(1000);
    pixels.setPixelColor(0, pixels.Color(0,0,0));
    pixels.show();
    Serial.println(F("BRINGING UP HITAGI SBCORE EMULATOR!"));
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    // setup the tft display
    tft.begin();
    tft.setRotation(1);
    tft.fillScreen(tft.color565(0,0,0));
    Serial.print("DISPLAY RESOLUTION: ");
    Serial.print(tft.width());
    Serial.print(" x ");
    Serial.println(tft.height());
    tft.println("BOOTING HITAGI SBCORE EMULATOR!");
    ts.begin();
    SPI.begin();
    while (!SD.begin(SDCardEnablePin)) {
        tft.println(F("NO SDCARD...WILL TRY AGAIN!"));
        delay(1000);
    }
    tft.println(F("SDCARD UP!"));
    if (auto theFile = SD.open("boot.sys", FILE_READ); !theFile) {
        tft.println(F("Could not open \"boot.sys\"! SD CARD may be corrupt?"));
        while (true) { delay(1000); }
    } else {
        memoryImage_.begin();
#ifndef USE_PSRAM_CHIP
        tft.println(F("SUCCESSFULLY OPENED \"live.bin\""));
#endif
        // now we copy from the pristine image over to the new one in blocks
        Address size = theFile.size();
        constexpr auto CacheSize = Cache::CacheSize;
#ifndef USE_PSRAM_CHIP
        tft.println(F("CONSTRUCTING NEW MEMORY IMAGE IN \"live.bin\""));
#else
        tft.println(F("TRANSFERRING IMAGE TO PSRAM!"));
#endif
        auto* transferCache = theCache_.asTransferCache();
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
            tft.print(F("."));
        }
        tft.println();
        tft.println(F("CONSTRUCTION COMPLETE!!!"));
        // make a new copy of this file
        theFile.close();
        while (SD.card()->isBusy());
        // okay also clear out the cache lines since the transfer buffer is shared with the cache
        theCache_.clear();
    }
    digitalWrite(LED_BUILTIN, LOW);
}



ByteOrdinal
FeatherM0AdaloggerSBCore::ioSpaceLoad(Address address, TreatAsByteOrdinal) {
    return 0;
}
void
FeatherM0AdaloggerSBCore::ioSpaceStore(Address address, ByteOrdinal value) {
    // nothing to do here right now
}
Ordinal
FeatherM0AdaloggerSBCore::ioSpaceLoad(Address address, TreatAsOrdinal ) {
    // right now there is nothing to do here
    return 0;
}
void
FeatherM0AdaloggerSBCore::pushCharacterOut(char value) {
    // push a character out to the screen
    if (tft.getCursorY() >= tft.height()) {
        tft.fillScreen(tft.color565(0,0,0));
        tft.setCursor(0, 0);
    }
    if (value == '\r') {
        tft.println();
    } else if (value == '\b') {
        // backspace
        if (tft.getCursorX() > 0) {
            // step back, clear the space and then step back again
            tft.setCursor(tft.getCursorX() - 1, tft.getCursorY());
            tft.print(' ');
            tft.setCursor(tft.getCursorX() - 1, tft.getCursorY());
        }
    } else {
        tft.write(value);
    }
}
ShortOrdinal
FeatherM0AdaloggerSBCore::ioSpaceLoad(Address address, TreatAsShortOrdinal) {
    switch (address) {
        case 0:
            return [this]() {
                auto result = Serial.read();
                if (result != -1) {
                    pushCharacterOut(static_cast<char>(result));
                }
                return result;
            }();
        case 2:
            Serial.flush();
            break;
        default:
            break;
    }
    return 0;
}

void
FeatherM0AdaloggerSBCore::ioSpaceStore(Address address, ShortOrdinal value) {
    switch (address) {
        case 0:
            pushCharacterOut(static_cast<char>(value));
            Serial.write(static_cast<char>(value));
            break;
        case 2:
            Serial.flush();
            break;
        default:
            break;
    }
}
void
FeatherM0AdaloggerSBCore::ioSpaceStore(Address address, Ordinal value) {
    // nothing to do right now
}
ByteOrdinal
FeatherM0AdaloggerSBCore::doIACLoad(Address address, TreatAsByteOrdinal ordinal) {
    return 0;
}
ShortOrdinal
FeatherM0AdaloggerSBCore::doIACLoad(Address address, TreatAsShortOrdinal ordinal) {
    return 0;
}
void
FeatherM0AdaloggerSBCore::doIACStore(Address address, ByteOrdinal value) {
    // do nothing
}
void
FeatherM0AdaloggerSBCore::doIACStore(Address address, ShortOrdinal value) {
    // do nothing
}
Ordinal
FeatherM0AdaloggerSBCore::doIACLoad(Address address, TreatAsOrdinal) {
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
FeatherM0AdaloggerSBCore::doIACStore(Address address, Ordinal value) {
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
FeatherM0AdaloggerSBCore::doRAMLoad(Address address, TreatAsOrdinal thingy) {
    return getCacheLine(address, memoryImage_).get(address, thingy);
}
ShortOrdinal
FeatherM0AdaloggerSBCore::doRAMLoad(Address address, TreatAsShortOrdinal thingy) {
    return getCacheLine(address, memoryImage_).get(address, thingy);
}
ByteOrdinal
FeatherM0AdaloggerSBCore::doRAMLoad(Address address, TreatAsByteOrdinal thingy) {
    return getCacheLine(address, memoryImage_).get(address, thingy);
}
void
FeatherM0AdaloggerSBCore::doRAMStore(Address address, ByteOrdinal value) {
    getCacheLine(address, memoryImage_).set(address, value);
}
void
FeatherM0AdaloggerSBCore::doRAMStore(Address address, ShortOrdinal value) {
    getCacheLine(address, memoryImage_).set(address, value);
}
void
FeatherM0AdaloggerSBCore::doRAMStore(Address address, Ordinal value) {
    getCacheLine(address, memoryImage_).set(address, value);
}
bool
FeatherM0AdaloggerSBCore::inRAMArea(Address target) noexcept{
    // since the ram starts at address zero, there is no need to worry about shifting the offset
    return target >= RamStart && target < RamSize;
}
Address
FeatherM0AdaloggerSBCore::toRAMOffset(Address target) noexcept{
    return target & RamMask;
}

FeatherM0AdaloggerSBCore::FeatherM0AdaloggerSBCore() : Parent(), tft(LCDCSPin, LCDDCPin),
pixels(1, NeopixelPin, NEO_GRB + NEO_KHZ800),
memoryImage_(
#ifndef USE_PSRAM_CHIP
        0,64_MB, 64_MB,"live.bin", FILE_WRITE
#else
        0
#endif
)
{}

// must be last line in file
#endif // end defined ARDUINO_FEATHER_M0_ADALOGGER