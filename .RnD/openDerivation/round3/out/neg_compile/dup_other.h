#pragma once
// a component defined in another header (translated on its own): dup_other_header.cpp cannot see inside it
#include <hapi/hapi.h>
struct C {int f() const {return 0;}};
struct A {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int f() const {return 1+Base::f();}};};
struct B {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int f() const {return 2+Base::f();}};};
struct X : hapi::Chain<A,B> {static_assert(hapi::Distinct<hapi::Chain<A,B>>, "duplicate layer in X");};
