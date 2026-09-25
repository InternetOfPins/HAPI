#pragma once
// a component defined in another header (translated on its own): dup_other_header.cpp cannot see inside it
#include <hapi/hapi.h>
struct C {int f() const {return 0;}};
struct A {int f() const {return 1+super::f();}};
struct B {int f() const {return 2+super::f();}};
struct X : A:B {};
