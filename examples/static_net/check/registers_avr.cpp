// The Fibonacci registers of registers_check.cpp as a whole AVR program: typed (default) or hand-indexed, one harness.
//   -DFLAT       hand-indexed, both old values read before either new one is written (what prev/next means): must be the same image
//   -DFLAT_NAIVE hand-indexed, the second register read after the first is written (p and n may alias, so the compiler reloads)
#include "waveCell.h"
#include "registers.h"
struct FibTag {};
struct SlotFib { uint8_t a, b; };
using FA = snet::Field<FibTag,&SlotFib::a>; using FB = snet::Field<FibTag,&SlotFib::b>;
using FibRegs = snet::Registers<FibTag,
  snet::Reg<&SlotFib::a, wave::Cell<0, wave::Wave<FB,0,0,0,0xff>>>,
  snet::Reg<&SlotFib::b, wave::Cell<0, wave::Wave<FA,0,0,0,0xff>, wave::Wave<FB,0,0,0,0xff>>>>;
using FibState = hapi::APIOf<snet::RegApi, snet::RegLayer<FibTag,SlotFib,FibRegs>>::Res;
volatile uint8_t in_a, in_b, out_a, out_b;
#if defined(FLAT) || defined(FLAT_NAIVE)
struct Flat { uint8_t v[2]; };                                                  // [0] a, [1] b
#ifdef FLAT_NAIVE
extern "C" __attribute__((noinline)) void run_step(const Flat* p, Flat* n) { n->v[0] = p->v[1]; n->v[1] = uint8_t(p->v[0] + p->v[1]); }
#else
extern "C" __attribute__((noinline)) void run_step(const Flat* p, Flat* n) { uint8_t a = p->v[0], b = p->v[1]; n->v[0] = b; n->v[1] = uint8_t(a + b); }
#endif
int main() { Flat p, n; p.v[0] = in_a; p.v[1] = in_b;
  for (;;) { run_step(&p, &n); p = n; out_a = p.v[0]; out_b = p.v[1]; } }
#else
extern "C" __attribute__((noinline)) void run_step(const FibState* p, FibState* n) { n->step(*p); }
int main() { FibState p, n; hapi::slot<FibTag>(p).a = in_a; hapi::slot<FibTag>(p).b = in_b;
  for (;;) { run_step(&p, &n); p = n; out_a = hapi::slot<FibTag>(p).a; out_b = hapi::slot<FibTag>(p).b; } }
#endif
