/**
 * @file zero_overhead_demo.cpp
 * @brief Minimal case proving HAPI's static composition compiles to the same
 *        code as hand-written direct calls, with a virtual-dispatch control
 *        case that the compiler genuinely cannot collapse.
 *
 * Companion to the Godbolt check (see check_zero_overhead.sh): run_static
 * must disassemble to a straight-line sequence with no indirect call/jmp;
 * run_dynamic must contain one (proving the test isn't just inlining
 * everything regardless of composition style).
 */

#include "hapi/hapi.h"
#include <iostream>

using namespace hapi;

template<typename I=Nil>
struct Item:I {
  static void api(const char*o) {std::cout<<o;}
  static void api() {I::Obj::api("default");}
};

template<char oc, char cc>
struct WrapWith {
  template<typename I>
  struct Part:I {
    using I::api;
    static void api(const char*o) {
      std::cout<<oc;
      I::api(o);
      std::cout<<cc;
    }
  };
};

using Parens=WrapWith<'(',')'>;
using SqBracks=WrapWith<'[',']'>;

struct IItem {
  virtual void api() const=0;
  virtual void api(const char*) const=0;
  virtual ~IItem()=default;
};

template<typename... OO> struct ItemDef;

template<typename... OO>
struct IItemOf:IItem,ItemDef<OO...> {
  using Base=ItemDef<OO...>;
  void api() const override {Base::api();}
  void api(const char*o) const override {Base::api(o);}
};

template<typename... OO>
struct ItemDef:APIOf<Item<CRTP<APIOf<Item<>,OO...>>>,OO...> {};

// Static composition: concrete type known at call site
ItemDef<Parens,SqBracks> staticItem;

// Dynamic composition: called ONLY through the abstract base pointer,
// forcing real virtual dispatch (compiler cannot devirtualize this)
using ItemV=IItemOf<Parens,SqBracks>;
ItemV dynItemStorage;
IItem* dynItem = &dynItemStorage; // opaque to the optimizer across TU boundary in real use

extern "C" void run_static() {
    staticItem.api("static");
}

extern "C" void run_dynamic() {
    dynItem->api("dynamic");
}

int main() {
    run_static();
    std::cout << "\n";
    run_dynamic();
    std::cout << "\n";
}
