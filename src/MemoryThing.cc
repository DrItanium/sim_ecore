//
// Created by jwscoggins on 8/22/21.
//
#include "MemoryThing.h"

MemoryThing::MemoryThing(Address baseAddress, Address size) : base_(baseAddress), end_(baseAddress + size), size_(size < ) { }

bool
MemoryThing::respondsTo(Address input) const noexcept {
    return input < end_ && input >= base_;
}

Address
MemoryThing::translateAddress(Address input) const noexcept {
    return input - base_;
}

