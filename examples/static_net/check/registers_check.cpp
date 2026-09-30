// Registers as layers of the state (registers.h) over the cells of a net, which stay `proc(in)`.
//   counter    one register, n' = n + 1
//   fibonacci  two registers that read each other, (a,b)' = (b, a+b): both new values come from the previous state, so the recurrence holds
//   output     a combinational cell (a Net cell) read over the state after the step
#include "waveCell.h"
#include "linCell.h"
#include "registers.h"
#include <cstdio>

struct CounterTag {};
struct SlotCounter { uint8_t n; };
using N = snet::Field<CounterTag,&SlotCounter::n>;
using CounterRegs = snet::Registers<CounterTag, snet::Reg<&SlotCounter::n, wave::Cell<0, wave::Wave<N,0,0,1,0xff>>>>;     // n + 1
using CounterState = hapi::APIOf<snet::RegApi, snet::RegLayer<CounterTag,SlotCounter,CounterRegs>>::Res;


struct FibTag {};
struct SlotFib { uint8_t a, b; };
using FA = snet::Field<FibTag,&SlotFib::a>; using FB = snet::Field<FibTag,&SlotFib::b>;
using FibRegs = snet::Registers<FibTag,
  snet::Reg<&SlotFib::a, wave::Cell<0, wave::Wave<FB,0,0,0,0xff>>>,                                   // a' = b
  snet::Reg<&SlotFib::b, wave::Cell<0, wave::Wave<FA,0,0,0,0xff>, wave::Wave<FB,0,0,0,0xff>>>>;       // b' = a + b   (mod 256, as the u8 engine)
using FibState = hapi::APIOf<snet::RegApi, snet::RegLayer<FibTag,SlotFib,FibRegs>>::Res;

// a combinational cell over the registers, read with Net<>::proc on the finished state: is b odd?
using Out = snet::Net<wave::Cell<128, wave::Threshold, wave::Wave<FB,0,7,0,0x80>>>;

int main() {
  printf("counter:"); CounterState p{}, q{}; hapi::slot<CounterTag>(p).n = 0;
  for (int i = 0; i < 5; i++) { q.step(p); p = q; printf(" %d", hapi::slot<CounterTag>(p).n); }
  printf("\n");
  FibState a{}, b{}; hapi::slot<FibTag>(a).a = 0; hapi::slot<FibTag>(a).b = 1; int x = 0, y = 1, bad = 0, badOut = 0;
  for (int i = 0; i < 30; i++) {
    b.step(a); a = b; int t = (x + y) & 255; x = y; y = t;
    bad += hapi::slot<FibTag>(a).a != x || hapi::slot<FibTag>(a).b != y;
    badOut += Out::proc<0>(a) != bool(y & 1);                                // (b<<7)&0x80 plus the bias 128 wraps to 0 when b is odd: Threshold (< 128) reads 1
  }
  printf("fibonacci: (a,b)=(%d,%d), %d of 30 passes wrong, output cell wrong %d times\n", hapi::slot<FibTag>(a).a, hapi::slot<FibTag>(a).b, bad, badOut);
  return bad || badOut;
}
