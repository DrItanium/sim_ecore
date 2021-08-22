//
// Created by jwscoggins on 8/22/21.
//

#ifndef SIM3_MEMORYMAPPEDFILETHING_H
#define SIM3_MEMORYMAPPEDFILETHING_H
#ifdef ARDUINO
#include <Arduino.h>
#include "MemoryThing.h"

class MemoryMappedFileThing : public MemoryThing {
public:
    using MemoryThing::MemoryThing;
};
#elif defined(DESKTOP_BUILD)
/// @todo implement here
#endif
#endif //SIM3_MEMORYMAPPEDFILETHING_H
