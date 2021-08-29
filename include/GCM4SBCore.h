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

#ifndef SIM3_GCM4SBCORE_H
#define SIM3_GCM4SBCORE_H
#ifdef ARDUINO_GRAND_CENTRAL_M4
#include <Arduino.h>
#include <SPI.h>
#include <SdFat.h>
#include "Types.h"
#include "SBCoreArduino.h"
#include "MemoryMappedFileThing.h"
#include "PSRAMChip.h"
#define USE_PSRAM_CHIP

/**
 * @brief a version of the ArduinoSBCore meant for the grand central m4
 */
class GCM4SBCore : public SBCoreArduino {
public:
    /**
     * @brief A grand central m4 specific cache line
     */
    struct CacheLine {
        static constexpr auto NumBytesPerCacheLine = 64;
        static constexpr auto NumCellsPerCacheLine = NumBytesPerCacheLine / sizeof(MemoryCell32);
        static constexpr auto Mask = NumBytesPerCacheLine - 1;
        static constexpr auto NumBitsForCacheLineOffset = 6;
        static constexpr auto NumBitsForCellIndex = 4;
        static constexpr auto NumBitsForCellOffset = 2;
        static_assert(NumBitsForCacheLineOffset == (NumBitsForCellIndex + NumBitsForCellOffset), "Cell offset + Cell index should equal the total offset in an address!");
        constexpr CacheLine() noexcept : address_(0), dirty_(false) { }
    public:
        TreatAsOrdinal::UnderlyingType get(Address targetAddress, TreatAsOrdinal) const noexcept;
        TreatAsShortOrdinal::UnderlyingType get(Address targetAddress, TreatAsShortOrdinal) const noexcept;
        TreatAsByteOrdinal::UnderlyingType get(Address targetAddress, TreatAsByteOrdinal) const noexcept;
        void set(Address targetAddress, Ordinal value);
        void set(Address targetAddress, ShortOrdinal value);
        void set(Address targetAddress, ByteOrdinal value);
        static constexpr auto toCacheLineAddress(Address input) noexcept { return input & ~Mask; }
        static constexpr auto toCacheLineOffset(Address input) noexcept { return input & Mask; }
        constexpr bool valid() const noexcept { return valid_; }
        constexpr bool matches(Address other) const noexcept {
            return valid() && (toCacheLineAddress(other) == address_);
        }
        /**
         * @brief Returns true if the cache line is valid and flagged as dirty
         * @return true if the cache line is valid and the dirty flag has been set
         */
        constexpr bool dirty() const noexcept { return dirty_; }
        void reset(Address newAddress, MemoryThing& newThing);
        void clear() noexcept;
    private:
        MemoryCell32 storage_[NumCellsPerCacheLine] = { 0 };
        Address address_ = 0;
        MemoryThing* backingStorage_ = nullptr;
        bool dirty_ = false;
        bool valid_ = false;
    };

public:
    static constexpr Address RamSize = 8_MB;
    static constexpr Address RamStart = 0x0000'0000;
    static constexpr Address RamMask = RamSize - 1;
public:
    using Parent = SBCoreArduino;
    GCM4SBCore();
    ~GCM4SBCore() override;
    void begin() override;
protected:
    ByteOrdinal ioSpaceLoad(Address address, TreatAsByteOrdinal ordinal) override;
    ShortOrdinal ioSpaceLoad(Address address, TreatAsShortOrdinal ordinal) override;
    Ordinal ioSpaceLoad(Address address, TreatAsOrdinal ordinal) override;
    void ioSpaceStore(Address address, ByteOrdinal value) override;
    void ioSpaceStore(Address address, ShortOrdinal value) override;
    void ioSpaceStore(Address address, Ordinal value) override;
    ByteOrdinal doIACLoad(Address address, TreatAsByteOrdinal ordinal) override;
    ShortOrdinal doIACLoad(Address address, TreatAsShortOrdinal ordinal) override;
    Ordinal doIACLoad(Address address, TreatAsOrdinal ordinal) override;
    void doIACStore(Address address, ByteOrdinal value) override;
    void doIACStore(Address address, ShortOrdinal value) override;
    void doIACStore(Address address, Ordinal value) override;
    Ordinal doRAMLoad(Address address, TreatAsOrdinal ordinal) override;
    ShortOrdinal doRAMLoad(Address address, TreatAsShortOrdinal ordinal) override;
    ByteOrdinal doRAMLoad(Address address, TreatAsByteOrdinal ordinal) override;
    void doRAMStore(Address address, ByteOrdinal value) override;
    void doRAMStore(Address address, ShortOrdinal value) override;
    void doRAMStore(Address address, Ordinal value) override;
    bool inRAMArea(Address target) noexcept override;
    Address toRAMOffset(Address target) noexcept override;
private:
    CacheLine& getCacheLine(Address target) noexcept;
private:
    static constexpr auto NumCacheLines = 1024;
    static constexpr auto TransferCacheSize = NumCacheLines * sizeof(CacheLine);
    static constexpr auto NumBitsForCacheLineIndex = 10;
    /**
     * @brief Readonly view of a cache address
     */
    union CacheAddress {
        constexpr explicit CacheAddress(Address target) noexcept : value_(target) { }
        constexpr auto getOriginalAddress() const noexcept { return value_; }
        constexpr auto getTagAddress () const noexcept { return value_ & ~(CacheLine::Mask); }
        constexpr auto getCacheIndex() const noexcept { return index; }
        constexpr auto getCellIndex() const noexcept { return cellIndex; }
        constexpr auto getCellOffset(TreatAsByteOrdinal) const noexcept { return cellOffset; }
        constexpr auto getCellOffset(TreatAsShortOrdinal) const noexcept { return cellOffset >> 1; }
    private:
        Address value_;
        struct {
            Address cellOffset : CacheLine::NumBitsForCellOffset;
            Address cellIndex : CacheLine::NumBitsForCellIndex;
            Address index : NumBitsForCacheLineIndex;
            Address rest : (32 - (NumBitsForCacheLineIndex + CacheLine::NumBitsForCacheLineOffset));
        };
    };
#ifndef USE_PSRAM_CHIP
    using RAM = MemoryMappedFileThing;
#else
    using RAM = PSRAMChip<53, 5_MHz>;
#endif
private:
    RAM memoryImage_;
    // we have so much space available, let's have some fun with this
    union {
        byte transferCache[TransferCacheSize] = { 0 };
        CacheLine lines_[NumCacheLines];
    };
    // make space for the on chip request cache as well as the psram copy buffer
    // minimum size is going to be 8k or so (256 x 32) but for our current purposes we
    // are going to allocate a 4k buffer
};

using SBCore = GCM4SBCore;
#endif
#endif //SIM3_GCM4SBCORE_H

