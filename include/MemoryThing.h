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
#include "Types.h"

class MemoryThing {
public:
    MemoryThing(Address baseAddress, Address size);
    MemoryThing() = default;
    virtual ~MemoryThing();
    virtual size_t write(Address baseAddress, byte* buf, size_t amount) noexcept = 0;
    virtual size_t read(Address baseAddress, byte* buf, size_t amount) noexcept = 0;
    constexpr auto getStartAddress() const noexcept { return base_; }
    constexpr auto getEndAddress() const noexcept { return end_; }
    constexpr auto size() const noexcept { return size_; }
    virtual Address translateAddress(Address input) const noexcept;
    virtual bool respondsTo(Address input) const noexcept;
private:
    Address base_;
    Address end_;
    Address size_;
    Address mask_;
};

#endif //SIM3_MEMORYTHING_H
