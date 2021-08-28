//
// Created by jwscoggins on 8/28/21.
//

#ifndef SIM3_PSRAMCHIP_H
#define SIM3_PSRAMCHIP_H
#include <Arduino.h>
#include <SPI.h>
#include "MemoryThing.h"

template<int pin>
class PSRAMChip : public MemoryThing
{
public:
    static constexpr auto EnablePin = pin;
public:
    explicit PSRAMChip(Address baseAddress) noexcept : MemoryThing(baseAddress, baseAddress + 8_MB) { }
    ~PSRAMChip() override = default;
    void begin() override {
        pinMode(EnablePin, OUTPUT);
        digitalWrite(EnablePin, HIGH);
        SPI.begin();
        delayMicroseconds(200); // give the psram chip time to come up
        digitalWrite(EnablePin, LOW);
        SPI.transfer(0x66);
        digitalWrite(EnablePin, HIGH);
        digitalWrite(EnablePin, LOW);
        SPI.transfer(0x99);
        digitalWrite(EnablePin, HIGH);
    }
protected:
    size_t blockWrite(Address baseAddress, uint8_t *buf, size_t amount) noexcept override {
        digitalWrite(EnablePin, LOW);
        SPI.transfer(0x02);
        SPI.transfer(baseAddress >> 16);
        SPI.transfer(baseAddress >> 8);
        SPI.transfer(baseAddress);
        SPI.transfer(buf, amount);
        digitalWrite(EnablePin, HIGH);
        return amount;
    }
    size_t blockRead(Address baseAddress, uint8_t *buf, size_t amount) noexcept override {
        digitalWrite(EnablePin, LOW);
        SPI.transfer(0x03);
        SPI.transfer(baseAddress >> 16);
        SPI.transfer(baseAddress >> 8);
        SPI.transfer(baseAddress);
        SPI.transfer(buf, amount);
        digitalWrite(EnablePin, HIGH);
        return amount;
    }
};


#endif //SIM3_PSRAMCHIP_H
