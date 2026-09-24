// The benchmark's correctness half on the host: the very same bench_correctness() a device runs (include/bench.h), printed to stdout.
// bitexact.py compares a device's report with this row by row.
#include <cstdio>
#include "bench.h"
void bench_str(const char* s) { fputs(s, stdout); }
void bench_u32(uint32_t v)    { printf("%u", v); }
void bench_irq_off()          {}
void bench_irq_on()           {}
int main() { bench_correctness(); }
