#pragma once
// shared by the round-3 programs; written in ':' syntax and translated like them
#include <hapi/hapi.h>
#include <cstdio>
#include <type_traits>
static int fails=0;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL %s:%d %s\n",__FILE__,__LINE__,#c); ++fails; } }while(0)
#define DONE(name) do{ if(!fails) std::printf("%s OK\n",name); return fails; }while(0)
