// expect: [od-rule8]
struct Y {int f() const {return 0;}};
struct A : Y {int f() const {return 1+super::f();}};
