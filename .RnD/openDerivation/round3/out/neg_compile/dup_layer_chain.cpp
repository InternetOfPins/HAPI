// expect: duplicate layer in Y
// the same layer twice in one chain: rejected by the compiler (hapi::Distinct)
#include <hapi/rules.h>
struct Term {int get() const {return 0;}};
struct Self {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int get() const {return Base::get();}};};
struct Y : hapi::Chain<Self,Self>::Part<Term> {using Base=hapi::Chain<Self,Self>::Part<Term>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<Self,Self,Term>>, "duplicate layer in Y");};
