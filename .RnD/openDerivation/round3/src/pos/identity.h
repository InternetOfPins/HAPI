#pragma once
// rule 3: included by identity.cpp, identity_tu1.cpp and identity_tu2.cpp
#include "common.h"
struct C   {int v=1;};
struct A   {int a() const {return 10*super::b();}};
struct B   {int b() const {return 2+super::v;}};
struct Any {int any() const {return 1+super::a();}};
using X = A:B:C;
int use(X& x);     // defined in identity_tu2.cpp, called from identity_tu1.cpp
