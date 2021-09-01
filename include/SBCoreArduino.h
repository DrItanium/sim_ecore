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

#ifndef SIM3_SBCOREARDUINO_H
#define SIM3_SBCOREARDUINO_H
#ifdef ARDUINO
#include "Types.h"
#include "SimplifiedSxCore.h"

class SBCoreArduino : public SimplifiedSxCore {
public:
    using Parent = SimplifiedSxCore;
    using Parent::Parent;
    static constexpr Address HaltRegisterOffset = 0x00FF'FFFC;
    static constexpr Address ConsoleRegisterOffset = 0x00E0'0000;
    static constexpr Address ConsoleFlushOffset = 0x00E0'0004;
    static constexpr Address IACBaseAddress = 0x0000'0010;
    SBCoreArduino();
    ~SBCoreArduino() override = default;
    virtual void begin();
protected:
    ShortOrdinal loadShortAligned(Address destination) override;
    void storeShortAligned(Address destination, ShortOrdinal value) override;
    ByteOrdinal loadByte(Address destination) override;
    void storeByte(Address destination, ByteOrdinal value) override;
    Ordinal loadAligned(Address address) override;
    void storeAligned(Address address, Ordinal value) override;
    void generateFault(FaultType ) override;
protected:
    virtual ByteOrdinal ioSpaceLoad(Address address, TreatAsByteOrdinal) = 0;
    virtual ShortOrdinal ioSpaceLoad(Address address, TreatAsShortOrdinal) = 0;
    virtual Ordinal ioSpaceLoad(Address address, TreatAsOrdinal) = 0;

    virtual void ioSpaceStore(Address address, ByteOrdinal value) = 0;
    virtual void ioSpaceStore(Address address, ShortOrdinal value) = 0;
    virtual void ioSpaceStore(Address address, Ordinal value) = 0;

    virtual ByteOrdinal doIACLoad(Address address, TreatAsByteOrdinal) = 0;
    virtual ShortOrdinal doIACLoad(Address address, TreatAsShortOrdinal) = 0;
    virtual Ordinal doIACLoad(Address address, TreatAsOrdinal) = 0;

    virtual void doIACStore(Address address, ByteOrdinal value) = 0;
    virtual void doIACStore(Address address, ShortOrdinal value) = 0;
    virtual void doIACStore(Address address, Ordinal value) = 0;

    virtual ByteOrdinal doRAMLoad(Address address, TreatAsByteOrdinal) = 0;
    virtual ShortOrdinal doRAMLoad(Address address, TreatAsShortOrdinal) = 0;
    virtual Ordinal doRAMLoad(Address address, TreatAsOrdinal) = 0;
    virtual void doRAMStore(Address address, ByteOrdinal value) = 0;
    virtual void doRAMStore(Address address, ShortOrdinal value) = 0;
    virtual void doRAMStore(Address address, Ordinal value) = 0;

    virtual bool inIOSpace(Address target) noexcept {
        return target >= 0xFE00'0000 && !inIACSpace(target);
    }
    virtual Address toIOSpaceOffset(Address target)  noexcept {
        return toIACSpaceOffset(target);
    }
    static constexpr bool inIACSpace(Address target) noexcept {
        return target >= 0xFF00'0000;
    }
    static constexpr Address toIACSpaceOffset(Address target) noexcept {
        return target & (~0xFF00'0000);
    }
    virtual bool inRAMArea(Address target) noexcept = 0;
    /**
     * @brief Convert the full 32-bit address into an offset into the ram area
     * @param target the raw 32-bit address
     * @return the raw 32-bit address now as an offset into ram
     */
    virtual Address toRAMOffset(Address target) noexcept = 0;
};

#endif
#endif //SIM3_SBCOREARDUINO_H
