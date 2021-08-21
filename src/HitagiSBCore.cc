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
#include <SD.h>
#include <SPI.h>
#include "Types.h"
#include "HitagiSBCore.h"

#ifdef ARDUINO_AVR_ATmega1284
#ifndef PIN_SERIAL_RX
#define PIN_SERIAL_RX 8
#endif // end !defined(PIN_SERIAL_RX)
#ifndef PIN_SERIAL_TX
#define PIN_SERIAL_TX 9
#endif // end !defined(PIN_SERIAL_TX)
#endif // end defined(ARDUINO_AVR_ATmega1284)
enum class HitagiChipsetPinout : int {
    // this is described in digial pin order!
    // leave this one alone
    PORT_B0 = 0,
    PORT_B1,
    PORT_B2,
    PORT_B3,
    PORT_B4,
    PORT_B5,
    PORT_B6,
    PORT_B7,
    PORT_D0,
    PORT_D1,
    PORT_D2,
    PORT_D3,
    PORT_D4,
    PORT_D5,
    PORT_D6,
    PORT_D7,
    PORT_C0,
    PORT_C1,
    PORT_C2,
    PORT_C3,
    PORT_C4,
    PORT_C5,
    PORT_C6,
    PORT_C7,
    PORT_A0,
    PORT_A1,
    PORT_A2,
    PORT_A3,
    PORT_A4,
    PORT_A5,
    PORT_A6,
    PORT_A7,
    Count,
    RX0 = PIN_SERIAL_RX,
    TX0 = PIN_SERIAL_TX,
    CS = PIN_SPI_SS,
    SCK = PIN_SPI_SCK,
    MOSI = PIN_SPI_MOSI,
    MISO = PIN_SPI_MISO,
    SCL = PIN_WIRE_SCL,
    SDA = PIN_WIRE_SDA,
    CLKO = PORT_B1,
    READY_ = PORT_B0,
    AS_ = PORT_B2,
    PSRAM_EN_ = PORT_B3,
    DEN_ = PORT_D2,
    RESET960_ = PORT_D5,
    Int0_ = PORT_D6,
    SPI_OFFSET0 = PORT_C2,
    SPI_OFFSET1 = PORT_C3,
    SPI_OFFSET2 = PORT_C4,
    DISPLAY_EN_ = PORT_C5,
    DC = PORT_C6,
    SD_EN_ = PORT_C7,
    W_R_ = PORT_A0,
    BA1 = PORT_A1,
    BA2 = PORT_A2,
    BA3 = PORT_A3,
    BE0_ = PORT_A4,
    BE1_ = PORT_A5,
    BLAST_ = PORT_A6,
    FAIL960 = PORT_A7,
};
namespace {
    inline void pinMode(HitagiChipsetPinout pin, decltype(OUTPUT) direction) noexcept {
        ::pinMode(static_cast<int>(pin), direction);
    }
    inline void digitalWrite(HitagiChipsetPinout pin, decltype(HIGH) value) noexcept {
        ::digitalWrite(static_cast<int>(pin), value);
    }
    inline auto digitalRead(HitagiChipsetPinout pin) noexcept {
        return ::digitalRead(static_cast<int>(pin));
    }

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
    while (!SD.begin(static_cast<int>(HitagiChipsetPinout::SD_EN_))) {
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
        static constexpr auto CacheSize = 512;
        byte storage[CacheSize] = { 0 };
        Serial.println(F("COPYING \"boot.sys\" to PSRAM"));
        for (Address addr = 0; addr < size; addr += CacheSize) {
            auto numRead = theFile.read(storage, CacheSize) ;
            (void) psramBlockWrite(addr, storage, numRead);
        }
        Serial.println(F("TRANSFER COMPLETE!!!"));
        theFile.close();
    }
    /// @todo implement support for other features as well




}

ByteOrdinal
HitagiSBCore::ioSpaceLoad(Address address, TreatAsByteOrdinal ordinal) {
    return 0;
}
ShortOrdinal
HitagiSBCore::ioSpaceLoad(Address address, TreatAsShortOrdinal ordinal) {
    return 0;
}
Ordinal
HitagiSBCore::ioSpaceLoad(Address address, TreatAsOrdinal ordinal) {
    return 0;
}
void
HitagiSBCore::ioSpaceStore(Address address, ByteOrdinal value) {

}
void
HitagiSBCore::ioSpaceStore(Address address, ShortOrdinal value) {

}
void
HitagiSBCore::ioSpaceStore(Address address, Ordinal value) {

}
ByteOrdinal
HitagiSBCore::doIACLoad(Address address, TreatAsByteOrdinal ordinal) {
    return 0;
}
ShortOrdinal
HitagiSBCore::doIACLoad(Address address, TreatAsShortOrdinal ordinal) {
    return 0;
}
Ordinal
HitagiSBCore::doIACLoad(Address address, TreatAsOrdinal ordinal) {
    return 0;
}
void
HitagiSBCore::doIACStore(Address address, ByteOrdinal value) {

}
void
HitagiSBCore::doIACStore(Address address, ShortOrdinal value) {

}
void
HitagiSBCore::doIACStore(Address address, Ordinal value) {

}
Ordinal
HitagiSBCore::doRAMLoad(Address address, TreatAsOrdinal ordinal) {
    return 0;
}
void
HitagiSBCore::doRAMStore(Address address, ByteOrdinal value) {

}
void
HitagiSBCore::doRAMStore(Address address, ShortOrdinal value) {

}
void
HitagiSBCore::doRAMStore(Address address, Ordinal value) {

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
#endif
