// AVR size build: 60 raw inputs from volatile, fold 0's quantized int32-accumulator
// lin cell (arbitrary fold choice -- size shouldn't meaningfully depend on which
// fold's constants are baked in, only on the shape: 60 In<> terms + Sign).
#include "waveCell.h"
#include "inputs.h"
#include "sonar_lin_params.h"
volatile uint8_t in_[60];
volatile bool out_;
int main(){
  for(;;){
    inp::Bytes60 f; for(int j=0;j<60;j++) inp::band(f)[j]=in_[j];
    out_=SonarLinFold0::proc(f);
  }
}
