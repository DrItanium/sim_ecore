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
//
// Created by jwscoggins on 8/18/21.
//

#ifndef SIM3_TYPES_H
#define SIM3_TYPES_H
#ifdef DESKTOP_BUILD
#include <cstdint>
#endif
#ifdef ARDUINO
#include <Arduino.h>
#endif

using LongOrdinal = uint64_t;
using Ordinal = uint32_t;
using LongInteger = int64_t;
using Integer = int32_t;
using ShortOrdinal = uint16_t;
using ShortInteger = int16_t;
using ByteOrdinal = uint8_t;
using ByteInteger = int8_t;

using Address = Ordinal;
using FullOpcode = uint16_t;
using Real = float;
using LongReal = double;
#ifdef NUMERICS_ARCHITECTURE
// right now, we can just exploit the fact that extended real and long real are different sizes on x86_64, just like i960
using ExtendedReal = long double;
#endif
/**
 * @brief A class meant to make tag dispatch easy
 * @tparam T The type desired through tag dispatch
 */
template <typename T>
struct TreatAs final {
    /**
     * @brief A way to identify the target type in template metaprogramming
     */
    using UnderlyingType = T;
};
using TreatAsOrdinal = TreatAs<Ordinal>;
using TreatAsInteger = TreatAs<Integer>;
using TreatAsLongOrdinal = TreatAs<LongOrdinal>;
using TreatAsLongInteger = TreatAs<LongInteger>;
using TreatAsShortOrdinal = TreatAs<ShortOrdinal>;
using TreatAsShortInteger = TreatAs<ShortInteger>;
using TreatAsByteOrdinal = TreatAs<ByteOrdinal>;
using TreatAsByteInteger = TreatAs<ByteInteger>;
using TreatAsReal = TreatAs<Real>;
using TreatAsLongReal = TreatAs<LongReal>;
#ifdef NUMERICS_ARCHITECTURE
using TreatAsExtendedReal = TreatAs<ExtendedReal>;
#endif
constexpr Ordinal bitsNeeded(Ordinal n) noexcept {
    // taken from https://stackoverflow.com/questions/23781506/compile-time-computing-of-number-of-bits-needed-to-encode-n-different-states
    return n <= 1 ? 0 : 1 + bitsNeeded((n+1) / 2);
}
static_assert(bitsNeeded(64) == 6, "Sanity check of bitsNeeded(64) failed");
/**
 * @brief List of opcodes translated from their 8/12-bit equivalents to a standard 12-bit opcode in all cases; Non-REG instructions are shifted left by four bits
 */
enum class Opcode : FullOpcode  {
    None = 0,
#define X(value, name) name = value ,
#include "OpcodesRaw.h"
#undef X
    Bad = 0xFFFF,
};

/**
 * @brief A unique representation for each type of operand an instruction is capable of showing. Floating point instructions require two levels conversion.
 */
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
    Bad = 0b1111'1111,
    PFP = Local0,
    SP960 = Local1,
    RIP = Local2,
    FP = Global15,
    // floating point numbers reuse existing infrastructure but introduce a new theoretical dimension via instruction type
    // If the type is non-fp, then it is lower 32 are registers, upper 32 are literals
    // If the type is fp, then the lower 32 are registers and the upper 32 are devoted to either one of the four fp regs or +0.0 and +1.0
    //      all other values are marked as reserved. To save on encoding space, we will be aliasing the floating point specific names to literals
    //
    // Previously, I was providing these registers with a unique index but this introduces new problems with special function registers
    // and such. This is the cleanest way to add floating point register references without doing really goofy mappings
    FP0 = Literal0,
    FP1 = Literal1,
    FP2 = Literal2,
    FP3 = Literal3,
    Literal0_0f = Literal16, // 0b110000
    Literal1_0f = Literal22, // 0b110110
};
static_assert(static_cast<uint8_t>(RegisterIndex::Literal0) == 0b1'00000, "Literal 0 needs to be 0b100000 to accurately reflect the i960 design");
/**
 * @brief Convert the given byte value into a literal; the value must be in the range [0,32) with this function only reading the bottom 5 bits
 * @param value The 8-bit literal
 * @return The lowest 5-bits of the literal converted to a RegisterIndex
 */
constexpr auto makeLiteral(uint8_t value) noexcept {
    return static_cast<RegisterIndex>(0b100000 | (value & 0b011111));
}
/**
 * @brief Convert the given numeric index into a specific register index
 * @param value The 8-bit literal
 * @return The lowest 5-bits of the provided literal interpreted as a RegisterIndex
 */
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

constexpr Ordinal getLiteral(RegisterIndex index, TreatAsOrdinal) noexcept {
    if (isLiteral(index)) {
        return static_cast<Ordinal>(static_cast<uint8_t>(index) & 0b11111);
    } else {
        return 0xFFFF'FFFF;
    }
}
constexpr Integer getLiteral(RegisterIndex index, TreatAsInteger) noexcept {
    if (isLiteral(index)) {
        return static_cast<Integer>(static_cast<uint8_t>(index) & 0b11111);
    } else {
        return -1;
    }
}
constexpr ByteOrdinal getLiteral(RegisterIndex index, TreatAsByteOrdinal) noexcept {
    return static_cast<ByteOrdinal>(getLiteral(index, TreatAsOrdinal{}));
}

constexpr ByteInteger getLiteral(RegisterIndex index, TreatAsByteInteger) noexcept {
    return static_cast<ByteInteger>(getLiteral(index, TreatAsInteger{}));
}

constexpr ShortOrdinal getLiteral(RegisterIndex index, TreatAsShortOrdinal) noexcept {
    return static_cast<ShortOrdinal>(getLiteral(index, TreatAsOrdinal{}));
}

constexpr ShortInteger getLiteral(RegisterIndex index, TreatAsShortInteger) noexcept {
    return static_cast<ShortInteger>(getLiteral(index, TreatAsInteger{}));
}

constexpr Real getLiteral(RegisterIndex index, TreatAsReal) noexcept {
    switch (index) {
        case RegisterIndex::Literal0_0f: return 0.0f;
        case RegisterIndex::Literal1_0f: return 1.0f;
        default: return NAN;
    }
}

constexpr LongReal getLiteral(RegisterIndex index, TreatAsLongReal) noexcept {
    switch (index) {
        case RegisterIndex::Literal0_0f: return 0.0;
        case RegisterIndex::Literal1_0f: return 1.0;
        default: return NAN;
    }
}


constexpr unsigned long long int operator "" _KB(unsigned long long value) noexcept {
    return value * 1024;
}
constexpr unsigned long long int operator "" _MB(unsigned long long value) noexcept {
    return value * 1024 * 1024;
}

constexpr unsigned long long int operator "" _KHz(unsigned long long value) noexcept {
    return value * 1000;
}
constexpr unsigned long long int operator "" _MHz(unsigned long long value) noexcept {
    return value * 1000 * 1000;
}

constexpr uint32_t bytesNeeded(uint32_t value) noexcept {
    if (value == 0) {
        return 1;
    } else {
        return bytesNeeded(value - 1) * 2;
    }
}
constexpr Ordinal modify(Ordinal mask, Ordinal src, Ordinal srcDest) noexcept {
    return (src & mask) | (srcDest & ~mask);
}
constexpr Ordinal rotate(Ordinal src, Ordinal length) noexcept {
    return (src << length)  | (src >> ((-length) & 31u));
}
#endif //SIM3_TYPES_H
