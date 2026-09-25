// expect: [od-scope] member 'k' of layer class 'K' is declared but not defined
// a CLOSED class used as a layer gets a Part carrying its body: a member defined out of line (in a .cpp) cannot be carried
struct T {};
struct K {int k() const;};
int K::k() const {return 4;}
struct Z : K:final T {};
