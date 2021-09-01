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
// Created by jwscoggins on 8/20/21.
//
#ifdef DESKTOP_BUILD
#include "DesktopSBCore.h"
#include <iostream>

ShortOrdinal
DesktopSBCore::loadShortAligned(Address destination) {
    if (inIOSpace(destination)) {
        switch (destination & 0x00FF'FFFF) {
            case 0: // console flush
                std::cout.flush();
                break;
            case 2: // console available
                return 1;
            case 4: // console available for write
                return 1;
            case 6:
                return static_cast<ShortOrdinal>(std::cin.get());
            default:
                break;
        }
        return 0;
    } else {
        // okay so it is aligned, we can easily just do a lookup at this point
        // compute the correct offset
        auto offset = (destination >> 1) & 0b1;
        auto index = destination >> 2;
        return memory_[index].getShortOrdinal(offset);
    }
}
void
DesktopSBCore::storeShortAligned(Address destination, ShortOrdinal value) {
    if (inIOSpace(destination)) {
        switch (destination & 0x00FF'FFFF) {
            case 0: // console flush
            std::cout.flush();
            break;
            case 6: // console io
            std::cout.put(value);
            break;
            default:
                break;
        }
    } else if (inRAMArea(destination)) {
        // implicitly aligned
        auto offset = (destination >> 1) & 0b1;
        auto index = destination >> 2;
        memory_[index].setShortOrdinal(value, offset);
    } else {
        // do nothing
    }
}
ByteOrdinal
DesktopSBCore::loadByte(Address destination) {
    if (inIOSpace(destination)) {
        return 0;
    } else if (inRAMArea(destination)) {
        return memory_[destination >> 2].getByteOrdinal(destination & 0b11);
    } else if (inIACSpace(destination)) {
        return 0;
    } else {
        return 0;
    }
}
void
DesktopSBCore::storeByte(Address destination, ByteOrdinal value) {
    if (inRAMArea(destination)) {
        memory_[destination >> 2].setByteOrdinal(value, destination);
    } else if (inIOSpace(destination)) {
        // do nothing
    } else if (inIACSpace(destination)) {
        // do nothing
    } else {
        // do nothing
    }
}
Ordinal
DesktopSBCore::loadAligned(Address address) {
// get target thing
    if (inRAMArea(address)) {
        return memory_[address >> 2].getOrdinalValue();
    } else if (inIACSpace(address)) {
// we use IAC space as a hack to "map" in all of our peripherals for this custom core
        switch (address & 0x00FF'FFFF) {
            case HaltRegisterOffset:
                haltExecution();
                break;
            case ConsoleRegisterOffset: // Serial read / write
                return static_cast<Ordinal>(std::cin.get());
                break;
            case ConsoleFlushOffset:
                std::cout.flush();
                break;
            default:
                break;
        }
    }
    return 0;
}

void
DesktopSBCore::storeAligned(Address address, Ordinal value) {
    if (inRAMArea(address)) {
        memory_[address >> 2].setOrdinalValue(value);
    } else if (inIACSpace(address)) {
        switch (address & 0x00FF'FFFF) {
            case HaltRegisterOffset:
                haltExecution();
                break;
            case ConsoleRegisterOffset: // serial console input output
                std::cout.put(static_cast<char>(value));
                break;
            case ConsoleFlushOffset:
                std::cout.flush();
                break;
            default:
                break;
        }
    }
}
void
DesktopSBCore::generateFault(FaultType ) {
    std::cout << "FAULT GENERATED AT 0x" << std::hex << ip_.getOrdinal() << "! HALTING!" << std::endl;
    haltExecution();
}

void
DesktopSBCore::clearMemory() noexcept {
    for (size_t i = 0; i < NumCells; ++i) {
        memory_[i].clear();
    }
}
void
DesktopSBCore::installToMemory(Address loc, Ordinal value) {
    auto alignedAddress = (RAMMask & loc) >> 2;
    memory_[alignedAddress].setOrdinalValue(value);
}
void
DesktopSBCore::installToMemory(Address loc, ByteOrdinal value) {
    auto alignedAddress = (RAMMask & loc) >> 2;
    auto offset = loc & 0b11;
    memory_[alignedAddress].setByteOrdinal(value, offset);
}
void
DesktopSBCore::installBlockToMemory(Address base, Ordinal curr) noexcept  {
    installToMemory(base, curr);
}

DesktopSBCore::DesktopSBCore() : Parent(), memory_(std::make_unique<MemoryCell32[]>(NumCells)) {

}
#endif