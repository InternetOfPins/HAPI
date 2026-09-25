// expect: [od-scope] member 'f' of open class 'A' is declared but not defined
struct A {int f() const; int g() const {return super::g();}};
