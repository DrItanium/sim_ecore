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

#ifdef ARDUINO
#include <Arduino.h>

#include <SPI.h>
#include "Types.h"
#include "HitagiSBCore.h"
#include <SdFat.h>
#ifdef ARDUINO_GRAND_CENTRAL_M4
SdFat SD(&SDCARD_SPI);
#else
SdFat SD;
#endif

#ifdef ARDUINO_AVR_ATmega1284
namespace {
    union Address26 {
        constexpr explicit Address26(Core::Address value = 0) : base(value) { }
        constexpr auto getAddress() const noexcept { return base; }
        constexpr auto getOffset() const noexcept { return offset; }
        constexpr auto getIndex() const noexcept { return index; }
        Core::Address base : 26; // 1 megabyte always
        struct {
            Core::Address offset : 23;
            Core::Address index : 3;
        };
    };
#ifndef PIN_SERIAL_RX
#define PIN_SERIAL_RX 8
#endif // end !defined(PIN_SERIAL_RX)
#ifndef PIN_SERIAL_TX
#define PIN_SERIAL_TX 9
#endif // end !defined(PIN_SERIAL_TX)
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
        CACHE_EN_ = PORT_D3,
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
    using i960Pinout = HitagiChipsetPinout;
    template<i960Pinout pin>
    constexpr auto isValidPin960_v = static_cast<uint8_t>(pin) < static_cast<uint8_t>(i960Pinout::Count);
    //static_assert(isValidPin<i960Pinout::CACHE_A0>, "The CACHE_A0 pin should be a valid pin!");
    template<i960Pinout pin>
    [[nodiscard]] inline volatile unsigned char& getAssociatedOutputPort() noexcept {
        static_assert(isValidPin960_v<pin>, "INVALID PIN PROVIDED");
        switch (pin) {
#define X(id, number) case i960Pinout :: PORT_ ## id ## number
#define Y(id) \
X(id, 0): \
X(id, 1): \
X(id, 2): \
X(id, 3): \
X(id, 4): \
X(id, 5): \
X(id, 6): \
X(id, 7)
            Y(A): return PORTA;
            Y(C): return PORTC;
            Y(D): return PORTD;
            Y(B): return PORTB;
#undef Y
#undef X

            default:
                return PORTA;
        }
    }

    template<i960Pinout pin>
    [[nodiscard]] inline volatile unsigned char& getAssociatedInputPort() noexcept {
        static_assert(isValidPin960_v<pin>, "INVALID PIN PROVIDED");
        switch (pin) {
#define X(id, number) case i960Pinout :: PORT_ ## id ## number
#define Y(id) \
X(id, 0): \
X(id, 1): \
X(id, 2): \
X(id, 3): \
X(id, 4): \
X(id, 5): \
X(id, 6): \
X(id, 7)
            Y(A): return PINA;
            Y(C): return PINC;
            Y(D): return PIND;
            Y(B): return PINB;
#undef Y
#undef X
            default:
                return PINA;
        }
    }
    template<i960Pinout pin>
    [[nodiscard]] constexpr decltype(auto) getPinMask() noexcept {
        static_assert(isValidPin960_v<pin>, "INVALID PIN PROVIDED");
        switch (pin) {
#define X(id, number) case i960Pinout :: PORT_ ## id ## number : return _BV ( P ## id ## number )
#define Y(id) \
X(id, 0); \
X(id, 1); \
X(id, 2); \
X(id, 3); \
X(id, 4); \
X(id, 5); \
X(id, 6); \
X(id, 7)
            Y(A);
            Y(C);
            Y(D);
            Y(B);
#undef Y
#undef X
            default:
                return 0xFF;
        }
    }

    template<i960Pinout pin, decltype(HIGH) value>
    inline void digitalWrite() {
        uint8_t theSREG = SREG;
        cli();
        auto& thePort = getAssociatedOutputPort<pin>();
        if constexpr (value == LOW) {
            thePort &= ~getPinMask<pin>();
        } else {
            thePort |= getPinMask<pin>();
        }
        SREG = theSREG;
    }
    template<i960Pinout pin>
    inline void digitalWrite(decltype(HIGH) value) noexcept {
        uint8_t theSREG = SREG;
        cli();
        auto& thePort = getAssociatedOutputPort<pin>();
        if (value == LOW) {
            thePort &= ~getPinMask<pin>();
        } else {
            thePort |= getPinMask<pin>();
        }
        SREG = theSREG;
    }

    template<i960Pinout pin>
    inline auto digitalRead() noexcept {
        return (getAssociatedInputPort<pin>() & getPinMask<pin>()) ? HIGH : LOW;
    }
    inline void pinMode(HitagiChipsetPinout pin, decltype(OUTPUT) direction) noexcept {
        ::pinMode(static_cast<int>(pin), direction);
    }
    inline void digitalWrite(HitagiChipsetPinout pin, decltype(HIGH) value) noexcept {
        ::digitalWrite(static_cast<int>(pin), value);
    }
    inline auto digitalRead(HitagiChipsetPinout pin) noexcept {
        return ::digitalRead(static_cast<int>(pin));
    }
    SPISettings psramSettings(8_MHz, MSBFIRST, SPI_MODE0);
}
#endif
void
HitagiSBCore::begin() {
    Serial.println(F("BRINGING UP HITAGI SBCORE EMULATOR!"));
#ifdef ARDUINO_GRAND_CENTRAL_M4
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
    }
