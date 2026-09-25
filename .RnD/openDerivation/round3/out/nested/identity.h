#pragma once
// rule 3, struct-only form: identity is nominal. Included by identity.cpp, identity_tu1.cpp and identity_tu2.cpp
#include "common.h"
struct C   {int v=1;};
struct A   {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int a() const {return 10*Base::b();}};};
struct B   {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int b() const {return 2+Base::v;}};};
struct Any {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int any() const {return 1+Base::a();}};};
struct X : A::Part<B::Part<C>> {using Base=A::Part<B::Part<C>>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<A,B,C>>, "duplicate layer in X"); static_assert(hapi::BuildRules<hapi::Chain<>,hapi::Chain<C,A,B>>::rules(), "HAPI: validation failed in X");};
int use(X& x);     // defined in identity_tu2.cpp, called from identity_tu1.cpp
