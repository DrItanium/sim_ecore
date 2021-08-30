//
// Created by jwscoggins on 8/30/21.
//

#ifndef SIM3_CACHEENTRY_H
#define SIM3_CACHEENTRY_H
#ifdef ARDUINO
#include <Arduino.h>
#else
#include <cstdint>
#endif
#include "MemoryThing.h"
#include "Types.h"
template<typename C, size_t numBytesPerLine, size_t numLines>
struct Cache {
public:
    using CellType = C;
    static constexpr auto NumBytesPerLine = numBytesPerLine;
    static constexpr auto NumLines = numLines;
    union CacheAddress;
/**
 * @brief A grand central m4 specific cache line
 */
    struct CacheLine {
    public:
        using CellType = C;
        static constexpr auto NumBytesPerCacheLine = numBytesPerLine;
        static constexpr auto NumCellsPerCacheLine = NumBytesPerCacheLine / sizeof(CellType);
        static constexpr auto Mask = NumBytesPerCacheLine - 1;
        static constexpr auto NumBitsForCacheLineOffset = bitsNeeded(NumBytesPerCacheLine);
        static constexpr auto NumBitsForCellIndex = bitsNeeded(NumCellsPerCacheLine);
        static constexpr auto NumBitsForCellOffset = bitsNeeded(sizeof(CellType));
        static_assert(NumBitsForCacheLineOffset == (NumBitsForCellIndex + NumBitsForCellOffset), "Cell offset + Cell index should equal the total offset in an address!");
        constexpr CacheLine() noexcept : address_(0), dirty_(false) { }
    public:
        [[nodiscard]] TreatAsOrdinal::UnderlyingType get(Address targetAddress, TreatAsOrdinal) const noexcept;
        [[nodiscard]] TreatAsShortOrdinal::UnderlyingType get(Address targetAddress, TreatAsShortOrdinal thingy) const noexcept;
        [[nodiscard]] TreatAsByteOrdinal::UnderlyingType get(Address targetAddress, TreatAsByteOrdinal thingy) const noexcept;
        void set(Address targetAddress, Ordinal value);
        void set(Address targetAddress, ShortOrdinal value);
        void set(Address targetAddress, ByteOrdinal value);
        static constexpr auto toCacheLineAddress(Address input) noexcept { return input & ~Mask; }
        static constexpr auto toCacheLineOffset(Address input) noexcept { return input & Mask; }
        [[nodiscard]] constexpr bool valid() const noexcept { return valid_; }
        [[nodiscard]] constexpr bool matches(Address other) const noexcept { return valid() && (toCacheLineAddress(other) == address_); }
        /**
         * @brief Returns true if the cache line is valid and flagged as dirty
         * @return true if the cache line is valid and the dirty flag has been set
         */
        [[nodiscard]] constexpr bool dirty() const noexcept { return dirty_; }
        void reset(Address newAddress, MemoryThing &newThing);
        void clear() noexcept;
    private:
        CellType storage_[NumCellsPerCacheLine] = { 0 };
        Address address_ = 0;
        MemoryThing* backingStorage_ = nullptr;
        bool dirty_ = false;
        bool valid_ = false;
    };
    static constexpr auto CacheSize = NumLines * sizeof(CellType);
    static constexpr auto NumBitsForCacheLineIndex = bitsNeeded(NumLines);
public:
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
public:
    CacheLine&
    getCacheLine(Address target, MemoryThing& backingMemoryThing) noexcept {
        CacheAddress addr(target);
        // okay we need to find out which cache line this current address targets
        auto& targetLine = lines_[addr.getCacheIndex()];
        if (!targetLine.matches(target)) {
            // right now we only have one thing "mapped" to the file cache
            targetLine.reset(target, backingMemoryThing);
        }
        return targetLine;
    }
    byte* asTransferCache() noexcept { return transferCache; }
    /**
     * @brief Go through and forcefully zero out all cache lines without saving anything
     */
    void clear() noexcept {
        for (auto& line : lines_) {
            line.clear();
        }
    }
    Cache() noexcept { }
private:
    union {
        byte transferCache[CacheSize] = { 0 };
        CacheLine lines_[NumLines];
    };

};
template<typename C, size_t a, size_t b>
TreatAsOrdinal::UnderlyingType
Cache<C,a,b>::CacheLine::get(Address targetAddress, TreatAsOrdinal) const noexcept {
    // assume aligned
    CacheAddress addr(targetAddress);
    return storage_[addr.getCellIndex()].getOrdinalValue();
}
template<typename C, size_t a, size_t b>
TreatAsShortOrdinal::UnderlyingType Cache<C,a,b>::CacheLine::get(Address targetAddress, TreatAsShortOrdinal thingy) const noexcept {
    CacheAddress addr(targetAddress);
    return storage_[addr.getCellIndex()].getShortOrdinal(addr.getCellOffset(thingy));
}
template<typename C, size_t a, size_t b>
TreatAsByteOrdinal::UnderlyingType
Cache<C,a,b>::CacheLine::get(Address targetAddress, TreatAsByteOrdinal thingy) const noexcept {
    CacheAddress addr(targetAddress);
    return storage_[addr.getCellIndex()].getByteOrdinal(addr.getCellOffset(thingy));
}

