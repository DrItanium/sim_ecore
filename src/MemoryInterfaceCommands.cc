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
#include "Core.h"
#include "Types.h"

/**
 * @brief Defines the start of the internal cache memory connected on the EBI, this is used by the microcontroller itself for whatever it needs (lower 32k)
 */
constexpr size_t CacheMemoryWindowStart = (RAMEND + 1);
/**
 * @brief Defines the start of the window used to peer out into the external bus itself
 */
constexpr size_t BusMemoryWindowStart = 0x8000;

ByteOrdinal
Core::loadByte(Address destination) {
    return 0;
}
void
Core::storeByte(Address destination, ByteOrdinal value) {

}
ShortOrdinal
Core::loadShortAligned(Address destination) {
    return 0;

}
void
Core::storeShortAligned(Address destination, ShortOrdinal value) {

}
Ordinal
Core::loadAligned(Address destination) {
    return 0;

}
void
Core::storeAligned(Address destination, Ordinal value) {

}

Ordinal
Core::load(Address destination) {
    if ((destination & 0b11) == 0) {
        // phew, things are aligned
        return loadAligned(destination);
    } else {
        // have to do this short by short as we could span cache lines or other such nonsense
        // we want to get 16-bit quantities out because it could turn out that the lsb is still zero and thus we would still be able to do
        // partially fast loads
        auto lower = static_cast<Ordinal>(loadShort(destination + 0));
        auto upper = static_cast<Ordinal>(loadShort(destination + 2)) << 16;
        return lower | upper;
    }
}

ShortOrdinal
Core::loadShort(Address destination) noexcept {
    if ((destination & 0b1) == 0) {
        // okay, it is aligned to 2-byte boundaries, we can call the aligned version of this function
        return loadShortAligned(destination);
    } else {
        // bad news, we are looking at an unaligned load so do byte by byte instead and then compose it together
        auto lower = static_cast<ShortOrdinal>(loadByte(destination + 0));
        auto upper = static_cast<ShortOrdinal>(loadByte(destination + 1)) << 8;
        return lower | upper;
    }
}

void
Core::store(Address destination, Ordinal value) {
    if ((destination & 0b11) == 0b00) {
        storeAligned(destination, value);
    } else {
        // store the upper and lower halves in separate requests
        storeShort(destination + 0, value);
        storeShort(destination + 2, value >> 16);
    }
}
void
Core::storeShort(Address destination, ShortOrdinal value) {
    if ((destination & 1) == 0) {
        // yay! aligned
        storeShortAligned(destination, value);
    } else {
        // store the components into memory
        storeByte(destination + 0, value)  ;
        storeByte(destination + 1, value >> 8);
    }
}
void
Core::storeLong(Address destination, LongOrdinal value) {
    DoubleRegister wrapper(value);
    store(destination + 0, wrapper.getOrdinal(0));
    store(destination + 4, wrapper.getOrdinal(1));
}
void
Core::store(Address destination, const TripleRegister& reg) {
    store(destination + 0, reg.getOrdinal(0));
    store(destination + 4, reg.getOrdinal(1));
    store(destination + 8, reg.getOrdinal(2));
}
void
Core::store(Address destination, const QuadRegister& reg) {
    store(destination + 0, reg.getOrdinal(0));
    store(destination + 4, reg.getOrdinal(1));
    store(destination + 8, reg.getOrdinal(2));
    store(destination + 12, reg.getOrdinal(3));
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
    auto lower = load(destination + 0);
    auto upper = load(destination + 4);
    auto outcome = DoubleRegister(lower, upper).getLongOrdinal();
    return outcome;
}
void
Core::load(Address destination, TripleRegister& reg) noexcept {
    reg.setOrdinal(load(destination + 0), 0);
    reg.setOrdinal(load(destination + 4), 1);
    reg.setOrdinal(load(destination + 8), 2);
}
void
Core::load(Address destination, QuadRegister& reg) noexcept {
    reg.setOrdinal(load(destination + 0), 0);
    reg.setOrdinal(load(destination + 4), 1);
    reg.setOrdinal(load(destination + 8), 2);
    reg.setOrdinal(load(destination + 12), 3);
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
    store(destination, value);
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
    store(destination, value.getOrdinal());
}
