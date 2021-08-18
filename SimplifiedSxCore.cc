//
// Created by jwscoggins on 8/18/21.
//

#include "SimplifiedSxCore.h"

void
SimplifiedSxCore::boot() {
    if (!initialized_) {
        initialized_ = true;
        auto q = loadQuad(0);
        systemAddressTableBase_ = q.getOrdinal(0);
        prcbBase_ = q.getOrdinal(1);
        // skip the check words
        ip_.setOrdinal(q.getOrdinal(3));
        executing_ = true;
        pc_.setPriority(31);
        pc_.setState(1); // needs to be set as interrupted
        getRegister(RegisterIndex::FP).setOrdinal(getInterruptStackPointer());
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
