//
// Created by jwscoggins on 8/18/21.
//
#include "SimplifiedSxCore.h"
#include <array>
#include <iostream>
union MemoryCell {
    constexpr MemoryCell(Ordinal value = 0) noexcept : raw(value) { }

    Ordinal raw;
#define X(type, name) type name [ sizeof(raw) / sizeof(type)]
    X(ShortOrdinal, ordinalShorts);
    X(ShortInteger, integerShorts);
    X(ByteOrdinal, ordinalBytes);
    X(ByteInteger, integerBytes);
#undef X
};
static_assert(sizeof(MemoryCell) == sizeof(Ordinal), "MemoryCell must be the same size as a long ordinal");
/**
 * @brief A theoretical i960Sx derived core with 64 megabytes of built in ram.
 * Everything runs from ram in this implementation
 */
class ZCT_Core : public SimplifiedSxCore {
public:
    using Parent = SimplifiedSxCore;
    using Parent::Parent;
    ~ZCT_Core() override = default;
    void clearMemory() noexcept {
        for (auto& cell : memory_) {
            cell.raw = 0;
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
    template<typename ... Rest>
    void installBlockToMemory(Address base, Ordinal curr, Rest&& ... values) noexcept {
        installToMemory(base, curr);
        installBlockToMemory(base + 4, values...);
    }
    void installBlockToMemory(Address base, Ordinal curr) noexcept  {
        installToMemory(base, curr);
    }
protected:
    Ordinal load(Address address) override {
        // get target thing
        if (inRAMArea(address)) {
            auto alignedAddress = address & ~0b11;
            auto offset = address & 0b11;
            auto& cell = memory_[alignedAddress];
            switch (offset) {
                case 0b00: // ah... aligned :D
                    return cell.raw;
                case 0b01: // upper 24 bits of current cell + lowest 8 bits of next cell
                    return [this, &cell, address]() {
                        auto cell2 = static_cast<Ordinal>(memory_[address + 3].ordinalBytes[0]) << 24;
                        auto lowerPart = cell.raw >> 8;
                        return cell2 | lowerPart;
                    }();
                case 0b10: // lower is in this cell, upper is in the next cell
                    return [this, &cell, address]() {
                        auto upperPart = static_cast<Ordinal>(memory_[address + 2].ordinalShorts[0]) << 16; // instant alignment
                        auto lowerPart = static_cast<Ordinal>(cell.ordinalShorts[1]);
                        return upperPart | lowerPart;
                    }();
                case 0b11: // requires reading a second word... gross
                    return [this, &cell, address]() {
                        auto cell2 = (memory_[address + 1].raw) << 8; // instant alignment
                        auto lowerPart = static_cast<Ordinal>(cell.ordinalBytes[0b11]);
                        return cell2 | lowerPart;
                    }();
                default:
                    throw "IMPOSSIBLE PATH";
            }
        } else if (inIACSpace(address)) {
            // we use IAC space as a hack to "map" in all of our peripherals for this custom core
            switch (address & 0x00FF'FFFF) {
                case 0: // Serial read / write
                    return static_cast<Ordinal>(std::cin.get());
                case 4:
                    std::cout.flush();
                    break;
                default:
                    return 0;
            }
        }
        return 0;
    }

    void store(Address address, Ordinal value) override {
        if (inRAMArea(address)) {
            auto alignedAddress = address & ~0b11;
            auto offset = address & 0b11;
            auto& cell = memory_[alignedAddress];
            MemoryCell temp(value);
            switch (offset) {
                case 0b00: // ah... aligned :D
                    cell.raw = value;
                    break;
                case 0b01: // upper 24 bits of current cell + lowest 8 bits of next cell
                    memory_[address + 3].ordinalBytes[0] = temp.ordinalBytes[3];
                    cell.ordinalBytes[3] = temp.ordinalBytes[2];
                    cell.ordinalBytes[2] = temp.ordinalBytes[1];
                    cell.ordinalBytes[1] = temp.ordinalBytes[0];
                    // we have to store the lower 24 bits into memory
                    break;
                case 0b10: // lower is in this cell, upper is in the next cell
                    cell.ordinalShorts[1] = temp.ordinalShorts[0];
                    memory_[address + 3].ordinalShorts[0] = temp.ordinalShorts[1];
                    break;
                case 0b11: // lower 24-bits next word, upper 8 bits, this word
                    [this, &cell, address, &temp]() {
                        auto& cell2 = memory_[address + 1];
                        cell2.ordinalBytes[2] = temp.ordinalBytes[3];
                        cell2.ordinalBytes[1] = temp.ordinalBytes[2];
                        cell2.ordinalBytes[0] = temp.ordinalBytes[1];
                        cell.ordinalBytes[3] = temp.ordinalBytes[0];
                    }();
                    break;
                default:
                    throw "IMPOSSIBLE PATH";
            }
        } else if (inIACSpace(address)) {
            switch (address & 0x00FF'FFFF) {
                case 0: // serial console input output
                    std::cout.put(static_cast<char>(value));
                    break;
                case 4:
                    std::cout.flush();
                    break;
                default:
                    // do nothing
                    break;
            }
        }
    }
private:
    static constexpr bool inIACSpace(Address target) noexcept {
        return target >= 0xFF00'0000;
    }
    static constexpr bool inRAMArea(Address target) noexcept {
        return target < 64_MB;
    }
private:
    // allocate a 128 megabyte memory storage buffer
    std::array<MemoryCell, 64_MB / sizeof(MemoryCell)> memory_;
};
int main(int /*argc*/, char** /*argv*/) {
    ZCT_Core core;
    core.clearMemory(); // make doubly sure
    // install the imi for testing purposes
    core.installBlockToMemory(0, 0x0000'0000u,
                                 0x0000'00b0u,
                                 0x0000'0000u,
                                 0x0000'032cu,
                                 0xffff'fc24u);
    core.installToMemory(0x1c, 0xffff'ffff);
    core.installBlockToMemory(0x78, 0x0000'0180,
                              0x3040'00fb); // prcb_ptr
    core.installBlockToMemory(0x88, 0u,
                              0x00fc'00fb);
    core.installBlockToMemory(0x98, 0x0000'0180,
                              0x304000fb);
    core.installBlockToMemory(0xa8, 0x0000'0200,
                                    0x304000fb);
    return 0;
}
