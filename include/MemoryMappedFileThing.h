//
// Created by jwscoggins on 8/22/21.
//

#ifndef SIM3_MEMORYMAPPEDFILETHING_H
#define SIM3_MEMORYMAPPEDFILETHING_H
#ifdef ARDUINO
#include <Arduino.h>
#include <SdFat.h>
#include "MemoryThing.h"
class MemoryMappedFileThing : public MemoryThing {
public:
    MemoryMappedFileThing(Address baseAddress, Address addressSize, Address maximumFileSize, const char* path, decltype(FILE_WRITE) permissions) : MemoryThing(baseAddress, addressSize), maxSize_(maximumFileSize), path_(path), permissions_(permissions) { }
    ~MemoryMappedFileThing() override = default;
    void begin() override;
    Address getEndAddress() const noexcept override { return fileSize_; }

protected:
    size_t blockWrite(Address baseAddress, byte *buf, size_t amount) noexcept override;
    size_t blockRead(Address baseAddress, byte *buf, size_t amount) noexcept override;
private:
    Address maxSize_;
    const char* path_;
    decltype(FILE_WRITE) permissions_;
    Address fileSize_;
    File backingStorage_;
};
#elif defined(DESKTOP_BUILD)
/// @todo implement here
#endif
#endif //SIM3_MEMORYMAPPEDFILETHING_H
