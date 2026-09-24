#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <stdint.h>
static void uputc(char c){ while(!(UCSR0A&(1<<UDRE0))); UDR0=c; }
static void uputs(const char*s){ while(*s) uputc(*s++); }
static void uputu(uint16_t v){ char b[6]; int i=0; do{b[i++]='0'+v%10; v/=10;}while(v); while(i) uputc(b[--i]); }
static void uinit(){ UBRR0=8; UCSR0B=(1<<TXEN0); UCSR0C=3<<UCSZ00; TCCR1A=0; TCCR1B=1; }
static void done(){ uputc('\n'); cli(); sleep_cpu(); }
// measure: cycles of body minus empty-measure overhead
// compiler barrier: pure inlined code may otherwise be hoisted out of the timed window
#define KEEP(x) asm volatile("" : "+r"(x))
#define MEASURE(label,...) do{ uint16_t t0,t1; cli(); TCNT1=0; t0=TCNT1; __VA_ARGS__; t1=TCNT1; \
  uint16_t e0,e1; TCNT1=0; e0=TCNT1; e1=TCNT1; uputs(label); uputu((t1-t0)-(e1-e0)); uputc(' '); }while(0)
