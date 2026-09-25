// expect: duplicate layer in Y
// the same layer twice in one chain: rejected by the compiler (hapi::Distinct)
#include <hapi/rules.h>
struct Term {int get() const {return 0;}};
struct Self {int get() const {return super::get();}};
struct Y : Self:Self:Term {};
