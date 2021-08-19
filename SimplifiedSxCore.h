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
#ifndef SIM3_SIMPLIFIEDSXCORE_H
#define SIM3_SIMPLIFIEDSXCORE_H
#include "Core.h"
class IACMessage {
public:
    explicit IACMessage(const QuadRegister& qr) noexcept :
            messageType_((qr.getOrdinal(0) & 0xFF00'0000) >> 24),
            field1_((qr.getOrdinal(0) & 0x00FF'0000) >> 16),
            field2_(qr.getOrdinal(0)),
            field3_(qr.getOrdinal(1)),
            field4_(qr.getOrdinal(2)),
            field5_(qr.getOrdinal(3)) { }
    constexpr uint8_t getMessageType() const noexcept { return messageType_; }
    constexpr uint8_t getField1() const noexcept { return field1_; }
    constexpr uint16_t getField2() const noexcept { return field2_; }
    constexpr uint32_t getField3() const noexcept { return field3_; }
    constexpr uint32_t getField4() const noexcept { return field4_; }
    constexpr uint32_t getField5() const noexcept { return field5_; }
private:
    uint8_t messageType_;
    uint8_t field1_;
    uint16_t field2_;
    uint32_t field3_;
    uint32_t field4_;
    uint32_t field5_;
};
class SimplifiedSxCore : public Core
{
public:
    SimplifiedSxCore() : Core(4) { }
    ~SimplifiedSxCore() override = default;
protected:
    void boot() override;
    Ordinal getSystemAddressTableBase() const noexcept override;
    Ordinal getPRCBPtrBase() const noexcept override;
    bool continueToExecute() const noexcept override;
    constexpr bool initialized() const noexcept { return initialized_; }
    void resetExecutionStatus() noexcept override;
    void haltExecution() noexcept { executing_ = false; }
    void synchronizedStore(Address destination, const DoubleRegister &value) noexcept override;
    void synchronizedStore(Address destination, const QuadRegister &value) noexcept override;
    void synchronizedStore(Address destination, const Register &value) noexcept override;
protected:
    virtual void processIACMessage(const IACMessage& message) noexcept;
    virtual void boot0(Ordinal sat, Ordinal pcb, Ordinal startIP);
private:
    Ordinal systemAddressTableBase_ = 0;
    Ordinal prcbBase_ = 0;
    bool executing_ = false;
    bool initialized_ = false;
};

#endif //SIM3_SIMPLIFIEDSXCORE_H