#elif defined(ARDUINO_AVR_ATmega1284)
    // hold the i960 in reset for the rest of execution, we really don't care about anything else with respect to this processor now
    pinMode(HitagiChipsetPinout::RESET960_, OUTPUT);
    digitalWrite(HitagiChipsetPinout::RESET960_, LOW);
    pinMode(HitagiChipsetPinout::CACHE_EN_, OUTPUT);
    digitalWrite(HitagiChipsetPinout::CACHE_EN_, HIGH);
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
    Serial.println(F("SDCARD UP!"));
    if (auto theFile = SD.open("boot.sys", FILE_READ); !theFile) {
        Serial.println(F("Could not open \"boot.sys\"! SD CARD may be corrupt?"));
        while (true) { delay(1000); }
    } else {
        Serial.println(F("Found boot image!"));
        setupPSRAMChips();
        Address size = theFile.size();
        Serial.println(F("COPYING \"boot.sys\" to PSRAM"));
        static constexpr auto PSRAMCopyBufferSize = 1_KB;
        byte psramCopyBuffer[PSRAMCopyBufferSize] = { 0 };
        for (Address addr = 0; addr < size; addr += PSRAMCopyBufferSize) {
            auto numRead = theFile.read(psramCopyBuffer, PSRAMCopyBufferSize) ;
            // introduce some delay in between options to make sure that the SD Card has time to
            // recover.
            if (numRead < 0) {
                SD.errorHalt();
            }
            (void) psramBlockWrite(addr, psramCopyBuffer, numRead);
            // wait for the block device to be ready again
            while (theFile.isBusy());
        }
        Serial.println(F("TRANSFER COMPLETE!\n"));
        theFile.close();
    }
    /// @todo implement support for other features as well
    Serial.println(F("INVALIDATING CACHE AFTER BEING USED FOR IMAGE INSTALLATION!"));
#else
#endif
}
#ifdef ARDUINO_AVR_ATmega1284
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
    Decomposition temp(id);
    if (temp.getIndex() != chipId_.getIndex()) {
        digitalWrite<HitagiChipsetPinout::SPI_OFFSET0>(temp.s0 ? HIGH : LOW);
        digitalWrite<HitagiChipsetPinout::SPI_OFFSET1>(temp.s1 ? HIGH : LOW);
        digitalWrite<HitagiChipsetPinout::SPI_OFFSET2>(temp.s2 ? HIGH : LOW);
        chipId_ = temp;
    }
}
template<HitagiChipsetPinout pin = HitagiChipsetPinout::PSRAM_EN_>
void
singleOperation(byte opcode, Core::Address address, byte* buf, size_t count) {
    SPI.beginTransaction(SPISettings(5'000'000, MSBFIRST, SPI_MODE0));
    MemoryCell32 translated(address);
    digitalWrite<pin, LOW>();
    SPI.transfer(opcode);
    SPI.transfer(translated.getByteOrdinal(2));
    SPI.transfer(translated.getByteOrdinal(1));
    SPI.transfer(translated.getByteOrdinal(0));
    SPI.transfer(buf, count);
    digitalWrite<pin, HIGH>();
    SPI.endTransaction();
}
size_t
HitagiSBCore::psramBlockWrite(Address address, byte *buf, size_t count) {
    Address26 curr(address);
    Address26 end(address + count);
    if (curr.getIndex() == end.getIndex()) {
        // okay they are part of the same chip so we can safely just do a single operation
        setPSRAMId(curr.getIndex());
        singleOperation(0x02, address, buf, count);
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
        singleOperation(0x02, address, buf, numBytesToFirstChip);
        setPSRAMId(end.getIndex());
        // jump ahead to the left overs and retrieve them
        // start at the beginning of the new chip with the slop
        singleOperation(0x02, 0, buf + numBytesToFirstChip, numBytesToSecondChip);
    }
    return count;
}
size_t
HitagiSBCore::psramBlockRead(Address address, byte *buf, size_t count) {
    Address26 curr(address);
    Address26 end(address + count);
    if (curr.getIndex() == end.getIndex()) {
        // okay they are part of the same chip so we can safely just do a single operation
        setPSRAMId(curr.getIndex());
        singleOperation(0x03, address, buf, count);
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
        singleOperation(0x03, address, buf, numBytesToFirstChip);
        setPSRAMId(end.getIndex());
        // jump ahead to the left overs and retrieve them
        // start at the beginning of the new chip with the slop
        singleOperation(0x03, 0, buf + numBytesToFirstChip, numBytesToSecondChip);
    }
    return count;
}
#endif



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
#ifdef ARDUINO_AVR_ATmega1284
    Ordinal value = 0;
    (void)psramBlockRead(address, reinterpret_cast<byte*>(value), sizeof(value));
    return value;
#elif defined(ARDUINO_GRAND_CENTRAL_M4)
    return 0;
#endif
}
void
HitagiSBCore::doRAMStore(Address address, ByteOrdinal value) {
#ifdef ARDUINO_AVR_ATmega1284
    psramBlockWrite(address, reinterpret_cast<byte*>(value), sizeof(value));
#elif defined(ARDUINO_GRAND_CENTRAL_M4)
#endif
}
void
HitagiSBCore::doRAMStore(Address address, ShortOrdinal value) {
#ifdef ARDUINO_AVR_ATmega1284
    psramBlockWrite(address, reinterpret_cast<byte*>(value), sizeof(value));
#elif defined(ARDUINO_GRAND_CENTRAL_M4)
#endif
}
void
HitagiSBCore::doRAMStore(Address address, Ordinal value) {
#ifdef ARDUINO_AVR_ATmega1284
    psramBlockWrite(address, reinterpret_cast<byte*>(value), sizeof(value));
#elif defined(ARDUINO_GRAND_CENTRAL_M4)
#endif
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
#ifdef ARDUINO_AVR_ATmega1284
HitagiSBCore::HitagiSBCore() : Parent(), chipId_(0xff) {

}
#elif defined(ARDUINO_GRAND_CENTRAL_M4)
HitagiSBCore::HitagiSBCore() : Parent() {}
#else
#endif
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
