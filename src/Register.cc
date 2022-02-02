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
Ordinal operator&(const Register& src2, const Register& src1) noexcept {
    return src2.get<Ordinal>() & src1.get<Ordinal>();
}
Ordinal operator|(const Register& src2, const Register& src1) noexcept {
    return src2.get<Ordinal>() | src1.get<Ordinal>();
}
Ordinal operator^(const Register& src2, const Register& src1) noexcept {
    return src2.get<Ordinal>() ^ src1.get<Ordinal>();
}

Ordinal operator~(const Register& value) noexcept {
    return ~value.get<Ordinal>();
}
