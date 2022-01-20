//
// Created by jwscoggins on 8/22/21.
//

#ifdef ARDUINO
#ifndef I960_CPU_REPLACEMENT_CORE
#include "MemoryMappedFileThing.h"
extern SdFat SD;
void
MemoryMappedFileThing::begin() {
// okay now we need to open the file and such
    if (!SD.exists(const_cast<char*>(path_)) ) {
        Serial.print(F("Could not find file \""));
        Serial.print(path_);
        Serial.println("\"");
        while (true) { delay(1000); }
    }
    backingStorage_ = SD.open(path_, permissions_);
    if (!backingStorage_) {
        Serial.print(F("Could not open file \""));
        Serial.print(path_);
        Serial.println("\"");
        while (true) { delay(1000); }
    }
    fileSize_ = backingStorage_.size();
    if (fileSize_ > maxSize_) {
        Serial.println(F("File too large!"));
    }

}

size_t
MemoryMappedFileThing::blockRead(Address baseAddress, byte *buf, size_t amount) noexcept {
    backingStorage_.seek(baseAddress);
    return backingStorage_.readBytes(buf, amount);
}

size_t
MemoryMappedFileThing::blockWrite(Address baseAddress, byte *buf, size_t amount) noexcept {
    backingStorage_.seek(baseAddress);
    return backingStorage_.write(buf, amount);

}
#endif
#endif