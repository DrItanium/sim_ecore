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
#include "SBCore.h"
/**
 * @brief A theoretical i960Sx derived core with 64 megabytes of built in ram.
 * Everything runs from ram in this implementation
 */
class ZCT_Core : public SBCore {
public:
    using Parent = SBCore;
    using Parent::Parent;
    static constexpr Address HaltRegisterOffset = 0x00FF'FFFC;
    static constexpr Address ConsoleRegisterOffset = 0x00E0'0000;
    static constexpr Address ConsoleFlushOffset = 0x00E0'0004;
    static constexpr size_t MemorySize = 64_MB / sizeof(MemoryCell);
    ~ZCT_Core() override = default;
protected:
    Ordinal load(Address address) override {
        if (inIACSpace(address)) {
            switch (address & 0x00FF'FFFF) {
                case HaltRegisterOffset:
                    haltExecution();
                    break;
                case ConsoleRegisterOffset: // Serial read / write
                    return static_cast<Ordinal>(std::cin.get());
                case ConsoleFlushOffset:
                    std::cout.flush();
                    break;
                default:
                    break;
            }
            return 0;
        } else {
            return Parent::load(address);
        }
    }
    void store(Address address, Ordinal value) override {
        if (inIACSpace(address))  {
            // first check and see if it is in the processor mapped area
            switch (address & 0x00FF'FFFF) {
                case HaltRegisterOffset:
                    haltExecution();
                    break;
                case ConsoleRegisterOffset: // serial console input output
                    std::cout.put(static_cast<char>(value));
                    break;
                case ConsoleFlushOffset:
                    std::cout.flush();
                    break;

                default:
                    // do nothing
                    break;
            }
        } else {
            Parent::store(address, value);
        }
    }

    void generateFault(FaultType ) override {
        std::cout << "FAULT GENERATED! HALTING!" << std::endl;
        haltExecution();
    }
private:
    static constexpr bool inIACSpace(Address target) noexcept {
        return target >= 0xFF00'0000;
    }
    static constexpr bool inRAMArea(Address target) noexcept {
        return target < 64_MB;
    }
private:
    // allocate a 128 megabyte memory storage buffer
    std::unique_ptr<MemoryCell[]> memory_;
};
void standardHaltState() {
    ZCT_Core core;
    core.clearMemory(); // make doubly sure
    // install the imi for testing purposes
    core.installBlockToMemory(0, 0x0000'0000, // sat_address
                              0x0000'00b0, // prcb_ptr
                              0x0000'0000, // check word
                              0x0000'032c, // start ip
                              0xffff'fc24 /* cs1 */);
    core.installToMemory(0x1c, 0xffff'ffff);
    core.installBlockToMemory(0x78, 0x0000'0180,
                              0x3040'00fb);
    core.installBlockToMemory(0x88, 0u,
                              0x00fc'00fb);
    core.installBlockToMemory(0x98, 0x0000'0180,
                              0x304000fb);
    core.installBlockToMemory(0xa8, 0x0000'0200u,
                              0x304000fbu);
    core.installBlockToMemory(0xb0, 0x0,
                              0xc);
    core.installBlockToMemory(0xc4,
                              0x6c0,
                              0x810500,
                              0,
                              0x1ff,
                              0x27f,
                              0x500);
    core.installBlockToMemory(0x18c, 0x00820501);
    // now just install our simple three line program into memory to test execution
    core.installBlockToMemory(0x32c, 0x5cf0'1e00, // mov 0, g14
                              0x8c80'3000, 0xffff'fffc, // lda HaltRegister, g0
                              0x92f4'2000 /* st g14, 0(g0) */);
    core.run();
}
void printSomeStuffToTheScreen() {
    ZCT_Core core;
    core.clearMemory(); // make doubly sure
    // install the imi for testing purposes
    core.installBlockToMemory(0, 0x0000'0000, // sat_address
                              0x0000'00b0, // prcb_ptr
                              0x0000'0000, // check word
                              0x0000'032c, // start ip
                              0xffff'fc24 /* cs1 */);
    core.installToMemory(0x1c, 0xffff'ffff);
    core.installBlockToMemory(0x78, 0x0000'0180,
                              0x3040'00fb);
    core.installBlockToMemory(0x88, 0u,
                              0x00fc'00fb);
    core.installBlockToMemory(0x98, 0x0000'0180,
                              0x304000fb);
    core.installBlockToMemory(0xa8, 0x0000'0200u,
                              0x304000fbu);
    core.installBlockToMemory(0xb0, 0x0,
                              0xc);
    core.installBlockToMemory(0xc4,
                              0x6c0,
                              0x810500,
                              0,
                              0x1ff,
                              0x27f,
                              0x500);
    core.installBlockToMemory(0x18c, 0x00820501);
    // now just install our simple three line program into memory to test execution
    core.installBlockToMemory(0x32c,
                              0x5cf0'1e00, // mov 0, g14
                              0x8c80'3000, 0xffe0'0000, // lda console_write_char, g0
                              0x8cf0'0061, // lda 'a', g14
                              0x92f4'2000, // st g14, 0(g0)
                              0x8cf0'0062, // lda 'b', g14
                              0x92f4'2000, // st g14, 0(g0)
                              0x5cf0'1e00, // mov 0, g14
                              0x8c80'3000, 0xffe0'0004, // lda consoleflush_register, g0
                              0x92f4'2000, /* st g14, 0(g0) */
                              0x5cf0'1e00, // mov 0, g14
                              0x8c80'3000, 0xffff'fffc, // lda HaltRegister, g0
                              0x92f4'2000 /* st g14, 0(g0) */);
    core.run();

}
int main(int /*argc*/, char** /*argv*/) {
    // two test cases to start off with
    standardHaltState();
    printSomeStuffToTheScreen();
    return 0;
}
