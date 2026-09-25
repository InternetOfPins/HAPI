// chains in base clauses: several operands, an access specifier next to an ordinary base, a pack fold
#include "common.h"
struct Term  {int v=1; int get() const {return v;}};
struct Other {int o=100;};
struct Inc   {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int get() const {return 1+Base::get();}};};
struct Dbl   {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int get() const {return 2*Base::get();}};};

struct Z  : hapi::Chain<Inc,Dbl>::Part<Term> {using Base=hapi::Chain<Inc,Dbl>::Part<Term>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<Inc,Dbl,Term>>, "duplicate layer in Z");};
struct Z2 : public hapi::Chain<Inc>::Part<Term>, Other {using Base=hapi::Chain<Inc>::Part<Term>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<Inc,Term>>, "duplicate layer in Z2");};
template<typename... PP> struct ZF : hapi::Chain<PP...>::template Part<Term> {using Base=typename hapi::Chain<PP...>::template Part<Term>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<PP...,Term>>, "duplicate layer in ZF");};
template<typename... PP> struct ZG : hapi::Chain<PP...,Dbl>::template Part<Term> {using Base=typename hapi::Chain<PP...,Dbl>::template Part<Term>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<PP...,Dbl,Term>>, "duplicate layer in ZG");};

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
