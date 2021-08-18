//
// Created by jwscoggins on 8/18/21.
//

#ifndef SIM3_SIMPLIFIEDSXCORE_H
#define SIM3_SIMPLIFIEDSXCORE_H
#include "Core.h"
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
private:
    Ordinal systemAddressTableBase_ = 0;
    Ordinal prcbBase_ = 0;
    bool executing_ = false;
    bool initialized_ = false;
};

#endif //SIM3_SIMPLIFIEDSXCORE_H
