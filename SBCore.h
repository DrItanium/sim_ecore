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
// Created by jwscoggins on 8/19/21.
//

#ifndef SIM3_SBCORE_H
#define SIM3_SBCORE_H
#include <memory>
#include <fstream>
#include <iomanip>
#include "Core.h"
#include "SimplifiedSxCore.h"

union MemoryCell32 {
    constexpr MemoryCell32(Ordinal value = 0) noexcept : raw(value) { }

    Ordinal raw;
#define X(type, name) type name [ sizeof(raw) / sizeof(type)]
    X(ShortOrdinal, ordinalShorts);
    X(ShortInteger, integerShorts);
    X(ByteOrdinal, ordinalBytes);
    X(ByteInteger, integerBytes);
#undef X
};
static_assert(sizeof(MemoryCell32) == sizeof(Ordinal), "MemoryCell32 must be the same size as a long ordinal");
/**
 * @brief A theoretical i960Sx derived core with 64 megabytes of built in ram.
 * Everything runs from ram in this implementation
 */
class SBCore : public SimplifiedSxCore {
public:
    using Parent = SimplifiedSxCore;
    using Parent::Parent;
    static constexpr Address HaltRegisterOffset = 0x00FF'FFFC;
    static constexpr Address ConsoleRegisterOffset = 0x00E0'0000;
    static constexpr Address ConsoleFlushOffset = 0x00E0'0004;
    static constexpr Address IACBaseAddress = 0x0000'0010;
    static constexpr size_t MemorySize = 64_MB / sizeof(MemoryCell32);
    SBCore() : Parent(), memory_(std::make_unique<MemoryCell32[]>(MemorySize)) {}
    ~SBCore() override = default;
    void clearMemory() noexcept {
        for (size_t i = 0; i < MemorySize; ++i) {
            memory_[i].raw = 0;
        }
    }
    /**
     * @brief Install an ordinal to a given memory address
     * @param loc
     * @param value
     */
    void installToMemory(Address loc, Ordinal value) {
        auto alignedAddress = ((64_MB -1) & loc) >> 2;
        memory_[alignedAddress].raw = value;
    }
    void installToMemory(Address loc, ByteOrdinal value) {
        auto alignedAddress = ((64_MB - 1) & loc) >> 2;
        auto offset = loc & 0b11;
        memory_[alignedAddress].ordinalBytes[offset] = value;
    }
    template<typename ... Rest>
    void installBlockToMemory(Address base, Ordinal curr, Rest&& ... values) noexcept {
        installToMemory(base, curr);
        installBlockToMemory(base + 4, values...);
    }
    void installBlockToMemory(Address base, Ordinal curr) noexcept  {
        installToMemory(base, curr);
    }
protected:
    ShortOrdinal loadShort(Address destination) override;
    void storeShort(Address destination, ShortOrdinal value) override;
    ByteOrdinal loadByte(Address destination) override;
    Ordinal load(Address address) override;
    void store(Address address, Ordinal value) override;
    void generateFault(FaultType ) override;
private:
    static constexpr bool inIOSpace(Address target) noexcept {
        return target >= 0xFE00'0000 && !inIACSpace(target);
    }
    static constexpr bool inIACSpace(Address target) noexcept {
        return target >= 0xFF00'0000;
    }
    static constexpr bool inRAMArea(Address target) noexcept {
        return target < 64_MB;
    }
private:
    // allocate a 128 megabyte memory storage buffer
    std::unique_ptr<MemoryCell32[]> memory_;
};
#endif //SIM3_SBCORE_H
