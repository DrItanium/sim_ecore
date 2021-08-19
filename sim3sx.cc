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
// Created by jwscoggins on 8/18/21.
//
#include <iostream>
#include <memory>
#include <fstream>
#include <iomanip>
#include "SimplifiedSxCore.h"

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
class SBCore : public SimplifiedSxCore {
public:
    using Parent = SimplifiedSxCore;
    using Parent::Parent;
    static constexpr Address HaltRegisterOffset = 0x00FF'FFFC;
    static constexpr Address ConsoleRegisterOffset = 0x00E0'0000;
    static constexpr Address ConsoleFlushOffset = 0x00E0'0004;
    static constexpr Address IACBaseAddress = 0x0000'0010;
    static constexpr size_t MemorySize = 64_MB / sizeof(MemoryCell);
    SBCore() : Parent(), memory_(std::make_unique<MemoryCell[]>(MemorySize)) {}
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
    ShortOrdinal loadShort(Address destination) override {
        if (inIOSpace(destination)) {
            switch (destination & 0x00FF'FFFF) {
                case 0: // console flush
                    std::cout.flush();
                    break;
                case 2: // console available
                    return static_cast<ShortOrdinal>(std::cin.peek() != decltype(std::cin)::traits_type::eof() ? 1 : 0);
                case 4: // console available for write
                    // always available for writing
                    return 1;
                case 6:
                    return std::cin.get();
                default:
                    break;
            }
            return 0;
        } else {
            return Core::loadShort(destination);
        }
    }
    void storeShort(Address destination, ShortOrdinal value) override {
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
        } else {
            Core::storeShort(destination, value);
        }
    }
    ByteOrdinal loadByte(Address destination) override {
        if (inIOSpace(destination)) {
            return 0;
        } else {
            return Core::loadByte(destination);
        }
    }
    Ordinal load(Address address) override {
        // get target thing
        //std::cout << "LOAD: 0x" << std::hex << address << " yielded 0x" << std::hex;
        auto result = 0u;
        if (inRAMArea(address)) {
            auto alignedAddress = address >> 2;
            auto offset = address & 0b11;
            auto& cell = memory_[alignedAddress];
            switch (offset) {
                case 0b00: // ah... aligned :D
                    result = cell.raw;
                    break;
                case 0b01: // upper 24 bits of current cell + lowest 8 bits of next cell
                    result = [this, &cell, alignedAddress]() {
                        auto cell2 = static_cast<Ordinal>(memory_[alignedAddress+1].ordinalBytes[0]) << 24;
                        auto lowerPart = cell.raw >> 8;
                        return cell2 | lowerPart;
                    }();
                    break;
                case 0b10: // lower is in this cell, upper is in the next cell
                    result = [this, &cell, alignedAddress]() {
                        auto upperPart = static_cast<Ordinal>(memory_[alignedAddress+ 1].ordinalShorts[0]) << 16; // instant alignment
                        auto lowerPart = static_cast<Ordinal>(cell.ordinalShorts[1]);
                        return upperPart | lowerPart;
                    }();
                    break;
                case 0b11: // requires reading a second word... gross
                    result = [this, &cell, alignedAddress]() {
                        auto cell2 = (memory_[alignedAddress+ 1].raw) << 8; // instant alignment
                        auto lowerPart = static_cast<Ordinal>(cell.ordinalBytes[0b11]);
                        return cell2 | lowerPart;
                    }();
                    break;
                default:
                    throw "IMPOSSIBLE PATH";
            }
        } else if (inIACSpace(address)) {
            // we use IAC space as a hack to "map" in all of our peripherals for this custom core
            switch (address & 0x00FF'FFFF) {
                case HaltRegisterOffset:
                    haltExecution();
                    break;
                case ConsoleRegisterOffset: // Serial read / write
                    result = static_cast<Ordinal>(std::cin.get());
                    break;
                case ConsoleFlushOffset:
                    std::cout.flush();
                    break;
                default:
                    break;
            }
        }
        //std::cout << result << "!" << std::endl;
        return result;
    }

    void store(Address address, Ordinal value) override {
        //std::cout << "STORE 0x" << std::hex << value << " to 0x" << std::hex << address << std::endl;
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
                    // do nothing
                    break;
            }
        }
    }
    void generateFault(FaultType ) override {
        std::cout << "FAULT GENERATED AT 0x" << std::hex << ip_.getOrdinal() << "! HALTING!" << std::endl;
        haltExecution();
    }
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
    std::unique_ptr<MemoryCell[]> memory_;
};

int main(int argc, char** argv) {
    if (argc == 2) {
        std::ifstream inputFile(argv[1]) ;
        if (inputFile.is_open())  {
            // okay install
            SBCore theCore;
            theCore.clearMemory();
            Ordinal currentAddress = 0;
            std::cout << "Installing image from: " << std::quoted(argv[1]) << std::endl;
            while(inputFile.good()) {
                // install byte by byte into memory
                theCore.installToMemory(currentAddress, static_cast<ByteOrdinal>(inputFile.get()));
                ++currentAddress;
            }
            std::cout << "Image installation complete" << std::endl;
            // now that we have that setup run the core
            theCore.run();
            return 0;
        } else {
            std::cout << "The file " << std::quoted(argv[1]) << " could not be opened";
            return 1;
        }
    } else {
        std::cout << "sim3sx [filename]" << std::endl;
        return 1;
    }
}
