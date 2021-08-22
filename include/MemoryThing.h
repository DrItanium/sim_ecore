//
// Created by jwscoggins on 8/22/21.
//

#ifndef SIM3_MEMORYTHING_H
#define SIM3_MEMORYTHING_H
#ifdef ARDUINO
#include <Arduino.h>
#else
#include <cstdint>
#endif

class MemoryThing {
public:
    MemoryThing() = default;
    virtual ~MemoryThing();
    virtual size_t write(Address baseAddress, byte* buf, size_t amount) noexcept = 0;
    virtual size_t read(Address baseAddress, byte* buf, size_t amount) noexcept = 0;
};

#endif //SIM3_MEMORYTHING_H
