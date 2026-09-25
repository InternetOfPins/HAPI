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
struct X : hapi::APIOf<C,A,B> {using Base=hapi::APIOf<C,A,B>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<A,B,C>>, "duplicate layer in X");};
int use(X& x);     // defined in identity_tu2.cpp, called from identity_tu1.cpp
