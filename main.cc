//
// Created by jwscoggins on 8/18/21.
//
#include "SimplifiedSxCore.h"
union SplitWord64 {
    explicit constexpr SplitWord64(LongOrdinal value = 0) noexcept : raw(value) { }

    LongOrdinal raw;
#define X(type, name) type name [ sizeof(raw) / sizeof(type)]
    X(Ordinal, ordinals);
    X(Integer, integers);
    X(ShortOrdinal, ordinalShorts);
    X(ShortInteger, integerShorts);
    X(ByteOrdinal, ordinalBytes);
    X(ByteInteger, integerBytes);
#undef X
};
static_assert(sizeof(SplitWord64) == sizeof(LongOrdinal), "SplitWord64 must be the same size as a long ordinal");
/**
 * @brief A theoretical i960Sx derived core with 64 megabytes of built in ram.
 * Everything runs from ram in this implementation
 */
class ZCT_Core : public SimplifiedSxCore {
public:

private:
    // allocate a 128 megabyte memory storage buffer
};
int main(int /*argc*/, char** /*argv*/) {
    //SimplifiedSxCore core;
    return 0;
}
