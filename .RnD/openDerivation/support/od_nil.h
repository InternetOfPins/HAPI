#pragma once
// od_nil.h -- the implicit empty terminal of translate.py: a chain whose rightmost operand is open (names `super`) ends in
// od::Nil, so `struct Z : A:B {};` with B open lowers to hapi::Chain<A,B>::Part<od::Nil>. A `super::f()` that reaches
// od::Nil is a compile error only when that member is used (rule 7: expressions naming `super` are dependent).
namespace od {
  struct Nil {};
}
