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
// Created by jwscoggins on 8/18/21.
//

#ifndef SIM3_TYPES_H
#define SIM3_TYPES_H
#include <cstdint>
using LongOrdinal = uint64_t;
using Ordinal = uint32_t;
using LongInteger = int64_t;
using Integer = int32_t;
using ShortOrdinal = uint16_t;
using ShortInteger = int16_t;
using ByteOrdinal = uint8_t;
using ByteInteger = int8_t;

using FullOpcode = uint16_t;
#ifdef NUMERICS_ARCHITECTURE
using Real = float;
using LongReal = double;
// right now, we can just exploit the fact that extended real and long real are different sizes on x86_64, just like i960
using ExtendedReal = long double;
#endif

enum class Opcode : FullOpcode  {
    None = 0,
#define X(value, name) name = value ,
#include "OpcodesRaw.h"
#undef X
    Bad = 0xFFFF,
};

enum class RegisterIndex : uint8_t {
    Local0 = 0,
    Local1,
    Local2,
    Local3,
    Local4,
    Local5,
    Local6,
    Local7,
    Local8,
    Local9,
    Local10,
    Local11,
    Local12,
    Local13,
    Local14,
    Local15,
    Global0,
    Global1,
    Global2,
    Global3,
    Global4,
    Global5,
    Global6,
    Global7,
    Global8,
    Global9,
    Global10,
    Global11,
    Global12,
    Global13,
    Global14,
    Global15,
    Literal0,
    Literal1,
    Literal2,
    Literal3,
    Literal4,
    Literal5,
    Literal6,
    Literal7,
    Literal8,
    Literal9,
    Literal10,
    Literal11,
    Literal12,
    Literal13,
    Literal14,
    Literal15,
    Literal16,
    Literal17,
    Literal18,
    Literal19,
    Literal20,
    Literal21,
    Literal22,
    Literal23,
    Literal24,
    Literal25,
    Literal26,
    Literal27,
    Literal28,
    Literal29,
    Literal30,
    Literal31,
#ifdef NUMERICS_ARCHITECTURE
    // floating point literals and registers have to be handled a tad
// differently they must be converted from the standard bits to this
// extended format to make it unique. The instruction bits itself will
// denote if the instruction operates on float data or not
FP0 = 0b010'00000,
FP1,
FP2,
FP3,
// reserved entries here
Literal0_0f = 0b010'10000,
// reserved
Literal1_0f = 0b010'10110,
#endif
    Bad = 0b1111'1111,
    PFP = Local0,
    SP = Local1,
    RIP = Local2,
    FP = Global15,
};
static_assert(static_cast<uint8_t>(RegisterIndex::Literal0) == 0b1'00000, "Literal 0 needs to be 0b100000 to accurately reflect the i960 design");

constexpr auto makeLiteral(uint8_t value) noexcept {
    return static_cast<RegisterIndex>(0b100000 | (value & 0b011111));
}
constexpr auto makeRegister(uint8_t value) noexcept {
    return static_cast<RegisterIndex>(value & 0b11111);
}
constexpr auto makeRegisterIndex(uint8_t value, bool isLiteral) noexcept {
    if (isLiteral) {
        return makeLiteral(value);
    } else {
        return makeRegister(value);
    }
}

constexpr auto isLiteral(RegisterIndex index) noexcept {
    return static_cast<uint8_t>(index) >= static_cast<uint8_t>(RegisterIndex::Literal0) &&
           static_cast<uint8_t>(index) <= static_cast<uint8_t>(RegisterIndex::Literal31);
}
constexpr auto isRegister(RegisterIndex index) noexcept {
    return static_cast<uint8_t>(index) < static_cast<uint8_t>(RegisterIndex::Literal0);
}
constexpr auto isLocalRegister(RegisterIndex index) noexcept {
    return static_cast<uint8_t>(index) < static_cast<uint8_t>(RegisterIndex::Global0);
}
constexpr auto isGlobalRegister(RegisterIndex index) noexcept {
    return isRegister(index) && !isLocalRegister(index);
}
#ifdef NUMERICS_ARCHITECTURE
constexpr auto toFloatingPointForm(RegisterIndex index) noexcept {
    if (isLiteral(index)) {
        switch (index) {
            case RegisterIndex::Literal0: return RegisterIndex::FP0;
            case RegisterIndex::Literal1: return RegisterIndex::FP1;
            case RegisterIndex::Literal2: return RegisterIndex::FP2;
            case RegisterIndex::Literal3: return RegisterIndex::FP3;
            case RegisterIndex::Literal16: return RegisterIndex::Literal0_0f;
            case RegisterIndex::Literal22: return RegisterIndex::Literal1_0f;
            default: return RegisterIndex::Bad;
        }
    } else {
        return index;
    }
}

constexpr auto isFloatingPointRegister(RegisterIndex index) noexcept {
    switch (index) {
        case RegisterIndex::FP0:
            case RegisterIndex::FP1:
                case RegisterIndex::FP2:
                    case RegisterIndex::FP3:
                        return true;
                        default:
                            return false;
    }
}

constexpr auto isFloatingPointLiteral(RegisterIndex index) noexcept {
    return index == RegisterIndex::Literal0_0f || index == RegisterIndex::Literal1_0f;
}
static_assert(toFloatingPointForm(RegisterIndex::Bad) == RegisterIndex::Bad, "toFloatingPointForm is not passing through correctly!");
#endif

constexpr Ordinal getLiteralOrdinal(RegisterIndex index) noexcept {
    if (isLiteral(index)) {
        return static_cast<Ordinal>(static_cast<uint8_t>(index) & 0b11111);
    } else {
        return 0xFFFF'FFFF;
    }
}

constexpr auto operator "" _KB(unsigned long long value) noexcept {
    return value * 1024;
}
constexpr auto operator "" _MB(unsigned long long value) noexcept {
    return value * 1024 * 1024;
}

union MemoryCell32 {
    template<typename T>
    static constexpr Ordinal NumThingsPerOrdinal = sizeof(Ordinal) / sizeof(T);
    template<typename T>
    static constexpr Ordinal NumThingsPerOrdinalMask = NumThingsPerOrdinal<T> - 1;
    static constexpr Ordinal NumShortOrdinalsPerOrdinal = NumThingsPerOrdinal<ShortOrdinal >;
    static constexpr Ordinal NumShortIntegersPerOrdinal = NumThingsPerOrdinal<ShortInteger>;
    static constexpr Ordinal NumByteOrdinalsPerOrdinal = NumThingsPerOrdinal<ByteOrdinal >;
    static constexpr Ordinal NumByteIntegersPerOrdinal = NumThingsPerOrdinal<ByteInteger>;
    static constexpr Ordinal NumShortOrdinalsPerOrdinalMask = NumThingsPerOrdinalMask<ShortOrdinal >;
    static constexpr Ordinal NumShortIntegersPerOrdinalMask = NumThingsPerOrdinalMask<ShortInteger>;
    static constexpr Ordinal NumByteOrdinalsPerOrdinalMask = NumThingsPerOrdinalMask<ByteOrdinal >;
    static constexpr Ordinal NumByteIntegersPerOrdinalMask = NumThingsPerOrdinalMask<ByteInteger>;
public:
    constexpr explicit MemoryCell32(Ordinal value = 0) noexcept : raw(value) { }
    constexpr MemoryCell32(ShortOrdinal lower, ShortOrdinal upper) noexcept : ordinalShorts{lower, upper} { }
    constexpr MemoryCell32(ByteOrdinal lowest, ByteOrdinal lower, ByteOrdinal higher, ByteOrdinal highest)  noexcept : ordinalBytes{lowest, lower, higher, highest} { }
    constexpr Ordinal getOrdinalValue() const noexcept { return raw; }
    void setOrdinalValue(Ordinal value) noexcept { raw = value; }
    constexpr ShortOrdinal getShortOrdinal(Ordinal which) const noexcept { return ordinalShorts[which & NumShortOrdinalsPerOrdinalMask]; }
    constexpr ShortInteger getShortInteger(Ordinal which) const noexcept {return integerShorts[which & NumShortIntegersPerOrdinalMask]; }
    void setShortOrdinal(ShortOrdinal value, Ordinal which) noexcept { ordinalShorts[which & NumShortOrdinalsPerOrdinalMask] = value; }
    void setShortInteger(ShortInteger value, Ordinal which) noexcept { integerShorts[which & NumShortIntegersPerOrdinalMask] = value; }
    constexpr ByteOrdinal getByteOrdinal(Ordinal which) const noexcept {return ordinalBytes[which & NumByteOrdinalsPerOrdinalMask]; }
    constexpr ByteInteger getByteInteger(Ordinal which) const noexcept {return integerBytes[which & NumByteIntegersPerOrdinalMask]; }
    void setByteOrdinal(ByteOrdinal value, Ordinal which) noexcept { ordinalBytes[which & NumByteOrdinalsPerOrdinalMask] = value; }
    void setByteInteger(ByteInteger value, Ordinal which) noexcept { integerBytes[which & NumByteIntegersPerOrdinalMask] = value; }
    void clear() noexcept {
        raw = 0;
    }
private:
    Ordinal raw;
    ShortOrdinal ordinalShorts[NumShortOrdinalsPerOrdinal];
    ShortInteger integerShorts[NumShortIntegersPerOrdinal];
    ByteOrdinal ordinalBytes[NumByteOrdinalsPerOrdinal];
    ByteInteger integerBytes[NumByteIntegersPerOrdinal];
};
static_assert(sizeof(MemoryCell32) == sizeof(Ordinal), "MemoryCell32 must be the same size as a long ordinal");
#endif //SIM3_TYPES_H
