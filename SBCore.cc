//
// Created by jwscoggins on 8/20/21.
//
#include "SBCore.h"

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
