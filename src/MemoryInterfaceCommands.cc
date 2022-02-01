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


namespace {
    constexpr auto getCPUClockFrequency() noexcept {
        return F_CPU;
    }
    constexpr auto EnableEmulatorTrace = false;

    class SPIInterface {
    public:
        enum class Registers : byte {
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
        static constexpr decltype(getCPUClockFrequency()) SPIClockFrequencies[] {
                getCPUClockFrequency() / 4,
                getCPUClockFrequency() / 16,
                getCPUClockFrequency() / 64,
                getCPUClockFrequency() / 128,
                getCPUClockFrequency() / 2,
                getCPUClockFrequency() / 8,
                getCPUClockFrequency() / 32,
                getCPUClockFrequency() / 64,
        };
        static constexpr byte SPIClockFrequencies_Decomposed[8][4]{
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
    public:
        SPIInterface() = delete;
        ~SPIInterface() = delete;
        SPIInterface(SPIInterface&&) = delete;
        SPIInterface(const SPIInterface&) = delete;
        SPIInterface& operator=(const SPIInterface&) = delete;
        SPIInterface& operator=(SPIInterface&&) = delete;
        [[nodiscard]] static byte computeClockRateSetup() noexcept {
            return (SPCR & 0b11) | ((SPSR | 0b1) << 3);
        }
        static byte read(byte offset) noexcept {
            switch (static_cast<Registers>(offset)) {
                case Registers::Data:
                    return SPDR;
                case Registers::PeripheralEnable:
                    return (SPCR & _BV(SPE)) ? 0xFF : 0x00;
                case Registers::ClockPolarity:
                    return (SPCR & _BV(CPOL)) ? 0xFF : 0x00;
                case Registers::ClockPhase:
                    return (SPCR & _BV(CPHA)) ? 0xFF : 0x00;
                case Registers::DataOrder:
                    return (SPCR & _BV(DORD)) ? 0xFF : 0x00;
                case Registers::ClockRate:
                    return computeClockRateSetup();
                case Registers::WriteCollision:
                    return (SPSR & _BV(WCOL)) ? 0xFF : 0x00;
                case Registers::TransferComplete:
                    return (SPSR & _BV(SPIF)) ? 0xFF : 0x00;
                case Registers::ClockFrequency00: return SPIClockFrequencies_Decomposed[computeClockRateSetup()][0];
                case Registers::ClockFrequency01: return SPIClockFrequencies_Decomposed[computeClockRateSetup()][1];
                case Registers::ClockFrequency10: return SPIClockFrequencies_Decomposed[computeClockRateSetup()][2];
                case Registers::ClockFrequency11: return SPIClockFrequencies_Decomposed[computeClockRateSetup()][3];
                default:
                    return 0;
            }
        }
        static void write(byte offset, byte value) noexcept {
            switch (static_cast<Registers>(offset)) {
                case Registers::Data:
                    SPDR = offset;
                    break;
                case Registers::PeripheralEnable:
                    if (value == 0) {
                        SPI.end();
                    } else {
                        SPI.begin();
                    }
                    break;
                case Registers::ClockPolarity:
                    if (value == 0) {
                        SPCR &= ~_BV(CPOL);
                    } else {
                        SPCR |= _BV(CPOL);
                    }
                    break;
                case Registers::ClockPhase:
                    if (value == 0) {
                        SPCR &= ~_BV(CPHA);
                    } else {
                        SPCR |= _BV(CPHA);
                    }
                    break;
                case Registers::DataOrder:
                    if (value == 0) {
                        SPCR &= ~_BV(DORD);
                    } else {
                        SPCR |= _BV(DORD);
                    }
                    break;
                case Registers::ClockRate:
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
    };
    class QueryInterface {
    public:
        enum class Registers : byte {
#define Register16(name) name ## 0, name ## 1
#define Register32(name) Register16(name ## 0), Register16(name ## 1)
            Register32(ClockFrequency),
#undef Register32
#undef Register16
        };
        QueryInterface() = delete;
        ~QueryInterface() = delete;
        QueryInterface(QueryInterface&&) = delete;
        QueryInterface(const QueryInterface&) = delete;
        QueryInterface& operator=(const QueryInterface&) = delete;
        QueryInterface& operator=(QueryInterface&&) = delete;
    public:
        static byte read(byte offset) noexcept {
            switch (static_cast<Registers>(offset)) {
                case Registers::ClockFrequency00: return static_cast<byte>(getCPUClockFrequency());
                case Registers::ClockFrequency01: return static_cast<byte>(getCPUClockFrequency() >> 8);
                case Registers::ClockFrequency10: return static_cast<byte>(getCPUClockFrequency() >> 16);
                case Registers::ClockFrequency11: return static_cast<byte>(getCPUClockFrequency() >> 24);
                default:
                    return 0;
            }
        }
        static void write(byte, byte) noexcept { }
    };
    class SerialConsole {
    public:
        enum class Registers : byte {
            Data,
            Available,
            AvailableForWrite,
            Status,
#define Register16(name) name ## 0, name ## 1
#define Register32(name) Register16(name ## 0), Register16(name ## 1)
            Register32(ClockFrequency),
#undef Register32
#undef Register16
        };
    public:
        SerialConsole() = delete;
        ~SerialConsole() = delete;
        SerialConsole(SerialConsole&&) = delete;
        SerialConsole(const SerialConsole&) = delete;
        SerialConsole& operator=(const SerialConsole&) = delete;
        SerialConsole& operator=(SerialConsole&&) = delete;
        static void setClockRate(uint32_t rate) noexcept { clockRate_ = rate; }
        [[nodiscard]] static auto getClockRate() noexcept { return clockRate_; }
        static void putCharacter(byte value) noexcept { Serial.write(value); }
        [[nodiscard]] static byte getCharacter() noexcept { return static_cast<byte>(Serial.read()); }
        [[nodiscard]] static byte availableForWrite() noexcept { return Serial.availableForWrite(); }
        [[nodiscard]] static byte available() noexcept { return Serial.available(); }
        [[nodiscard]] static byte getStatus() noexcept { return Serial ? 0xFF : 0x00; }
        inline static void setStatus(byte value) noexcept {
            setStatus(value != 0);
        }
        static void setStatus(bool value) noexcept {
            if (value) {
                begin();
            } else {
                end();
            }
        }
        static void end() noexcept {
            if (Serial) {
                Serial.end();
            }
        }
        static void begin() noexcept {
            if (!Serial) {
                Serial.begin(clockRate_);
            }
        }
        static void write(byte offset, byte value) noexcept {
            switch (static_cast<Registers>(offset)) {
                case Registers::Data:
                    putCharacter(value);
                    break;
                case Registers::Status:
                    setStatus(value);
                    break;
                case Registers::ClockFrequency00:
                    clockRate_ = ((clockRate_ & 0xFFFFFF00) | static_cast<uint32_t>(value));
                    break;
                case Registers::ClockFrequency01:
                    clockRate_ = ((clockRate_ & 0xFFFF00FF) | (static_cast<uint32_t>(value)  << 8));
                    break;
                case Registers::ClockFrequency10:
                    clockRate_ = ((clockRate_ & 0xFF00FFFF) | (static_cast<uint32_t>(value)  << 16));
                    break;
                case Registers::ClockFrequency11:
                    clockRate_ = ((clockRate_ & 0x00FFFFFF) | (static_cast<uint32_t>(value)  << 24));
                    break;
                default:
                    break;
            }
        }
        [[nodiscard]] static byte read(byte offset) noexcept {
            switch (static_cast<Registers>(offset)) {
                case Registers::Data: return getCharacter();
                case Registers::Status: return getStatus();
                case Registers::Available: return available();
                case Registers::AvailableForWrite: return availableForWrite();
                case Registers::ClockFrequency00: return static_cast<byte>(clockRate_);
                case Registers::ClockFrequency01: return static_cast<byte>(clockRate_ >> 8);
                case Registers::ClockFrequency10: return static_cast<byte>(clockRate_ >> 16);
                case Registers::ClockFrequency11: return static_cast<byte>(clockRate_ >> 24);
                default:
                    return 0;
            }
        }
    private:
        static inline uint32_t clockRate_ {115200};
    };
    class GPIOInterface {
    public:
        enum class Registers : byte {

        };
    public:
        GPIOInterface() = delete;
        ~GPIOInterface() = delete;
        GPIOInterface(GPIOInterface&&) = delete;
        GPIOInterface(const GPIOInterface&) = delete;
        GPIOInterface& operator=(const GPIOInterface&) = delete;
        GPIOInterface& operator=(GPIOInterface&&) = delete;
    public:
        static void write(byte offset, byte value) noexcept {
            /// @todo implement
        }
        static byte read(byte offset) noexcept {
            /// @todo implement
            return 0;
        }
    };
}
ByteOrdinal
Core::readFromInternalSpace(Address destination) noexcept {
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
                return loadFromBus(destination, TreatAsByteOrdinal{});
            }
        case InternalPeripheralStart:
            if (destination >= Builtin::ConfigurationSpaceBaseAddress) {
                return EEPROM.read(static_cast<int>(destination & 0xFFF));
            } else {
                /// @todo handle other devices
                switch (auto offset = static_cast<byte>(destination); Builtin::addressToTargetPeripheral(destination))  {
                    case Builtin::Devices::SPI:
                        return SPIInterface::read(offset);
                    case Builtin::Devices::Query:
                        return QueryInterface::read(offset);
                    case Builtin::Devices::IO:
                        return GPIOInterface::read(offset);
                    case Builtin::Devices::SerialConsole:
                        return SerialConsole::read(offset);
                    default:
                        return loadFromBus(destination, TreatAsByteOrdinal{});
                }
            }
        default:
            return loadFromBus(destination, TreatAsByteOrdinal{});
    }
}
void
Core::writeToInternalSpace(Address destination, byte value) noexcept {
    constexpr byte BootProgramBaseStart = static_cast<byte>(Builtin::InternalBootProgramBase >> 16);
    constexpr byte InternalPeripheralStart = static_cast<byte>(Builtin::InternalPeripheralBase >> 16);
    constexpr byte InternalSRAMStart = static_cast<byte>(Builtin::InternalSRAMBase >> 16);
    byte subOffset = static_cast<byte>(destination >> 16);
    switch (subOffset) {
        case BootProgramBaseStart:
            // ignore writes made to this location
            break;
        case InternalSRAMStart:
            if (auto offset = destination - InternalSRAMStart; offset < Builtin::InternalSRAMEnd) {
                internalSRAM_[static_cast<size_t>(offset)] = value;
            } else {
                storeToBus(destination, value, TreatAsByteOrdinal{});
            }
            break;
        case InternalPeripheralStart:
            if (destination >= Builtin::ConfigurationSpaceBaseAddress) {
                EEPROM.update(static_cast<int>(destination & 0xFFF), value);
            } else {
                switch (auto offset = static_cast<byte>(destination); Builtin::addressToTargetPeripheral(destination))  {
                    case Builtin::Devices::SPI:
                        SPIInterface::write(offset, value);
                        break;
                    case Builtin::Devices::IO:
                        GPIOInterface::write(offset, value);
                        break;
                    case Builtin::Devices::SerialConsole:
                        SerialConsole::write(offset, value);
                        break;
                    default:
                        storeToBus(destination, value, TreatAsByteOrdinal{});
                        break;

                }
            }
            break;
        default:
            storeToBus(destination, value, TreatAsByteOrdinal{});
            break;
    }
}

void
Core::load(Address destination, TripleRegister& reg) noexcept {
    for (int i = 0; i < 3; ++i, destination += sizeof(Ordinal)) {
        reg.setOrdinal(load(destination), i);
    }
}
void
Core::store(Address destination, const TripleRegister& reg) noexcept {
    for (int i = 0; i < 3; ++i, destination += sizeof(Ordinal)) {
        store(destination, reg.getOrdinal(i), TreatAsOrdinal{});
    }
}
void
Core::load(Address destination, QuadRegister& reg) noexcept {
    reg.setLowerHalf(load(destination, TreatAsLongOrdinal{}));
    reg.setUpperHalf(load(destination + sizeof(LongOrdinal), TreatAsLongOrdinal {}));
}
void
Core::store(Address destination, const QuadRegister& reg) noexcept {
    store(destination, reg.getLowerHalf(), TreatAsLongOrdinal{});
    store(destination+sizeof(LongOrdinal), reg.getUpperHalf(), TreatAsLongOrdinal{});
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
    storeLong(destination, value.get(TreatAsLongOrdinal{}));
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
        store(destination, value.get<Ordinal>());
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
