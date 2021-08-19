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
#include "SimplifiedSxCore.h"
#include <iostream>

void
SimplifiedSxCore::boot0(Ordinal sat, Ordinal pcb, Ordinal startIP) {
    systemAddressTableBase_ = sat;
    prcbBase_ = pcb;
    // skip the check words
    ip_.setOrdinal(startIP);
    executing_ = true;
    pc_.setPriority(31);
    pc_.setState(true); // needs to be set as interrupted
    getRegister(RegisterIndex::FP).setOrdinal(getInterruptStackPointer());
    advanceIPBy = 0; // make sure that we don't do anything dumb at this point
}
void
SimplifiedSxCore::boot() {
    if (!initialized_) {
        initialized_ = true;
        auto q = loadQuad(0);
        boot0(q.getOrdinal(0), q.getOrdinal(1), q.getOrdinal(3));
    }
}
Ordinal
SimplifiedSxCore::getSystemAddressTableBase() const noexcept {
    return systemAddressTableBase_;
}
Ordinal
SimplifiedSxCore::getPRCBPtrBase() const noexcept {
    return prcbBase_;
}
bool
SimplifiedSxCore::continueToExecute() const noexcept {
    return executing_;
}
void
SimplifiedSxCore::resetExecutionStatus() noexcept {
    executing_ = true;
}
void
SimplifiedSxCore::synchronizedStore(Core::Address destination, const DoubleRegister &value) noexcept {
    // no special IAC locations when dealing with long versions so cool beans
    store(destination, value.getLongOrdinal());
}
void
SimplifiedSxCore::synchronizedStore(Core::Address destination, const QuadRegister &value) noexcept {
    if (destination == 0xFF00'0010) {
        IACMessage msg(value);
        processIACMessage(msg);
        // there are special IAC messages we need to handle here
    } else {
        store(destination, value);
    }
}
void
SimplifiedSxCore::synchronizedStore(Core::Address destination, const Register &value) noexcept {
    // there is a lookup for an interrupt control register, in the Sx manual, we are going to ignore that for now
    store(destination, value.getOrdinal());
}

void
SimplifiedSxCore::processIACMessage(const IACMessage &message) noexcept {
    switch (message.getMessageType()) {
        case 0x89: // purge instruction cache
            // do nothing as we don't have an instruction cache
            break;
        case 0x93: // reinitialize processor
            std::cout << "REINITAILIZING PROCESSOR!" << std::endl;
            std::cout << "(reinit-processor 0x" <<  std::hex << message.getField3() <<
                      " 0x" << std::hex << message.getField4() <<
                      " 0x" << std::hex << message.getField5() << ")" << std::endl;
            boot0(message.getField3(), message.getField4(), message.getField5());
            break;
        case 0x8F:
            // set breakpoint register
            break;
        case 0x80:
            // store system base
            [this, &message]() {
                // stores the current locations of the system address table and the prcb in a specified location in memory.
                // The address of the system address table is stored in the word starting at the byte specified in field 3,
                // and the address of the PRCB is stored in the next word in memory (field 3 address plus 4)
                DoubleRegister pack(getSystemAddressTableBase(), getPRCBPtrBase());
                storeLong(message.getField3(), pack.getLongOrdinal());
            }();
            break;
        case 0x40: // interrupt
            // Generates an interrupt request. The interrup vector is given in field 1 of the IAC message. The processor handles the
            // interrupt request just as it does interrupts received from other sources. If the interrupt priority is higher than the prcessor's
            // current priority, the processor services the interrupt immediately. Otherwise, it posts the interrup in the pending interrupts
            // section of the interrupt table.
            break;
        case 0x41: // Test pending interrupts
            // tests for pending interrupts. The processor checks the pending interrupt section of the interrupt
            // table for a pending interrupt with a priority higher than the prcoessor's current priority. If a higher
            // priority interrupt is found, it is serviced immediately. Otherwise, no action is taken

            /// @todo implement this
            break;
        default:
            break;

    }
}
