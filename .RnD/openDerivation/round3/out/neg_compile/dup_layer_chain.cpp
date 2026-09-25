// expect: duplicate layer in Y
// the same layer twice in one chain: rejected by the compiler (hapi::Distinct)
#include <hapi/hapi.h>
struct Term {int get() const {return 0;}};
struct Self {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int get() const {return Base::get();}};};
struct Y : hapi::APIOf<Term,Self,Self> {using Base=hapi::APIOf<Term,Self,Self>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<Self,Self,Term>>, "duplicate layer in Y");}; using Y_APIOf=hapi::APIOf<Term,Self,Self>;  namespace hapi { template<> struct Expand< ::Y> : Expand< ::Y_APIOf> {}; template<> struct HasOwnRules< ::Y> : HasOwnRules< ::Y_APIOf> {}; }
