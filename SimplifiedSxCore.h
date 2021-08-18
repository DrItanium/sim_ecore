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
    Ordinal getFirstIP() const noexcept override;
    bool continueToExecute() const noexcept override;
    void storeByte(Address destination, ByteOrdinal value) override;
    void storeShort(Address destination, ShortOrdinal value) override;
    void storeLong(Address destination, LongOrdinal value) override;
    void atomicStore(Address destination, Ordinal value) override;
    void store(Address destination, Ordinal value) override;
    void store(Address destination, const TripleRegister &reg) override;
    void store(Address destination, const QuadRegister &reg) override;
    void storeShortInteger(Address destination, ShortInteger value) override;
    void storeByteInteger(Address destination, ByteInteger value) override;
    Ordinal load(Address destination) override;
    Ordinal atomicLoad(Address destination) override;
    ByteOrdinal loadByte(Address destination) override;
    ShortOrdinal loadShort(Address destination) override;
    LongOrdinal loadLong(Address destination) override;
    void load(Address destination, TripleRegister &reg) noexcept override;
    void load(Address destination, QuadRegister &reg) noexcept override;
private:
    Ordinal systemAddressTableBase_ = 0;
    Ordinal prcbBase_ = 0;
    Ordinal startIP_ = 0;
    bool executing_ = false;
};

#endif //SIM3_SIMPLIFIEDSXCORE_H
