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
#include <EEPROM.h>
#include <SPI.h>
#include "Core.h"
#include "Types.h"
#include "InternalBootProgram.h"

/**
 * @brief Defines the start of the internal cache memory connected on the EBI, this is used by the microcontroller itself for whatever it needs (lower 32k)
 */
constexpr size_t CacheMemoryWindowStart = (RAMEND + 1);
/**
 * @brief Defines the start of the window used to peer out into the external bus itself
 */
constexpr size_t BusMemoryWindowStart = 0x8000;


namespace {
    constexpr auto getCPUClockFrequency() noexcept {
        return F_CPU;
    }
    enum class SPIRegisters : byte {
#define Register16(name) name ## 0, name ## 1
#define Register32(name) Register16(name ## 0), Register16(name ## 1)
        Data,
        TransferComplete,
        WriteCollision,
        ClockPhase,
        ClockPolarity,
        ClockRate,
        DataOrder,
        PeripheralEnable,
        Register32(ClockFrequency),
#undef Register32
#undef Register16
    };
    constexpr size_t computeWindowOffsetAddress(size_t offset) noexcept {
        return BusMemoryWindowStart + (offset & 0x7FFF);
    }
    ByteOrdinal
    readFromBusWindow(size_t offset) noexcept {
        return memory<ByteOrdinal>(computeWindowOffsetAddress(offset));
    }

