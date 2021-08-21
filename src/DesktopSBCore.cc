//
// Created by jwscoggins on 8/20/21.
//
#ifdef DESKTOP_BUILD
#include "DesktopSBCore.h"
#include <iostream>

ShortOrdinal
DesktopSBCore::loadShort(Address destination) {
    if (inIOSpace(destination)) {
        switch (destination & 0x00FF'FFFF) {
            case 0: // console flush
                std::cout.flush();
                break;
            case 2: // console available
                return 1;
            case 4: // console available for write
                return 1;
            case 6:
                return static_cast<ShortOrdinal>(std::cin.get());
            default:
                break;
        }
        return 0;
    } else {
        return Core::loadShort(destination);
    }
}
void
DesktopSBCore::storeShort(Address destination, ShortOrdinal value) {
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
                cell.setShortOrdinal(value, 0);
                break;
            case 0b10:
                cell.setShortOrdinal(value, 1);
                break;
            case 0b01: // unaligned store
                cell.setByteOrdinal(value, 1);
                cell.setByteOrdinal(value >> 8, 2);
                break;
            case 0b11: // access the next element
                [&cell, &cell2 = memory_[((destination >> 2) + 1)], value]() {
                    cell.setByteOrdinal(value, 3);
                    cell2.setByteOrdinal(value >> 8, 0);
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
DesktopSBCore::loadByte(Address destination) {
    if (inIOSpace(destination)) {
        return 0;
    } else if (inRAMArea(destination)) {
        auto& cell = memory_[destination >> 2];
        auto offset = destination & 0b11;
        return cell.getByteOrdinal(offset);
    } else if (inIACSpace(destination)) {
        return 0;
    } else {
        return 0;
    }
}
Ordinal
DesktopSBCore::load(Address address) {
// get target thing
    auto result = 0u;
    if (inRAMArea(address)) {
        auto alignedAddress = address >> 2;
        auto offset = address & 0b11;
        auto& cell = memory_[alignedAddress];
        switch (offset) {
            case 0b00: // ah... aligned :D
                result = cell.getOrdinalValue();
                break;
            case 0b01: // upper 24 bits of current cell + lowest 8 bits of next cell
                result = [this, &cell, alignedAddress]() {
                    auto cell2 = static_cast<Ordinal>(memory_[alignedAddress+1].getByteOrdinal(0)) << 24;
                    auto lowerPart = cell.getOrdinalValue() >> 8;
                    return cell2 | lowerPart;
                }();
                break;
            case 0b10: // lower is in this cell, upper is in the next cell
                result = [this, &cell, alignedAddress]() {
                    auto upperPart = static_cast<Ordinal>(memory_[alignedAddress+ 1].getShortOrdinal(0)) << 16; // instant alignment
                    auto lowerPart = static_cast<Ordinal>(cell.getShortOrdinal(1));
                    return upperPart | lowerPart;
                }();
                break;
            case 0b11: // requires reading a second word... gross
                result = [this, &cell, alignedAddress]() {
                    auto cell2 = (memory_[alignedAddress+ 1].getOrdinalValue()) << 8; // instant alignment
                    auto lowerPart = static_cast<Ordinal>(cell.getByteOrdinal(0b11));
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
    return result;
}

void
DesktopSBCore::store(Address address, Ordinal value) {
    if (inRAMArea(address)) {
        auto alignedAddress = address >> 2;
        auto offset = address & 0b11;
        auto& cell = memory_[alignedAddress];
        MemoryCell32 temp(value);
        switch (offset) {
            case 0b00: // ah... aligned :D
                cell.setOrdinalValue(value);
                break;
            case 0b01: // upper 24 bits of current cell + lowest 8 bits of next cell
                [this, &cell, alignedAddress, &temp]() {
// we have to store the lower 24 bits into memory
                    auto& cell2 = memory_[alignedAddress + 1];
                    cell.setByteOrdinal(temp.getByteOrdinal(0), 1);
                    cell.setByteOrdinal(temp.getByteOrdinal(1), 2);
                    cell.setByteOrdinal(temp.getByteOrdinal(2), 3);
                    cell2.setByteOrdinal(temp.getByteOrdinal(3), 0);
                }();
                break;
            case 0b10: // lower is in this cell, upper is in the next cell
                [this, &cell, alignedAddress, &temp]() {
                    auto& cell2 = memory_[alignedAddress + 1];
                    cell.setByteOrdinal(temp.getByteOrdinal(0), 2);
                    cell.setByteOrdinal(temp.getByteOrdinal(1), 3);
                    cell2.setByteOrdinal(temp.getByteOrdinal(2), 0);
                    cell2.setByteOrdinal(temp.getByteOrdinal(3), 1);
                }();
                break;
            case 0b11: // lower 24-bits next word, upper 8 bits, this word
                [this, &cell, alignedAddress, &temp]() {
                    auto& cell2 = memory_[alignedAddress + 1];
                    cell.setByteOrdinal(temp.getByteOrdinal(0), 3);
                    cell2.setByteOrdinal(temp.getByteOrdinal(1), 0);
                    cell2.setByteOrdinal(temp.getByteOrdinal(2), 1);
                    cell2.setByteOrdinal(temp.getByteOrdinal(3), 2);
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
DesktopSBCore::generateFault(FaultType ) {
    std::cout << "FAULT GENERATED AT 0x" << std::hex << ip_.getOrdinal() << "! HALTING!" << std::endl;
    haltExecution();
}

void
DesktopSBCore::clearMemory() noexcept {
    for (size_t i = 0; i < MemorySize; ++i) {
        memory_[i].clear();
    }
}
void
DesktopSBCore::installToMemory(Address loc, Ordinal value) {
    auto alignedAddress = ((64_MB -1) & loc) >> 2;
    memory_[alignedAddress].setOrdinalValue(value);
}
void
DesktopSBCore::installToMemory(Address loc, ByteOrdinal value) {
    auto alignedAddress = ((64_MB - 1) & loc) >> 2;
    auto offset = loc & 0b11;
    memory_[alignedAddress].setByteOrdinal(value, offset);
}
void
DesktopSBCore::installBlockToMemory(Address base, Ordinal curr) noexcept  {
    installToMemory(base, curr);
}

DesktopSBCore::DesktopSBCore() : Parent(), memory_(std::make_unique<MemoryCell32[]>(MemorySize)) {

}
#endif