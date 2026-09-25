// chains in base clauses: several operands, an access specifier next to an ordinary base, a pack fold
#include "common.h"
struct Term  {int v=1; int get() const {return v;}};
struct Other {int o=100;};
struct Inc   {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int get() const {return 1+Base::get();}};};
struct Dbl   {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int get() const {return 2*Base::get();}};};

struct Z  : hapi::APIOf<Term,Inc,Dbl> {using Base=hapi::APIOf<Term,Inc,Dbl>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<Inc,Dbl,Term>>, "duplicate layer in Z");}; using Z_APIOf=hapi::APIOf<Term,Inc,Dbl>;  namespace hapi { template<> struct Expand<Z> : Expand<Z_APIOf> {}; template<> struct HasOwnRules<Z> : HasOwnRules<Z_APIOf> {}; }
struct Z2 : public hapi::APIOf<Term,Inc>, Other {using Base=hapi::APIOf<Term,Inc>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<Inc,Term>>, "duplicate layer in Z2");}; using Z2_APIOf=hapi::APIOf<Term,Inc>;  namespace hapi { template<> struct Expand<Z2> : Expand<Z2_APIOf> {}; template<> struct HasOwnRules<Z2> : HasOwnRules<Z2_APIOf> {}; }
template<typename... PP> struct ZF : hapi::APIOf<Term,PP...> {using Base=hapi::APIOf<Term,PP...>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<PP...,Term>>, "duplicate layer in ZF");}; template<typename... PP> using ZF_APIOf=hapi::APIOf<Term,PP...>;  namespace hapi { template<typename... PP> struct Expand<ZF<PP...>> : Expand<ZF_APIOf<PP...>> {}; template<typename... PP> struct HasOwnRules<ZF<PP...>> : HasOwnRules<ZF_APIOf<PP...>> {}; }
template<typename... PP> struct ZG : hapi::APIOf<Term,PP...,Dbl> {using Base=hapi::APIOf<Term,PP...,Dbl>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<PP...,Dbl,Term>>, "duplicate layer in ZG");}; template<typename... PP> using ZG_APIOf=hapi::APIOf<Term,PP...,Dbl>;  namespace hapi { template<typename... PP> struct Expand<ZG<PP...>> : Expand<ZG_APIOf<PP...>> {}; template<typename... PP> struct HasOwnRules<ZG<PP...>> : HasOwnRules<ZG_APIOf<PP...>> {}; }

int main() {
  Z z; Z2 z2; ZF<Dbl,Inc> zf; ZF<> ze; ZG<Inc> zg;
  CHECK(z.get()==3);                 // 1+2*1
  CHECK(z2.get()==2 && z2.o==100);
  CHECK(zf.get()==4);                // 2*(1+1); ZF<Inc,Dbl,Inc> is now a duplicate layer (hapi::Distinct)
  CHECK(ze.get()==1);                // empty pack: the terminal itself
  CHECK(zg.get()==3);
  static_assert(std::is_base_of<Term,Z>::value && std::is_base_of<Other,Z2>::value, "bases");
  DONE("base_chain");
}