    void
    writeToBusWindow(size_t offset, ByteOrdinal value) noexcept {
        memory<ByteOrdinal>(computeWindowOffsetAddress(offset)) = value;
    }
    constexpr auto EnableEmulatorTrace = false;
    constexpr decltype(getCPUClockFrequency()) SPIClockFrequencies[] {
        getCPUClockFrequency() / 4,
        getCPUClockFrequency() / 16,
        getCPUClockFrequency() / 64,
        getCPUClockFrequency() / 128,
        getCPUClockFrequency() / 2,
        getCPUClockFrequency() / 8,
        getCPUClockFrequency() / 32,
        getCPUClockFrequency() / 64,
    };
    constexpr byte SPIClockFrequencies_Decomposed[8][4]{
#define X(ind) { static_cast<byte>(SPIClockFrequencies[ind]), static_cast<byte>(SPIClockFrequencies[ind] >> 8), static_cast<byte>(SPIClockFrequencies[ind] >> 16), static_cast<byte>(SPIClockFrequencies[ind] >> 24), }
            X(0),
            X(1),
            X(2),
            X(3),
            X(4),
            X(5),
            X(6),
            X(7),
#undef X
    };
    byte doSPIReads(byte offset) noexcept {
        auto computeClockRateSetup = []() {
            return (SPCR & 0b11) | ((SPSR | 0b1) << 3);
        };
        switch (static_cast<SPIRegisters>(offset)) {
            case SPIRegisters::Data:
                return SPDR;
            case SPIRegisters::PeripheralEnable:
                return (SPCR & _BV(SPE)) ? 0xFF : 0x00;
            case SPIRegisters::ClockPolarity:
                return (SPCR & _BV(CPOL)) ? 0xFF : 0x00;
            case SPIRegisters::ClockPhase:
                return (SPCR & _BV(CPHA)) ? 0xFF : 0x00;
            case SPIRegisters::DataOrder:
                return (SPCR & _BV(DORD)) ? 0xFF : 0x00;
            case SPIRegisters::ClockRate:
                return computeClockRateSetup();
            case SPIRegisters::WriteCollision:
                return (SPSR & _BV(WCOL)) ? 0xFF : 0x00;
            case SPIRegisters::TransferComplete:
                return (SPSR & _BV(SPIF)) ? 0xFF : 0x00;
            case SPIRegisters::ClockFrequency00: return SPIClockFrequencies_Decomposed[computeClockRateSetup()][0];
            case SPIRegisters::ClockFrequency01: return SPIClockFrequencies_Decomposed[computeClockRateSetup()][1];
            case SPIRegisters::ClockFrequency10: return SPIClockFrequencies_Decomposed[computeClockRateSetup()][2];
            case SPIRegisters::ClockFrequency11: return SPIClockFrequencies_Decomposed[computeClockRateSetup()][3];
            default:
                return 0;
        }
    }
    void doSPIWrite(byte offset, byte value) noexcept {
        switch (static_cast<SPIRegisters>(offset)) {
            case SPIRegisters::Data:
                SPDR = offset;
                break;
            case SPIRegisters::PeripheralEnable:
                if (value == 0) {
                    SPI.end();
                } else {
                    SPI.begin();
                }
                break;
            case SPIRegisters::ClockPolarity:
                if (value == 0) {
                    SPCR &= ~_BV(CPOL);
                } else {
                    SPCR |= _BV(CPOL);
                }
                break;
            case SPIRegisters::ClockPhase:
                if (value == 0) {
                    SPCR &= ~_BV(CPHA);
                } else {
                    SPCR |= _BV(CPHA);
                }
                break;
            case SPIRegisters::DataOrder:
                if (value == 0) {
                    SPCR &= ~_BV(DORD);
                } else {
                    SPCR |= _BV(DORD);
                }
                break;
            case SPIRegisters::ClockRate:
                [value]() noexcept {
                    if (value & 0b001) {
                        SPCR &= ~_BV(SPR0);
                    } else {
                        SPCR |= _BV(SPR0);
                    }
                    if (value & 0b010) {
                        SPCR &= ~_BV(SPR1);
                    } else {
                        SPCR |= _BV(SPR1);
                    }
                    if (value & 0b100) {
                        SPSR &= ~_BV(SPI2X);
                    } else {
                        SPSR |= _BV(SPI2X);
                    }
                }();
                break;
            default:
                break;
        }
    }
    enum class QueryRegisters : byte {
#define Register16(name) name ## 0, name ## 1
#define Register32(name) Register16(name ## 0), Register16(name ## 1)
    Register32(ClockFrequency),
#undef Register32
#undef Register16
    };
    byte
    readQuerySpace(byte offset) noexcept {
        switch (static_cast<QueryRegisters>(offset)) {
            case QueryRegisters::ClockFrequency00: return static_cast<byte>(getCPUClockFrequency());
            case QueryRegisters::ClockFrequency01: return static_cast<byte>(getCPUClockFrequency() >> 8);
            case QueryRegisters::ClockFrequency10: return static_cast<byte>(getCPUClockFrequency() >> 16);
            case QueryRegisters::ClockFrequency11: return static_cast<byte>(getCPUClockFrequency() >> 24);
            default:
                return 0;
        }
    }
}
ByteOrdinal
Core::loadByte(Address destination) {
    if (static_cast<byte>(destination >> 24) == 0xFF) {
        constexpr byte BootProgramBaseStart = static_cast<byte>(Builtin::InternalBootProgramBase >> 16);
        constexpr byte InternalPeripheralStart = static_cast<byte>(Builtin::InternalPeripheralBase >> 16);
        constexpr byte InternalSRAMStart = static_cast<byte>(Builtin::InternalSRAMBase >> 16);
        byte subOffset = static_cast<byte>(destination >> 16);
        switch (subOffset) {
            case BootProgramBaseStart:
                return readFromInternalBootProgram(static_cast<size_t>(destination - Builtin::InternalBootProgramBase));
            case InternalSRAMStart:
                if (auto offset = destination - InternalSRAMStart; offset < Builtin::InternalSRAMEnd) {
                    return internalSRAM_[static_cast<size_t>(offset)];
                } else {
                    return 0;
                }
            case InternalPeripheralStart:
                if (destination >= Builtin::ConfigurationSpaceBaseAddress) {
                    return EEPROM.read(static_cast<int>(destination & 0xFFF));
                } else {
                    /// @todo handle other devices
                    switch (Builtin::addressToTargetPeripheral(destination))  {
                        case Builtin::Devices::SPI:
                            return doSPIReads(static_cast<byte>(destination));
                        case Builtin::Devices::Query:
                            return readQuerySpace(static_cast<byte>(destination));
                        default:
                            return 0;
                    }
                }
            default:
                break;
        }
        return 0;
    } else {
        setEBIUpper(destination);
        return readFromBusWindow(static_cast<size_t>(destination));
    }
}
void
Core::storeByte(Address destination, ByteOrdinal value) {
    if (static_cast<byte>(destination >> 24) == 0xFF) {
        constexpr byte BootProgramBaseStart = static_cast<byte>(Builtin::InternalBootProgramBase >> 16);
        constexpr byte InternalPeripheralStart = static_cast<byte>(Builtin::InternalPeripheralBase >> 16);
        constexpr byte InternalSRAMStart = static_cast<byte>(Builtin::InternalSRAMBase >> 16);
        byte subOffset = static_cast<byte>(destination >> 16);
        switch (subOffset) {
            case BootProgramBaseStart:
                break;
            case InternalSRAMStart:
                if (auto offset = destination - InternalSRAMStart; offset < Builtin::InternalSRAMEnd) {
                    internalSRAM_[static_cast<size_t>(offset)] = value;
                }
                break;
            case InternalPeripheralStart:
                if (destination >= Builtin::ConfigurationSpaceBaseAddress) {
                    EEPROM.update(static_cast<int>(destination & 0xFFF), value);
                } else {
                    switch (Builtin::addressToTargetPeripheral(destination))  {
                        case Builtin::Devices::SPI:
                            doSPIWrite(static_cast<byte>(destination), value);
                            break;
                        case Builtin::Devices::I2C:
                        case Builtin::Devices::IO:
                        case Builtin::Devices::Query:
                        case Builtin::Devices::AnalogComparator:
                        default:
                            break;

                    }
                }
                break;
            default:
                break;
        }
    } else {
        setEBIUpper(destination);
        writeToBusWindow(static_cast<size_t>(destination), value);
    }
}

