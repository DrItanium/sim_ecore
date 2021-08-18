//
// Created by jwscoggins on 8/18/21.
//
#include "SimplifiedSxCore.h"
#include <array>
#include <iostream>
#include <memory>
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
    static constexpr size_t MemorySize = 64_MB / sizeof(MemoryCell);
    ZCT_Core() : Parent(), memory_(std::make_unique<MemoryCell[]>(MemorySize)) {}
    ~ZCT_Core() override = default;
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
            auto alignedAddress = address >> 2;
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
                case 0:
                    haltExecution();
                    break;
                case 4: // Serial read / write
                    return static_cast<Ordinal>(std::cin.get());
                case 8:
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
            auto alignedAddress = address >> 2;
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
                case 0:
                    std::cout << "CALLED HALT EXECUTION!" << std::endl;
                    haltExecution();
                    break;
                case 4: // serial console input output
                    std::cout.put(static_cast<char>(value));
                    break;
                case 8:
                    std::cout.flush();
                    break;

                default:
                    // do nothing
                    break;
            }
        }
    }
    void generateFault(FaultType ) override {
        std::cout << "FAULT GENERATED! HALTING!" << std::endl;
        haltExecution();
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
    std::unique_ptr<MemoryCell[]> memory_;
};
void standardHaltState() {
    ZCT_Core core;
    core.clearMemory(); // make doubly sure
    // install the imi for testing purposes
    core.installBlockToMemory(0, 0x0000'0000, // sat_address
                              0x0000'00b0, // prcb_ptr
                              0x0000'0000, // check word
                              0x0000'032c, // start ip
                              0xffff'fc24 /* cs1 */);
    core.installToMemory(0x1c, 0xffff'ffff);
    core.installBlockToMemory(0x78, 0x0000'0180,
                              0x3040'00fb);
    core.installBlockToMemory(0x88, 0u,
                              0x00fc'00fb);
    core.installBlockToMemory(0x98, 0x0000'0180,
                              0x304000fb);
    core.installBlockToMemory(0xa8, 0x0000'0200u,
                              0x304000fbu);
    core.installBlockToMemory(0xb0, 0x0,
                              0xc);
    core.installBlockToMemory(0xc4,
                              0x6c0,
                              0x810500,
                              0,
                              0x1ff,
                              0x27f,
                              0x500);
    core.installBlockToMemory(0x18c, 0x00820501);
    // now just install our simple three line program into memory to test execution
    core.installBlockToMemory(0x32c, 0x5cf0'1e00, // mov 0, g14
                              0x8c80'3000, 0xff00'0000, // lda 0xFF00'0000, g0
                              0x92f4'2000 /* st g14, 0(g0) */);
    core.run();
}
void printSomeStuffToTheScreen() {
    ZCT_Core core;
    core.clearMemory(); // make doubly sure
    // install the imi for testing purposes
    core.installBlockToMemory(0, 0x0000'0000, // sat_address
                              0x0000'00b0, // prcb_ptr
                              0x0000'0000, // check word
                              0x0000'032c, // start ip
                              0xffff'fc24 /* cs1 */);
    core.installToMemory(0x1c, 0xffff'ffff);
    core.installBlockToMemory(0x78, 0x0000'0180,
                              0x3040'00fb);
    core.installBlockToMemory(0x88, 0u,
                              0x00fc'00fb);
    core.installBlockToMemory(0x98, 0x0000'0180,
                              0x304000fb);
    core.installBlockToMemory(0xa8, 0x0000'0200u,
                              0x304000fbu);
    core.installBlockToMemory(0xb0, 0x0,
                              0xc);
    core.installBlockToMemory(0xc4,
                              0x6c0,
                              0x810500,
                              0,
                              0x1ff,
                              0x27f,
                              0x500);
    core.installBlockToMemory(0x18c, 0x00820501);
    // now just install our simple three line program into memory to test execution
    core.installBlockToMemory(0x32c,
                              0x5cf0'1e00, // mov 0, g14
                              0x8c80'3000, 0xff00'0004, // lda 0xFF00'0004, g0
                              0x8cf0'0061, // lda 'a', g14
                              0x92f4'2000, // st g14, 0(g0)
                              0x8cf0'0062, // lda 'b', g14
                              0x92f4'2000, // st g14, 0(g0)
                              0x5cf0'1e00, // mov 0, g14
                              0x8c80'3000, 0xff00'0008, // lda 0xFF00'0008, g0
                              0x92f4'2000, /* st g14, 0(g0) */
                              0x5cf0'1e00, // mov 0, g14
                              0x8c80'3000, 0xff00'0000, // lda 0xFF00'0000, g0
                              0x92f4'2000 /* st g14, 0(g0) */);
    core.run();

}
int main(int /*argc*/, char** /*argv*/) {
    standardHaltState();
    printSomeStuffToTheScreen();
    return 0;
}
