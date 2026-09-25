#pragma once
// a named composition defined in another header (translated on its own): dup_other_header.cpp cannot see inside it
#include <hapi/rules.h>
struct C {int f() const {return 0;}};
struct A {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int f() const {return 1+Base::f();}};};
struct X : hapi::Chain<A>::Part<C> {using Base=hapi::Chain<A>::Part<C>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<A,C>>, "duplicate layer in X");};