Ordinal
Core::load(Address destination) {
    union {
       byte bytes[sizeof(Ordinal)] ;
       Ordinal value;
    } container;
    for (size_t i = 0; i < sizeof(Ordinal); ++i, ++destination) {
        container.bytes[i] = loadByte(destination);
    }
    return container.value;
}

ShortOrdinal
Core::loadShort(Address destination) noexcept {
    union {
        byte bytes[sizeof(ShortOrdinal)] ;
        ShortOrdinal value;
    } container;
    for (size_t i = 0; i < sizeof(ShortOrdinal); ++i, ++destination) {
        container.bytes[i] = loadByte(destination);
    }
    return container.value;
}

void
Core::store(Address destination, Ordinal value) {
    using K = decltype(value);
    union {
        K value;
        byte bytes[sizeof(K)];
    } container;
    container.value = value;
    for (size_t i = 0; i < sizeof(K); ++i, ++destination) {
        storeByte(destination, container.bytes[i]);
    }
}
void
Core::storeShort(Address destination, ShortOrdinal value) {
    using K = decltype(value);
    union {
        K value;
        byte bytes[sizeof(K)];
    } container;
    container.value = value;
    for (size_t i = 0; i < sizeof(K); ++i, ++destination) {
        storeByte(destination, container.bytes[i]);
    }
}
void
Core::storeLong(Address destination, LongOrdinal value) {
    using K = decltype(value);
    union {
        K value;
        byte bytes[sizeof(K)];
    } container;
    container.value = value;
    for (size_t i = 0; i < sizeof(K); ++i, ++destination) {
        storeByte(destination, container.bytes[i]);
    }
}
void
Core::store(Address destination, const TripleRegister& reg) {
    for (size_t i = 0; i < 3; ++i, destination += sizeof(Ordinal)) {
        store(destination, reg.getOrdinal(i));
    }
}
void
Core::store(Address destination, const QuadRegister& reg) {
    for (size_t i = 0; i < 4; ++i, destination += sizeof(Ordinal)) {
        store(destination, reg.getOrdinal(i));
    }
}
void
Core::storeShortInteger(Address destination, ShortInteger value) {
    union {
        ShortInteger in;
        ShortOrdinal out;
    } thing;
    thing.in = value;
    storeShort(destination, thing.out);
}
void
Core::storeByteInteger(Address destination, ByteInteger value) {
    union {
        ByteInteger in;
        ByteOrdinal out;
    } thing;
    thing.in = value;
    storeByte(destination, thing.out);
}
LongOrdinal
Core::loadLong(Address destination) {
    DoubleRegister reg;
    for (int i = 0; i < 2; ++i, destination += sizeof(Ordinal)) {
        reg.setOrdinal(load(destination), i);
    }
    return reg.getLongOrdinal();
}
void
Core::load(Address destination, TripleRegister& reg) noexcept {
    for (int i = 0; i < 3; ++i, destination += sizeof(Ordinal)) {
        reg.setOrdinal(load(destination), i);
    }
}
void
Core::load(Address destination, QuadRegister& reg) noexcept {
    for (int i = 0; i < 4; ++i, destination += sizeof(Ordinal)) {
        reg.setOrdinal(load(destination), i);
    }
}
QuadRegister
Core::loadQuad(Address destination) noexcept {
    QuadRegister tmp;
    load(destination, tmp);
    return tmp;
}
void Core::synchronizedStore(Address destination, const DoubleRegister& value) noexcept {
    // there is a lookup for an interrupt control register, in the Sx manual, we are going to ignore that for now
    synchronizeMemoryRequests();
    storeLong(destination, value.getLongOrdinal());
}
void Core::synchronizedStore(Address destination, const QuadRegister& value) noexcept {
    synchronizeMemoryRequests();
    if (destination == 0xFF00'0010) {
        IACMessage msg(value);
        processIACMessage(msg);
        // there are special IAC messages we need to handle here
    } else {
        // synchronized stores are always aligned but still go through the normal mechanisms
        store(destination, value);
    }
}
void Core::synchronizedStore(Address destination, const Register& value) noexcept {
    // there is a lookup for an interrupt control register, in the Sx manual, we are going to ignore that for now
    synchronizeMemoryRequests();
    if (destination == 0xFF00'0004) {
        // interrupt control register is here, ignore it for now
        if constexpr (EnableEmulatorTrace) {
            Serial.println(F("Writing To Interrupt Control Register!!!"));
        }
    } else {
        store(destination, value.getOrdinal());
    }
}

void
Core::setEBIUpper(Address address) noexcept {
    static constexpr Address upperMask = 0xFFFF'8000;
    static constexpr Address bit15Mask = 0x0000'8000;
    auto realAddress = address & upperMask;
    if (realAddress != ebiUpper_) {
        // set our fake A15
        digitalWrite(Pinout::EBI_A15, bit15Mask & realAddress ? HIGH : LOW);
        PORTL = static_cast<byte>(address >> 16);
        PORTK = static_cast<byte>(address >> 24);
        ebiUpper_ = realAddress;
    }
}
