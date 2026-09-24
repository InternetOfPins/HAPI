#!/bin/bash
# The crossover behind sugar's SUGAR_ROLL_AT: the same cell with N terms, unrolled (lin::Cell of lin::In) and rolled (Roll of T), on AVR:
# total flash of a tiny program (avr-size) and cycles of the cell (simavr, harness.h, as run.sh). The weights are the first N of
# the Sonar fold's. Needs HAPI=<hapi/include>, avr-g++, simavr, python3. usage: ./roll_sweep.sh [N ...]   (default: 1..60 in steps)
H=${HAPI:-../../../include}
W="-93,18,89,78,68,-111,-62,-97,-1,67,-12,-7,39,-30,74,-74,-103,-3,-120,101,86,-28,-17,28,68,69,-127,51,-13,-59,57,78,-69,24,114,-101,103,-46,-120,-122,-121,39,11,-125,113,98,-30,48,-72,121,-19,58,-120,8,-71,68,-15,113,-1,14"
gen(){ python3 - "$1" "$2" "$3" "$W" <<'PY'
import sys
N=int(sys.argv[1]); form=sys.argv[2]; kind=sys.argv[3]; W=[int(v) for v in sys.argv[4].split(",")]
if form=='u': net="snet::Net<lin::Cell<-2000,lin::Sign,%s>>"%",".join("lin::In<%d,%d>"%(i,W[i]) for i in range(N))
else:         net="snet::Net<hapi::APIOf<lin::API,lin::Sign,snet::Roll<%s>,lin::Bias<-2000>>>"%",".join("snet::T<%d,%d>"%(i,W[i]) for i in range(N))
hdr='#include "waveCell.h"\n#include "linCell.h"\n#include "roll.h"\n'
if kind=='cyc':
    print('#include "harness.h"\n'+hdr+"using NetT=%s;"%net)
    print("volatile uint8_t x[%d]; volatile uint8_t out_;"%N)
    print("__attribute__((noinline)) bool cls(wave::Features<%d>& f){ return NetT::proc<0>(f); }"%N)
    print("int main(){ uinit(); wave::Features<%d> f; for(int i=0;i<%d;i++) f.v[i]=x[i]^(i*37);"%(N,N))
    print('  MEASURE("%s:", out_=cls(f)); done(); }'%form)
else:
    print(hdr+"using NetT=%s;"%net)
    print("volatile uint8_t in_[%d]; volatile uint8_t out_;"%N)
    print("int main(){ for(;;){ wave::Features<%d> f; for(int i=0;i<%d;i++) f.v[i]=in_[i]; out_=NetT::proc<0>(f);} }"%(N,N))
PY
}
D=$(mktemp -d); trap 'rm -rf "$D"' EXIT
declare -A fl cy
printf "%3s | %8s %8s | %8s %8s\n" terms "flash U" "flash R" "cycles U" "cycles R"
for N in ${@:-1 2 3 4 5 6 7 8 9 10 12 16 24 32 48 60}; do
  for f in u r; do
    gen $N $f size > $D/s.cpp; avr-g++ -std=c++17 -Os -mmcu=atmega328p -I. -I../include -I$H $D/s.cpp -o $D/s.elf 2>/dev/null; fl[$f]=$(avr-size -C --mcu=atmega328p $D/s.elf | awk '/^Program:/{print $2}')
    gen $N $f cyc > $D/c.cpp; avr-gcc -std=c++17 -Os -mmcu=atmega328p -I. -I../include -I$H $D/c.cpp -o $D/c.elf 2>/dev/null; cy[$f]=$(timeout 10 simavr -m atmega328p -f 16000000 $D/c.elf 2>&1 | grep -aoE "${f}:[0-9]+" | cut -d: -f2)
  done
  printf "%3d | %8s %8s | %8s %8s\n" $N ${fl[u]} ${fl[r]} ${cy[u]} ${cy[r]}
done
