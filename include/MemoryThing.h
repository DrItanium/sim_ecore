//
// Created by jwscoggins on 8/22/21.
//

#ifndef SIM3_MEMORYTHING_H
#define SIM3_MEMORYTHING_H
#ifdef ARDUINO
#include <Arduino.h>
#elif defined(DESKTOP_BUILD)
#include <cstdint>
#include <cstddef>
#endif
#include "Types.h"

class MemoryThing {
public:
    MemoryThing(Address baseAddress, Address size);
    MemoryThing() = default;
    virtual ~MemoryThing();
    constexpr auto getStartAddress() const noexcept { return base_; }
    virtual Address getEndAddress() const noexcept { return end_; }
    constexpr auto size() const noexcept { return size_; }
    /**
     * @brief Converts an absolute address to one relative to this memory thing
     * @param input The aboslute address
     * @return The address relative to the start of this memory thing's mapping
     */
    virtual Address translateAddress(Address input) const noexcept;
    /**
     * @brief Does the given address fall within the mapping of this memory thing
     * @param input The address to check
     * @return True, if the input address is in this things memory space
     */
    virtual bool respondsTo(Address input) const noexcept;
    size_t write(Address baseAddress, uint8_t* buf, size_t amount) noexcept;
    size_t read(Address baseAddress, uint8_t* buf, size_t amount) noexcept;
    virtual void begin();
protected:
    virtual size_t blockWrite(Address baseAddress, uint8_t* buf, size_t amount) noexcept = 0;
    virtual size_t blockRead(Address baseAddress, uint8_t* buf, size_t amount) noexcept = 0;
private:
    Address base_;
    Address end_;
    Address size_;
};

#endif //SIM3_MEMORYTHING_H
