//
// Created by jwscoggins on 8/28/21.
//

#ifndef SIM3_PSRAMCHIP_H
#define SIM3_PSRAMCHIP_H
#include "MemoryThing.h"

template<int pin>
class PSRAMChip : public MemoryThing
{
public:
    static constexpr auto EnablePin = pin;
public:
    explicit PSRAMChip(Address baseAddress) noexcept : MemoryThing(baseAddress, baseAddress + 8_MB) { }
    ~PSRAMChip() override { }
    Address getEndAddress() const noexcept override;
    Address translateAddress(Address input) const noexcept override;
    bool respondsTo(Address input) const noexcept override {

    }
    void begin() override {

    }
protected:
    size_t blockWrite(Address baseAddress, uint8_t *buf, size_t amount) noexcept override {

    }
    size_t blockRead(Address baseAddress, uint8_t *buf, size_t amount) noexcept override {

    }
};

template<int pin>
PSRAMChip<pin>::~PSRAMChip() {

}
template<int pin>
Address
PSRAMChip<pin>::getEndAddress() const noexcept {
    return MemoryThing::getEndAddress();
}
template<int pin>
Address
PSRAMChip<pin>::translateAddress(Address input) const noexcept {
    return MemoryThing::translateAddress(input);
}
template<int pin>
bool
PSRAMChip<pin>::respondsTo(Address input) const noexcept {
    return MemoryThing::respondsTo(input);
}
template<int pin>
void
PSRAMChip<pin>::begin() {
    MemoryThing::begin();
}
template<int pin>
size_t
PSRAMChip<pin>::blockWrite(Address baseAddress, uint8_t *buf, size_t amount) noexcept {
    return 0;
}
template<int pin>
size_t
PSRAMChip<pin>::blockRead(Address baseAddress, uint8_t *buf, size_t amount) noexcept {
    return 0;
}

#endif //SIM3_PSRAMCHIP_H
