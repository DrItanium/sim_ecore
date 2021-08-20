//
// Created by jwscoggins on 8/20/21.
//
#include "SBCore.h"
#include <iostream>

ShortOrdinal
SBCore::loadShort(Address destination) {
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
void
SBCore::storeShort(Address destination, ShortOrdinal value) {
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
    } else if (inRAMArea(destination)) {
        auto& cell = memory_[destination >> 2];
        auto offset = destination & 0b11;
        switch (offset) {
            case 0b00:
                cell.ordinalShorts[0] = value;
                break;
            case 0b10:
                cell.ordinalShorts[1] = value;
                break;
            case 0b01: // unaligned store
                cell.ordinalBytes[1] = static_cast<ByteOrdinal>(value);
                cell.ordinalBytes[2] = static_cast<ByteOrdinal>(value >> 8);
                break;
            case 0b11: // access the next element
                [&cell, &cell2 = memory_[((destination >> 2) + 1)], value]() {
                    cell.ordinalBytes[3] = static_cast<ByteOrdinal>(value);
                    cell2.ordinalBytes[0] = static_cast<ByteOrdinal>(value >> 8);
                }();
                break;
            default:
                break;
        }
    } else {
        // do nothing
    }
}
ByteOrdinal
SBCore::loadByte(Address destination) {
    if (inIOSpace(destination)) {
        return 0;
    } else if (inRAMArea(destination)) {
        auto& cell = memory_[destination >> 2];
        auto offset = destination & 0b11;
        return cell.ordinalBytes[offset];
    } else if (inIACSpace(destination)) {
        return 0;
    } else {
        return 0;
    }
}
Ordinal
SBCore::load(Address address) {
// get target thing
//std::cerr << "LOAD: 0x" << std::hex << address << " yielded 0x" << std::hex;
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
//std::cerr << result << "!" << std::endl;
    return result;
}

void
SBCore::store(Address address, Ordinal value) {
//std::cerr << "STORE 0x" << std::hex << value << " to 0x" << std::hex << address << std::endl;
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
                [this, &cell, alignedAddress, &temp]() {
// we have to store the lower 24 bits into memory
                    auto& cell2 = memory_[alignedAddress + 1];
                    cell.ordinalBytes[1] = temp.ordinalBytes[0];
                    cell.ordinalBytes[2] = temp.ordinalBytes[1];
                    cell.ordinalBytes[3] = temp.ordinalBytes[2];
                    cell2.ordinalBytes[0] = temp.ordinalBytes[3];
                }();
                break;
            case 0b10: // lower is in this cell, upper is in the next cell
                [this, &cell, alignedAddress, &temp]() {
                    auto& cell2 = memory_[alignedAddress + 1];
                    cell.ordinalBytes[2] = temp.ordinalBytes[0];
                    cell.ordinalBytes[3] = temp.ordinalBytes[1];
                    cell2.ordinalBytes[0] = temp.ordinalBytes[2];
                    cell2.ordinalBytes[1] = temp.ordinalBytes[3];
                }();
                break;
            case 0b11: // lower 24-bits next word, upper 8 bits, this word
                [this, &cell, alignedAddress, &temp]() {
                    auto& cell2 = memory_[alignedAddress + 1];
                    cell.ordinalBytes[3] = temp.ordinalBytes[0];
                    cell2.ordinalBytes[0] = temp.ordinalBytes[1];
                    cell2.ordinalBytes[1] = temp.ordinalBytes[2];
                    cell2.ordinalBytes[2] = temp.ordinalBytes[3];
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
void
SBCore::generateFault(FaultType ) {
    std::cout << "FAULT GENERATED AT 0x" << std::hex << ip_.getOrdinal() << "! HALTING!" << std::endl;
    haltExecution();
}
