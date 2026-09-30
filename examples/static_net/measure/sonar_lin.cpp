#include "harness.h"
#include "waveCell.h"
#include "inputs.h"
#include "sonar_lin_params.h"
volatile uint8_t x[60]; volatile uint8_t out_;
int main(){ uinit(); inp::Bytes60 f; for(int i=0;i<60;i++) inp::band(f)[i]=x[i]^(i*37);
  MEASURE("slnarrow:", out_=SonarLinFold0::proc(f));
  MEASURE("slwide:", out_=SonarLinFold0Wide::proc(f));
  done(); }
