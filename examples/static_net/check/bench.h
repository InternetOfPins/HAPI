// The Sonar benchmark, independent of the platform: the same function runs on the host (host_ref.cpp), on an ATmega328p
// (sonar_rows_avr.cpp) and, in the author's notes, on Cortex-M. On AVR the 32-bit product costs one real call per term
// (__umulhisi3/__muluhisi3); on chips with a native 32-bit multiply (Cortex-M3/M4: mul/mla/smull) that cost does not exist.
//   1. correctness: fold 0's 42 held-out rows through the quantized cell, narrow (Prod=int16_t) and wide (Prod=int32_t);
//      expect the host numbers: correct 33/42, agree 42/42 (the Sonar table in the README). Each cell's 42 outputs are printed as a bit string
//      ("narrow rows 0110..."), so a device can be compared with the host ROW BY ROW, not just by counts: bitexact.py,
//      against host_ref.cpp (the same code on the host)
//   2. cycles: each cell on one row, min and median of REPS runs, minus the empty-measurement baseline. The timed region
//      runs with interrupts off (a tick would otherwise land in it) behind a compiler barrier (KEEP), like timing/harness.h.
//      Flash wait states and the prefetch buffer are part of what is measured: this is the number on this chip, not an ideal.
// The platform provides the four functions below, calls bench_start() once and bench_run() as often as it likes (bench_correctness()
// alone needs no counter and runs anywhere: that is what host_ref.cpp does).
#pragma once
#include <stdint.h>
#include "waveCell.h"
// the rows live in RAM/flash as plain data unless the platform says otherwise (AVR: SONAR_LIN_VEC_ATTR PROGMEM and BENCH_VEC with pgm_read_byte)
#ifndef SONAR_LIN_VEC_ATTR
  #define SONAR_LIN_VEC_ATTR
#endif
#ifndef BENCH_VEC
  #define BENCH_VEC(r,j) (SONAR_LIN_FOLD0_VEC[r][j])
#endif
#include "sonar_lin_params.h"
#include "sonar_lin_vectors.h"

void bench_str(const char* s);       // print a string
void bench_u32(uint32_t v);          // print v in decimal
void bench_irq_off();
void bench_irq_on();

#define KEEP(x) asm volatile("" : "+r"(x))                       // forces x to exist here: the timed code cannot be hoisted out
// ARMv7-M debug registers at their architectural addresses (the CMSIS bundled with the Arduino SAM core predates DWT for the M3)
#define DEMCR      (*(volatile uint32_t*)0xE000EDFC)   // bit 24 TRCENA: enables DWT
#define DWT_CTRL   (*(volatile uint32_t*)0xE0001000)   // bit 0 CYCCNTENA
#define DWT_CYCCNT (*(volatile uint32_t*)0xE0001004)
static inline void bench_start(){ DEMCR|=(1ul<<24); DWT_CYCCNT=0; DWT_CTRL|=1u; }

// out of line, so the generated code of each cell can be read from the ELF on its own (arm-none-eabi-objdump -d)
__attribute__((noinline)) bool runNarrow(const wave::Features<60>& f){ return SonarLinFold0::proc(f); }
__attribute__((noinline)) bool runWide  (const wave::Features<60>& f){ return SonarLinFold0Wide::proc(f); }

static wave::Features<60> benchRow(uint16_t r){ wave::Features<60> f; for(int j=0;j<60;j++) f.v[j]=BENCH_VEC(r,j); return f; }

enum { REPS=25 };
[[maybe_unused]] static uint32_t benchCycles(bool (*fn)(const wave::Features<60>&), const wave::Features<60>& f){
  uint32_t best=0xFFFFFFFFu, all[REPS];
  for(int i=0;i<REPS;i++){
    bench_irq_off();
    uint32_t t0=DWT_CYCCNT; bool y=fn(f); KEEP(y); uint32_t t1=DWT_CYCCNT;
    uint32_t e0=DWT_CYCCNT; uint32_t e1=DWT_CYCCNT;            // the cost of reading the counter twice
    bench_irq_on();
    all[i]=(t1-t0)-(e1-e0); if(all[i]<best) best=all[i];
  }
  for(int i=1;i<REPS;i++){ uint32_t v=all[i]; int j=i-1; while(j>=0&&all[j]>v){all[j+1]=all[j];j--;} all[j+1]=v; }   // sort
  bench_u32(best); bench_str(" min, "); bench_u32(all[REPS/2]); bench_str(" median");
  return best;
}

// correctness: the counts and, for each cell, its output on every held-out row as a bit string
static void bench_correctness(){
  uint16_t okN=0,agN=0,okW=0,agW=0;
  char rowsN[SONAR_LIN_FOLD0_NVEC+1], rowsW[SONAR_LIN_FOLD0_NVEC+1];
  for(uint16_t r=0;r<SONAR_LIN_FOLD0_NVEC;r++){
    wave::Features<60> f=benchRow(r);
    bool truth=BENCH_VEC(r,60), flt=BENCH_VEC(r,61);
    bool n=runNarrow(f), w=runWide(f);
    okN+=(n==truth); agN+=(n==flt); okW+=(w==truth); agW+=(w==flt);
    rowsN[r]=n?'1':'0'; rowsW[r]=w?'1':'0';
  }
  rowsN[SONAR_LIN_FOLD0_NVEC]=0; rowsW[SONAR_LIN_FOLD0_NVEC]=0;
  bench_str("narrow (Prod=int16) correct "); bench_u32(okN); bench_str("/"); bench_u32(SONAR_LIN_FOLD0_NVEC);
  bench_str(" agree "); bench_u32(agN); bench_str("\r\n");
  bench_str("narrow rows "); bench_str(rowsN); bench_str("\r\n");
  bench_str("wide   (Prod=int32) correct "); bench_u32(okW); bench_str("/"); bench_u32(SONAR_LIN_FOLD0_NVEC);
  bench_str(" agree "); bench_u32(agW); bench_str("\r\n");
  bench_str("wide   rows "); bench_str(rowsW); bench_str("\r\n");
}

static void bench_cycles(){
  wave::Features<60> f=benchRow(0);
  bench_str("cycles narrow: "); benchCycles(runNarrow,f); bench_str("\r\n");
  bench_str("cycles wide  : "); benchCycles(runWide,f);   bench_str("\r\n");
}

[[maybe_unused]] static void bench_run(){ bench_correctness(); bench_cycles(); }
