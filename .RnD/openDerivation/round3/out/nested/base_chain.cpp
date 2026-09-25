// chains in base clauses: several operands, an access specifier next to an ordinary base, a pack fold
#include "common.h"
struct Term  {int v=1; int get() const {return v;}};
struct Other {int o=100;};
struct Inc   {template<typename O> struct Part:O {
 using Base=O; using Base::Base;int get() const {return 1+Base::get();}};};
struct Dbl   {template<typename O> struct Part:O {
 using Base=O; using Base::Base;int get() const {return 2*Base::get();}};};

struct Z  : Inc::Part<Dbl::Part<Term>> {};
struct Z2 : public Inc::Part<Term>, Other {};
template<typename... PP> struct ZF : od::FoldT<Term,PP...> {};
template<typename... PP> struct ZG : od::FoldT<Term,PP...,Dbl> {};

int main() {
  Z z; Z2 z2; ZF<Inc,Dbl,Inc> zf; ZF<> ze; ZG<Inc> zg;
  CHECK(z.get()==3);                 // 1+2*1
  CHECK(z2.get()==2 && z2.o==100);
  CHECK(zf.get()==5);                // 1+2*(1+1)
  CHECK(ze.get()==1);                // empty pack: the terminal itself
  CHECK(zg.get()==3);
  static_assert(std::is_base_of<Term,Z>::value && std::is_base_of<Other,Z2>::value, "bases");
  DONE("base_chain");
}
