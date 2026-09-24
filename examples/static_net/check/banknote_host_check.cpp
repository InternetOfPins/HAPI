// host: HAPI net vs reference C model (expected_model_out) and labels, fold-1 held-out rows
#define WAVE_VEC_ATTR
#include "net.h"
#include "wave_vectors.h"
#include <cstdio>
int main(){
  int agree=0,correct=0;
  for(int r=0;r<WAVE_NVEC;r++){
    wave::Features<4> f{{WAVE_VEC[r][0],WAVE_VEC[r][1],WAVE_VEC[r][2],WAVE_VEC[r][3]}};
    bool o=BanknoteNet::proc(f);
    agree+=o==WAVE_VEC[r][5]; correct+=o==WAVE_VEC[r][4];
  }
  printf("model agreement %d/%d, accuracy %d/%d\n",agree,WAVE_NVEC,correct,WAVE_NVEC);
  return agree!=WAVE_NVEC;
}
