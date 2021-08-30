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
/**
 * @brief A grand central m4 specific cache line
 */
template<typename C, size_t numBytes>
struct CacheLine {
    using CellType = C;
    static constexpr auto NumBytesPerCacheLine = numBytes;
    static constexpr auto NumCellsPerCacheLine = NumBytesPerCacheLine / sizeof(CellType);
    static constexpr auto Mask = NumBytesPerCacheLine - 1;
    static constexpr auto NumBitsForCacheLineOffset = bitsNeeded(NumBytesPerCacheLine);
    static constexpr auto NumBitsForCellIndex = bitsNeeded(NumCellsPerCacheLine);
    static constexpr auto NumBitsForCellOffset = bitsNeeded(sizeof(CellType));
    static_assert(NumBitsForCacheLineOffset == (NumBitsForCellIndex + NumBitsForCellOffset), "Cell offset + Cell index should equal the total offset in an address!");
    constexpr CacheLine() noexcept : address_(0), dirty_(false) { }
public:
    TreatAsOrdinal::UnderlyingType get(Address targetAddress, TreatAsOrdinal) const noexcept {
        // assume aligned
        CacheAddress addr(targetAddress);
        return storage_[addr.getCellIndex()].getOrdinalValue();
    }
    TreatAsShortOrdinal::UnderlyingType get(Address targetAddress, TreatAsShortOrdinal thingy) const noexcept {
        CacheAddress addr(targetAddress);
        return storage_[addr.getCellIndex()].getShortOrdinal(addr.getCellOffset(thingy));
    }
    TreatAsByteOrdinal::UnderlyingType CacheLine::get(Address targetAddress, TreatAsByteOrdinal thingy) const noexcept {
        CacheAddress addr(targetAddress);
        return storage_[addr.getCellIndex()].getByteOrdinal(addr.getCellOffset(thingy));
    }
    void
    set(Address targetAddress, Ordinal value) {
        // assume aligned
        CacheAddress addr(targetAddress);
        dirty_ = true;
        storage_[addr.getCellIndex()].setOrdinalValue(value);
    }
    void set(Address targetAddress, ShortOrdinal value) {
        // assume aligned
        CacheAddress addr(targetAddress);
        dirty_ = true;
        // just shift by one since we can safely assume it is aligned as that is the only way
        // to call this method through the memory system gestalt
        storage_[addr.getCellIndex()].setShortOrdinal(value, addr.getCellOffset(TreatAsShortOrdinal{}));
    }

    void set(Address targetAddress, ByteOrdinal value) {
        CacheAddress addr(targetAddress);
        dirty_ = true;
        storage_[addr.getCellIndex()].setByteOrdinal(value, addr.getCellOffset(TreatAsByteOrdinal{}));
    }
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
    void reset(Address newAddress, MemoryThing &newThing) {
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
    void clear() noexcept {
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
private:
    CellType storage_[NumCellsPerCacheLine] = { 0 };
    Address address_ = 0;
    MemoryThing* backingStorage_ = nullptr;
    bool dirty_ = false;
    bool valid_ = false;
};

#endif //SIM3_CACHEENTRY_H
