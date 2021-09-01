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
#ifdef ARDUINO
#include "SBCoreArduino.h"
SBCoreArduino::SBCoreArduino() : Parent() {

}
void
SBCoreArduino::generateFault(FaultType ) {
    Serial.print(F("FAULT GENERATED AT 0x"));
    Serial.print(ip_.getOrdinal(), HEX);
    Serial.println(F("! HALTING!!"));
    haltExecution();
}


void
SBCoreArduino::storeAligned(Address destination, Ordinal value) {
    if (inRAMArea(destination)) {
        doRAMStore(toRAMOffset(destination), value);
    } else if (inIOSpace(destination)) {
        ioSpaceStore(toIOSpaceOffset(destination), value);
    } else if (inIACSpace(destination)) {
        doIACStore(toIACSpaceOffset(destination), value);
    } else {
        // do nothing
    }
}

Ordinal
SBCoreArduino::loadAligned(Address address) {
    if (inRAMArea(address)) {
        return doRAMLoad(toRAMOffset(address), TreatAsOrdinal {});
    } else if (inIOSpace(address)) {
        return ioSpaceLoad(toIOSpaceOffset(address), TreatAsOrdinal{});
    } else if (inIACSpace(address)) {
        return doIACLoad(toIACSpaceOffset(address), TreatAsOrdinal{});
    } else {
        return 0;
    }
}

ByteOrdinal
SBCoreArduino::loadByte(Address destination) {
    if (inRAMArea(destination)) {
        return doRAMLoad(toRAMOffset(destination), TreatAsByteOrdinal {});
    } else if (inIOSpace(destination)) {
        return ioSpaceLoad(toIOSpaceOffset(destination), TreatAsByteOrdinal{});
    } else if (inIACSpace(destination)) {
        return doIACLoad(toIACSpaceOffset(destination), TreatAsByteOrdinal{});
    } else {
        return 0;
    }
}

void
SBCoreArduino::storeShortAligned(Address destination, ShortOrdinal value) {
    if (inRAMArea(destination)) {
        doRAMStore(toRAMOffset(destination), value);
    } else if (inIOSpace(destination)) {
        ioSpaceStore(toIOSpaceOffset(destination), value);
    } else if (inIACSpace(destination)) {
        doIACStore(toIACSpaceOffset(destination), value);
    } else {
        // do nothing
    }
}

ShortOrdinal
SBCoreArduino::loadShortAligned(Address destination) {
    if (inRAMArea(destination)) {
        return doRAMLoad(toRAMOffset(destination), TreatAsShortOrdinal {});
    } else if (inIOSpace(destination)) {
        return ioSpaceLoad(toIOSpaceOffset(destination), TreatAsShortOrdinal{});
    } else if (inIACSpace(destination)) {
        return doIACLoad(toIACSpaceOffset(destination), TreatAsShortOrdinal{});
    } else {
        return 0;
    }
}

void
SBCoreArduino::begin() {
    // by default do nothing
}

void
SBCoreArduino::storeByte(Address destination, ByteOrdinal value) {
    if (inRAMArea(destination)) {
        doRAMStore(toRAMOffset(destination), value);
    } else if (inIOSpace(destination)) {
        ioSpaceStore(toIOSpaceOffset(destination), value);
    } else if (inIACSpace(destination)) {
        doIACStore(toIACSpaceOffset(destination), value);
    } else {
        // do nothing
    }
}

#endif

