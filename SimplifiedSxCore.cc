//
// Created by jwscoggins on 8/18/21.
//

#include "SimplifiedSxCore.h"

void
SimplifiedSxCore::boot() {
    if (!initialized_) {
        initialized_ = true;
        auto q = loadQuad(0); // load four words starting at address 0
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
SimplifiedSxCore::storeByte(Core::Address destination, ByteOrdinal value) {

}
void
SimplifiedSxCore::storeShort(Core::Address destination, ShortOrdinal value) {

}
void
SimplifiedSxCore::storeLong(Core::Address destination, LongOrdinal value) {

}
void
SimplifiedSxCore::atomicStore(Core::Address destination, Ordinal value) {

}
void
SimplifiedSxCore::store(Core::Address destination, Ordinal value) {

}
void
SimplifiedSxCore::store(Core::Address destination, const TripleRegister &reg) {

}
void
SimplifiedSxCore::store(Core::Address destination, const QuadRegister &reg) {

}
void
SimplifiedSxCore::storeShortInteger(Core::Address destination, ShortInteger value) {

}
void
SimplifiedSxCore::storeByteInteger(Core::Address destination, ByteInteger value) {

}
Ordinal
SimplifiedSxCore::load(Core::Address destination) {
    return 0;
}
Ordinal
SimplifiedSxCore::atomicLoad(Core::Address destination) {
    return 0;
}
ByteOrdinal
SimplifiedSxCore::loadByte(Core::Address destination) {
    return 0;
}
ShortOrdinal
SimplifiedSxCore::loadShort(Core::Address destination) {
    return 0;
}
LongOrdinal
SimplifiedSxCore::loadLong(Core::Address destination) {
    return 0;
}
void
SimplifiedSxCore::load(Core::Address destination, TripleRegister &reg) noexcept {

}
void
SimplifiedSxCore::load(Core::Address destination, QuadRegister &reg) noexcept {

}
