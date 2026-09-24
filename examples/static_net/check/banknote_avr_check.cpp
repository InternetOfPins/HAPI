// AVR correctness build: runs all fold-1 vectors from PROGMEM, reports over Serial-free volatile counters
#include <avr/pgmspace.h>
#define WAVE_VEC_ATTR PROGMEM
#include "net.h"
#include "wave_vectors.h"
volatile uint16_t agree_,correct_;

#define F_CPU 16000000UL
#define BAUD 115200UL
#include <avr/io.h>

static void uart_init(){
  uint16_t ubrr=((F_CPU+8UL*BAUD)/(16UL*BAUD))-1;
  UBRR0H=ubrr>>8; UBRR0L=ubrr;
  UCSR0B=(1<<TXEN0);
  UCSR0C=(1<<UCSZ01)|(1<<UCSZ00);
}
static void uart_delay(){ for(volatile uint32_t i=0;i<800000UL;i++); }
static void uart_tx(char c){ while(!(UCSR0A&(1<<UDRE0))); UDR0=c; }
static void uart_str(const char* s){ while(*s) uart_tx(*s++); }
static void uart_u16(uint16_t v){
  char buf[6]; uint8_t i=0;
  if(v==0) buf[i++]='0';
  while(v){ buf[i++]='0'+(v%10); v/=10; }
  while(i) uart_tx(buf[--i]);
}

int main(){
  uart_init();
  uint16_t a=0,c=0;
  for(uint16_t r=0;r<WAVE_NVEC;r++){
    wave::Features<4> f{{pgm_read_byte(&WAVE_VEC[r][0]),pgm_read_byte(&WAVE_VEC[r][1]),
                         pgm_read_byte(&WAVE_VEC[r][2]),pgm_read_byte(&WAVE_VEC[r][3])}};
    bool o=BanknoteNet::proc(f);
    a+=o==pgm_read_byte(&WAVE_VEC[r][5]); c+=o==pgm_read_byte(&WAVE_VEC[r][4]);
  }
  agree_=a; correct_=c;
  for(;;){
    uart_str("agree "); uart_u16(a); uart_str("/"); uart_u16(WAVE_NVEC);
    uart_str(" correct "); uart_u16(c); uart_str("/"); uart_u16(WAVE_NVEC); uart_str("\r\n");
    uart_delay();
  }
}
