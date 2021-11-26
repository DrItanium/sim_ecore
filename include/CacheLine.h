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
/**
 * @brief Readonly view of a cache address
 */
template<typename C, byte numOffsetBits, byte numTagBits, byte totalBits = 32>
union CacheAddress {
    // Unlike the i960Sx chipset, this version of the cache address is broken up into several parts to make
    // it easy to drill down into each individual cell.
    static constexpr auto NumGroupBits = bitsNeeded(sizeof(C));
    static constexpr auto NumTotalOffsetBits = numOffsetBits;
    static constexpr auto NumOffsetBits = NumTotalOffsetBits - NumGroupBits;
    static constexpr auto NumTagBits = numTagBits;
    static constexpr auto TotalBits = totalBits;
    static constexpr auto NumRestBits = (TotalBits - (NumTagBits + NumOffsetBits + NumGroupBits));
    using CellOffsetType = Address;
    using OffsetType = Address;
    using TagType = Address;
    using RestType = Address;
    using Self = CacheAddress<C, numOffsetBits, numTagBits, totalBits>;
    constexpr explicit CacheAddress(Address target = 0) noexcept : value_(target) { }
    constexpr CacheAddress(RestType r, TagType t, OffsetType o = 0, CellOffsetType c = 0) noexcept : cellOffset(c), offset(o), tag(t), rest(r) { }
    [[nodiscard]] constexpr auto getOriginalAddress() const noexcept { return value_; }
    [[nodiscard]] constexpr auto getOffset() const noexcept { return offset; }
    [[nodiscard]] constexpr auto getTag() const noexcept { return tag; }
    [[nodiscard]] constexpr auto getRest() const noexcept { return rest; }
    [[nodiscard]] constexpr auto getCellOffset(TreatAsByteOrdinal) const noexcept { return cellOffset; }
    [[nodiscard]] constexpr auto getCellOffset(TreatAsShortOrdinal) const noexcept { return cellOffset >> 1; }
    [[nodiscard]] constexpr auto getCellOffset(TreatAsOrdinal) const noexcept { return cellOffset >> 2; }
    [[nodiscard]] constexpr auto restEqual(const Self& other) const noexcept { return other.getRest() == getRest(); }
    void setOffset(OffsetType v) noexcept { offset = v; }
    void setTag(TagType v) noexcept { tag = v; }
    void setRest(RestType v) noexcept { rest = v; }
    void setCellOffset(CellOffsetType v) noexcept { cellOffset = v; }
    [[nodiscard]] constexpr Self aligned() const noexcept { return Self{rest, tag, 0, 0}; }
    void clear() noexcept { value_ = 0; }
private:
    Address value_ = 0;
    struct
    {
        CellOffsetType cellOffset: NumGroupBits;
        OffsetType offset: NumOffsetBits;
        TagType tag: NumTagBits;
        RestType rest: NumRestBits;
    };
} __attribute__((packed));
/**
 * @brief A grand central m4 specific cache line
 */
