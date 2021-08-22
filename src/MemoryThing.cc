//
// Created by jwscoggins on 8/22/21.
//
#include "MemoryThing.h"

MemoryThing::MemoryThing(Address baseAddress, Address size) : base_(baseAddress), end_(baseAddress + size), size_(size) { }

bool
MemoryThing::respondsTo(Address input) const noexcept {
    return input < getEndAddress() && input >= getStartAddress();
}

Address
MemoryThing::translateAddress(Address input) const noexcept {
    return input - getStartAddress();
}


size_t
MemoryThing::read(Address baseAddress, byte *buf, size_t amount) noexcept {
    // translate the absolute address to a relative one and also figure how much of this we can actually store
    auto bufEndAddress = (baseAddress + amount);
    auto amountToRead = amount;
    if (bufEndAddress >= getEndAddress()) {
        // okay we have overage
        // compute how much we are actually going to read into the buffer
        amountToRead = amount - (bufEndAddress - getEndAddress());
    }

    return blockRead(translateAddress(baseAddress), buf, amountToRead);
}

size_t
MemoryThing::write(Address baseAddress, byte *buf, size_t amount) noexcept {
    // translate the absolute address to a relative one and also figure how much of this we can actually write
    auto bufEndAddress = (baseAddress + amount);
    auto amountToWrite = amount;
    if (bufEndAddress >= getEndAddress()) {
        // okay we have overage
        // compute how much we are actually going to read into the buffer
        amountToWrite = amount - (bufEndAddress - getEndAddress());
    }

    return blockWrite(translateAddress(baseAddress), buf, amountToWrite);
}

void
MemoryThing::begin() {
    // do nothing
}