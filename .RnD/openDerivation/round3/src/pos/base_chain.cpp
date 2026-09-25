// chains in base clauses: several operands, an access specifier next to an ordinary base, a pack fold
#include "common.h"
struct Term  {int v=1; int get() const {return v;}};
struct Other {int o=100;};
struct Inc   {int get() const {return 1+super::get();}};
struct Dbl   {int get() const {return 2*super::get();}};

struct Z  : Inc:Dbl:Term {};
struct Z2 : public Inc:Term, Other {};
template<typename... PP> struct ZF : (PP : ... : Term) {};
template<typename... PP> struct ZG : (PP : ... : Dbl : Term) {};

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
