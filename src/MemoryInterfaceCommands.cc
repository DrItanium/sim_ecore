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
