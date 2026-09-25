// expect: [od-scope] member of open class 'A' defined out of line
struct A {static int n; int g() const {return n+super::g();}};
int A::n = 1;
