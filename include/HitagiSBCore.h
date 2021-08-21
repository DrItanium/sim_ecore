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

#ifndef SIM3_HITAGISBCORE_H
#define SIM3_HITAGISBCORE_H
#ifdef ARDUINO_AVR_ATmega1284
#include <Arduino.h>
#include <SPI.h>
#include "Types.h"
#include "SBCoreArduino.h"

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

template<i960Pinout pin>
inline void pulse(decltype(HIGH) from = HIGH, decltype(LOW) to = LOW) noexcept {
    // save registers and do the pulse
    uint8_t theSREG = SREG;
    cli();
    auto& thePort = getAssociatedOutputPort<pin>();
    thePort ^= getPinMask<pin>();
    thePort ^= getPinMask<pin>();
    SREG = theSREG;
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
/**
 * @brief A version of the SBCore for an ATMega1284p originally designed to act as the chipset for an actual i960Sx processor;
 * This is meant to be a drop in replacement for that hardware so it will hold the i960Sx in reset and not use the CLKO pin either
 */
class HitagiSBCore : public SBCoreArduino {
public:
    static constexpr Address RamSize = 64_MB;
    static constexpr Address RamStart = 0x0000'0000;
    static constexpr Address RamMask = RamSize - 1;
    struct CacheLine {
        constexpr CacheLine() noexcept : address_(0), dirty_(false), valid_(false), storage_{ 0 } { }
        Address address_ = 0;
        bool dirty_ = false;
        bool valid_ = false;
        byte storage_[32] = { 0 };
        void clear() noexcept;
    };
public:
    using Parent = SBCoreArduino;
    HitagiSBCore();
    ~HitagiSBCore() override;
    void begin() override;
protected:
    ByteOrdinal ioSpaceLoad(Address address, TreatAsByteOrdinal ordinal) override;
    ShortOrdinal ioSpaceLoad(Address address, TreatAsShortOrdinal ordinal) override;
    Ordinal ioSpaceLoad(Address address, TreatAsOrdinal ordinal) override;
    void ioSpaceStore(Address address, ByteOrdinal value) override;
    void ioSpaceStore(Address address, ShortOrdinal value) override;
    void ioSpaceStore(Address address, Ordinal value) override;
    ByteOrdinal doIACLoad(Address address, TreatAsByteOrdinal ordinal) override;
    ShortOrdinal doIACLoad(Address address, TreatAsShortOrdinal ordinal) override;
    Ordinal doIACLoad(Address address, TreatAsOrdinal ordinal) override;
    void doIACStore(Address address, ByteOrdinal value) override;
    void doIACStore(Address address, ShortOrdinal value) override;
    void doIACStore(Address address, Ordinal value) override;
    Ordinal doRAMLoad(Address address, TreatAsOrdinal ordinal) override;
    void doRAMStore(Address address, ByteOrdinal value) override;
    void doRAMStore(Address address, ShortOrdinal value) override;
    void doRAMStore(Address address, Ordinal value) override;
    bool inRAMArea(Address target) noexcept override;
    Address toRAMOffset(Address target) noexcept override;
private:
    void setPSRAMId(byte id) noexcept;
    void setupPSRAMChips() noexcept;
    template<bool singleTransaction = true>
    size_t psramBlockWrite(Address address, byte *buf, size_t count) {
        MemoryCell32 translated(address);
        Address26 curr(address);
        Address26 end(address + count);
        if constexpr (singleTransaction) {
            SPI.beginTransaction(SPISettings(5_MHz, MSBFIRST, SPI_MODE0));
        }
        if (curr.getIndex() == end.getIndex()) {
            // okay they are part of the same chip so we can safely just do a single operation
            setPSRAMId(curr.getIndex());
            digitalWrite<i960Pinout::PSRAM_EN_, LOW>();
            SPI.transfer(0x02);
            SPI.transfer(translated.ordinalBytes[2]);
            SPI.transfer(translated.ordinalBytes[1]);
            SPI.transfer(translated.ordinalBytes[0]);
            SPI.transfer(buf, count);
            digitalWrite<i960Pinout::PSRAM_EN_, HIGH>();
        } else {
            // okay we are going to span multiple addresses
            // we need to compute how much will go into each area.
            // This isn't as much of a problem because a size_t is 16-bits on the 1284p
            // We will never have to span more than two psram chips which is very welcome.
            // We can easily compute the number of bytes that are going to be transferred
            // into the second chip and subtract that from the total count. That will be
            // the number of bytes to be transferred to the first chip

            // note that on any other target, this assumption would be wrong as size_t could be 4 or 8 bytes.
            // In that case, we would need to do some full chip spanning
            auto numBytesToSecondChip = end.getOffset();
            auto numBytesToFirstChip = count - numBytesToSecondChip;
            setPSRAMId(curr.getIndex());
            digitalWrite<i960Pinout::PSRAM_EN_, LOW>();
            SPI.transfer(0x02);
            SPI.transfer(translated.ordinalBytes[2]);
            SPI.transfer(translated.ordinalBytes[1]);
            SPI.transfer(translated.ordinalBytes[0]);
            SPI.transfer(buf, numBytesToFirstChip);
            digitalWrite<i960Pinout::PSRAM_EN_, HIGH>();
            setPSRAMId(end.getIndex());
            // jump ahead to the left overs and commit them
            // we are going to start at address zero of the next chip
            digitalWrite<i960Pinout::PSRAM_EN_, LOW>();
            SPI.transfer(0x02);
            SPI.transfer(0);
            SPI.transfer(0);
            SPI.transfer(0);
            SPI.transfer(buf + numBytesToFirstChip, numBytesToSecondChip);
            digitalWrite<i960Pinout::PSRAM_EN_, HIGH>();
        }
        if constexpr (singleTransaction) {
            SPI.endTransaction();
        }
        return count;
    }
    size_t psramBlockRead(Address address, byte* buf, size_t count);
private:
    byte chipId_ = 0xFF;
    // make space for the on chip request cache as well as the psram copy buffer
    // minimum size is going to be 8k or so (256 x 32) but for our current purposes we
    // are going to allocate a 4k buffer
    static constexpr auto PSRAMCopyBufferSize = 8_KB;
    union {
        byte psramCopyBuffer[PSRAMCopyBufferSize] = { 0 };
        CacheLine lines_[256];
    };
};

using SBCore = HitagiSBCore;
#endif
#endif //SIM3_HITAGISBCORE_H

