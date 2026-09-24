// Comparison point for sonar_lin_avr_size.cpp: same fold-0 cell, but with the
// product computed at the full int32_t accumulator width (SonarLinFold0Wide)
// instead of the int16_t product width (SonarLinFold0). See the README's Sonar table.
#include "waveCell.h"
#include "sonar_lin_params.h"
volatile uint8_t in_[60];
volatile bool out_;
int main(){
  for(;;){
    wave::Features<60> f; for(int j=0;j<60;j++) f.v[j]=in_[j];
    out_=SonarLinFold0Wide::proc(f);
  }
}