template<typename C, byte numOffsetBits,  byte numTagBits, byte totalNumberOfBits = 32>
struct CacheLine {
public:
    using CellType = C;
    using CacheAddress = ::CacheAddress<CellType, numOffsetBits, numTagBits, totalNumberOfBits>;
    static constexpr auto NumBytesPerCacheLine = bytesNeeded(numOffsetBits);
    static constexpr auto NumCellsPerCacheLine = NumBytesPerCacheLine / sizeof(CellType);
    static constexpr auto Mask = NumBytesPerCacheLine - 1;
    static constexpr auto NumBitsForCacheLineOffset = bitsNeeded(NumBytesPerCacheLine);
    static constexpr auto NumBitsForCellIndex = bitsNeeded(NumCellsPerCacheLine);
    static constexpr auto NumBitsForCellOffset = bitsNeeded(sizeof(CellType));
    static_assert(NumBitsForCacheLineOffset == (NumBitsForCellIndex + NumBitsForCellOffset), "Cell offset + Cell index should equal the total offset in an address!");
public:
    [[nodiscard]] TreatAsOrdinal::UnderlyingType get(const CacheAddress& targetAddress, TreatAsOrdinal) const noexcept {
        return storage_[targetAddress.getOffset()].getOrdinalValue();
    }
    [[nodiscard]] TreatAsShortOrdinal::UnderlyingType get(const CacheAddress& targetAddress, TreatAsShortOrdinal) const noexcept {
        return storage_[targetAddress.getOffset()].getShortOrdinal(targetAddress.getCellOffset(TreatAsShortOrdinal{}));
    }
    [[nodiscard]] TreatAsByteOrdinal::UnderlyingType get(const CacheAddress& targetAddress, TreatAsByteOrdinal) const noexcept {
        return storage_[targetAddress.getOffset()].getByteOrdinal(targetAddress.getCellOffset(TreatAsByteOrdinal{}));
    }
    void set(const CacheAddress& targetAddress, Ordinal value) {
        dirty_ = true;
        storage_[targetAddress.getOffset()].setByteOrdinal(value, targetAddress.getCellOffset(TreatAsOrdinal{}));
    }
    void set(const CacheAddress& targetAddress, ShortOrdinal value) {
        dirty_ = true;
        storage_[targetAddress.getOffset()].setByteOrdinal(value, targetAddress.getCellOffset(TreatAsShortOrdinal {}));
    }
    void set(const CacheAddress& targetAddress, ByteOrdinal value) {
        dirty_ = true;
        storage_[targetAddress.getOffset()].setByteOrdinal(value, targetAddress.getCellOffset(TreatAsByteOrdinal{}));
    }
    [[nodiscard]] constexpr bool valid() const noexcept { return valid_; }
    [[nodiscard]] constexpr bool matches(const CacheAddress& other) const noexcept { return address_.restEqual(other); }
    /**
     * @brief Returns true if the cache line is valid and flagged as dirty
     * @return true if the cache line is valid and the dirty flag has been set
     */
    [[nodiscard]] constexpr bool dirty() const noexcept { return dirty_; }
    void reset(const CacheAddress& newAddress, MemoryThing &newThing) {
        // okay this is the most complex part of the implementation
        // we need to do the following:
        // 1. If the cache line is dirty (and also valid), then commit it back to its backing storage
        // 2. compute the new base address using the new address
        // 3. use the new backing storage to load a cache line's worth of data into storage from teh new backing storage
        // 4. mark the line as clean and valid
        if (valid() && dirty()) {
            // okay so we have a valid dirty cache line, lets save it back to the underlying storage
            (void)backingStorage_->write(address_.getOriginalAddress(), reinterpret_cast<byte*>(storage_), sizeof(storage_));
            /// @todo check and see if we were able to write everything back to the underlying storage
            // at this point we've written back to the old backing storage
        }
        valid_ = true;
        dirty_ = false;
        address_ = newAddress.aligned();
        backingStorage_ = &newThing;
        (void)backingStorage_->read(address_.getOriginalAddress(), reinterpret_cast<byte*>(storage_), sizeof(storage_));
        /// @todo check and see if we were able to read a full cache line from underlying storage

    }
    void clear() noexcept {
        for (auto& entry : storage_) {
            entry.clear();
        }
        dirty_ = false;
        valid_ = false;
        backingStorage_ = nullptr;
        address_.clear();
    }
private:
    CellType storage_[NumCellsPerCacheLine];
    CacheAddress address_{0};
    MemoryThing* backingStorage_ = nullptr;
    bool dirty_ = false;
    bool valid_ = false;
    /// @todo pull the range commit data from the i960Sx to improve performance
};
template<typename C, byte numOffsetBits, byte numTagBits, byte totalNumberOfBits = 32>
struct DirectMappedCacheWay {
public:
    using Line = CacheLine<C, numOffsetBits, numTagBits, totalNumberOfBits>;
    using CacheAddress = typename Line::CacheAddress;
    Line& getCacheLine(const CacheAddress& target, MemoryThing& backingMemoryThing) noexcept {
       if (!way_.matches(target))  {
           way_.reset(target, backingMemoryThing);
       }
       return way_;
    }
    void clear() {
        way_.clear();
    }
private:
    Line way_;
};
template<template<typename, auto, auto, auto> typename W, typename C, size_t numLines, size_t numBytesPerLine>
struct Cache {
public:
    static constexpr auto NumLines = numLines;
    static constexpr auto NumBitsForCacheLineIndex = bitsNeeded(NumLines);
    static constexpr auto NumBitsPerLine = bitsNeeded(numBytesPerLine);
    using CacheWay = W<C, NumBitsPerLine, NumBitsForCacheLineIndex, 32>;
    using CacheLine = typename CacheWay::Line;
    using CacheAddress = typename CacheLine::CacheAddress;
    static constexpr auto CacheSize = NumLines * sizeof(CacheLine);
public:
public:
    auto&
    getCacheLine(const CacheAddress& target, MemoryThing& backingMemoryThing) noexcept {
        // okay we need to find out which cache line this current address targets
        return ways_[target.getTag()].getCacheLine(target, backingMemoryThing);
    }
    [[nodiscard]] byte* asTransferCache() noexcept { return reinterpret_cast<byte*>(ways_); }
    /**
     * @brief Go through and forcefully zero out all cache lines without saving anything
     */
    void clear() noexcept {
        for (auto& a : ways_) {
            a.clear();
        }
    }
private:
    CacheWay ways_[NumLines];
};

#endif //SIM3_CACHEENTRY_H
