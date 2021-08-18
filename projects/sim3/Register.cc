//
// Created by jwscoggins on 8/18/21.
//

#include "Register.h"

Ordinal
ArithmeticControls::modify(Ordinal mask, Ordinal src) noexcept {
    auto tmp = ord_;
    ord_ = ::modify(mask, src, ord_);
    return tmp;
}

Ordinal
ProcessControls::modify(Ordinal mask, Ordinal src) noexcept {
    auto tmp = ord_;
    ord_ = ::modify(mask, src, ord_);
    return tmp;
}

Ordinal
TraceControls::modify(Ordinal mask, Ordinal src) noexcept {
    auto tmp = ord_;
    ord_ = ::modify(mask, src, ord_);
    return tmp;
}