template<typename C, size_t a, size_t b>
void
Cache<C,a,b>::CacheLine::
set(Address targetAddress, Ordinal value) {
    // assume aligned
    CacheAddress addr(targetAddress);
    dirty_ = true;
    storage_[addr.getCellIndex()].setOrdinalValue(value);
}

template<typename C, size_t a, size_t b>
void
Cache<C,a,b>::CacheLine::
set(Address targetAddress, ShortOrdinal value) {
    // assume aligned
    CacheAddress addr(targetAddress);
    dirty_ = true;
    // just shift by one since we can safely assume it is aligned as that is the only way
    // to call this method through the memory system gestalt
    storage_[addr.getCellIndex()].setShortOrdinal(value, addr.getCellOffset(TreatAsShortOrdinal{}));
}

template<typename C, size_t a, size_t b>
void
Cache<C,a,b>::CacheLine::
set(Address targetAddress, ByteOrdinal value) {
    CacheAddress addr(targetAddress);
    dirty_ = true;
    storage_[addr.getCellIndex()].setByteOrdinal(value, addr.getCellOffset(TreatAsByteOrdinal{}));
}

template<typename C, size_t a, size_t b>
void
Cache<C,a,b>::CacheLine::
reset(Address newAddress, MemoryThing &newThing) {
    // okay this is the most complex part of the implementation
    // we need to do the following:
    // 1. If the cache line is dirty (and also valid), then commit it back to its backing storage
    // 2. compute the new base address using the new address
    // 3. use the new backing storage to load a cache line's worth of data into storage from teh new backing storage
    // 4. mark the line as clean and valid
    if (valid() && dirty()) {
        // okay so we have a valid dirty cache line, lets save it back to the underlying storage
        (void)backingStorage_->write(address_, reinterpret_cast<byte*>(storage_), sizeof(storage_));
        /// @todo check and see if we were able to write everything back to the underlying storage
        // at this point we've written back to the old backing storage
    }
    CacheAddress newAddr(newAddress);
    valid_ = true;
    dirty_ = false;
    address_ = newAddr.getTagAddress();
    backingStorage_ = &newThing;
    (void)backingStorage_->read(address_, reinterpret_cast<byte*>(storage_), sizeof(storage_));
    /// @todo check and see if we were able to read a full cache line from underlying storage
}
template<typename C, size_t a, size_t b>
void
Cache<C,a,b>::CacheLine::clear() noexcept {
    // calling clear means that you just want to do a hard clear without any saving of what is currently in the cache
    // We must do this after using part of the cacheline storage as a transfer buffer for setting up the live memory image
    // there will be garbage in memory here that can be interpreted as "legal" cache lines
    dirty_ = false;
    valid_ = false;
    backingStorage_ = nullptr;
    address_ = 0;
    for (auto& cell : storage_) {
        cell.setOrdinalValue(0);
    }
}
#endif //SIM3_CACHEENTRY_H
