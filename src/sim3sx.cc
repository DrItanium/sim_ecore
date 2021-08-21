// sim3
// Copyright (c) 2021, Joshua Scoggins
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Created by jwscoggins on 8/18/21.
//
#ifdef DESKTOP_BUILD
#include <iostream>
#include "SBCore.h"

int main(int argc, char** argv) {
    if (argc == 2) {
        std::ifstream inputFile(argv[1]) ;
        if (inputFile.is_open())  {
            // okay install
            SBCore theCore;
            theCore.clearMemory();
            Ordinal currentAddress = 0;
            std::cout << "Installing image from: " << std::quoted(argv[1]) << std::endl;
            while(inputFile.good()) {
                // install byte by byte into memory
                theCore.installToMemory(currentAddress, static_cast<ByteOrdinal>(inputFile.get()));
                ++currentAddress;
            }
            std::cout << "Image installation complete" << std::endl;
            inputFile.close();
            // now that we have that setup run the core
            theCore.run();
            return 0;
        } else {
            std::cout << "The file " << std::quoted(argv[1]) << " could not be opened";
            return 1;
        }
    } else {
        std::cout << "sim3sx [filename]" << std::endl;
        return 1;
    }
}
#elif defined(ARDUINO)
#include <Arduino.h>
#include "Types.h"

void setup() {

}
void loop() {

}
#endif
