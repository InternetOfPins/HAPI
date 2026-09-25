// What the struct Cell is, next to the hapi::APIOf alias it replaces (built against out/include, before the originals):
// `struct Cell : (OO : ... : Bias<k> : final API) {}` is closed by APIOf itself, so it IS-A that APIOf, with its Types (API first)
#include "waveCell.h"
#include <type_traits>
using C = wave::Cell<3, wave::Threshold, wave::Wave<0,false,0,0,255>>;
using A = hapi::APIOf<wave::API, wave::Threshold, wave::Wave<0,false,0,0,255>, wave::Bias<3>>;
static_assert(!std::is_same<C, A>::value && std::is_base_of<A, C>::value, "its own type, derived from exactly that APIOf");
static_assert(std::is_same<C::Types, A::Types>::value && std::is_same<C::Types, hapi::Chain<wave::API, wave::Threshold, wave::Wave<0,false,0,0,255>, wave::Bias<3>>>::value,
              "the same Types as APIOf, API first");
static_assert(std::is_same<hapi::Expand<C>::Children, hapi::Expand<A>::Children>::value && hapi::Validates<C>::value && !hapi::HasOwnRules<C>::value,
              "an XXXDef: hapi::Expand / HasOwnRules forward to that APIOf's, so HAPI's walks treat it as they treat the APIOf");
int main(){}
