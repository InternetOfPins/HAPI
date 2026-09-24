// The Sonar benchmark's correctness half on an ATmega328p: bench.h's bench_correctness(), the very function the host reference
// runs, over fold 0's 42 held-out rows in PROGMEM, both cells, printed as counts and as per-row bit strings over UART. Unlike
// a counts-only check this lets bitexact.py compare the device with the host row by row:
//   simavr -m atmega328p -f 16000000 x.elf | (strip colours) > report.txt; bitexact.py --log report.txt        (simulated)
//   bitexact.py --port /dev/ttyUSB0                                                                            (a real Nano)
// Build: avr-g++ -std=c++17 -Os -mmcu=atmega328p -I. -I../include -I../models/sonar -I<hapi/include> sonar_rows_avr.cpp
// -DSIM_ONCE prints one report and sleeps, which is how simavr ends (and flushes its output); the default repeats for a board.
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#define SONAR_LIN_VEC_ATTR PROGMEM
#define BENCH_VEC(r,j) pgm_read_byte(&SONAR_LIN_FOLD0_VEC[r][j])
#include "bench.h"

#define F_CPU 16000000UL
#define BAUD 115200UL
static void uart_init(){ uint16_t ubrr=((F_CPU+8UL*BAUD)/(16UL*BAUD))-1; UBRR0H=ubrr>>8; UBRR0L=ubrr; UCSR0B=(1<<TXEN0); UCSR0C=(1<<UCSZ01)|(1<<UCSZ00); }
static void uart_tx(char c){ while(!(UCSR0A&(1<<UDRE0))); UDR0=c; }
void bench_str(const char* s){ while(*s) uart_tx(*s++); }
void bench_u32(uint32_t v){ char b[11]; uint8_t i=0; do{ b[i++]='0'+(v%10); v/=10; }while(v); while(i) uart_tx(b[--i]); }
void bench_irq_off(){}
void bench_irq_on(){}

int main(){
  uart_init();
#ifdef SIM_ONCE
  bench_correctness(); cli(); sleep_cpu();
#else
  for(;;){ bench_correctness(); for(volatile uint32_t i=0;i<800000UL;i++); }     // repeated, so a late reader sees a report
#endif
}
