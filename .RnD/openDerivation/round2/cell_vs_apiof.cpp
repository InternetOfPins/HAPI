// What the struct-only Cell is, next to the hapi::APIOf it replaces (built against out/include, before the originals)
#include "waveCell.h"
#include <type_traits>
using C = wave::Cell<3, wave::Threshold, wave::Wave<0,false,0,0,255>>;
using A = hapi::APIOf<wave::API, wave::Threshold, wave::Wave<0,false,0,0,255>, wave::Bias<3>>;
static_assert(std::is_same<C::Types, hapi::Chain<wave::Threshold, wave::Wave<0,false,0,0,255>, wave::Bias<3>>>::value, "struct Cell: Types without the API");
static_assert(std::is_same<A::Types, hapi::Chain<wave::API, wave::Threshold, wave::Wave<0,false,0,0,255>, wave::Bias<3>>>::value, "APIOf: API first");
static_assert(!std::is_same<C, A>::value && std::is_base_of<typename A::Base, C>::value, "different type, same base");
int main(){}
