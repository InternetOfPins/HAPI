// expect: [od-dup] duplicate layer: 'A' is composed over 'X', which already derives from 'A'
// amendment: A:X is rejected when A already occurs in X's bases
struct A {int f() const {return 1+super::f();}};
struct C {int f() const {return 0;}};
struct X : A:C {};
struct Y : A:X {};
