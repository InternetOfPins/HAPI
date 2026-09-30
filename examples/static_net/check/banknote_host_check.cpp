// host: HAPI net vs reference C model (expected_model_out) and labels, fold-1 held-out rows
#define WAVE_VEC_ATTR
#include "net.h"
#include "wave_vectors.h"
#include <cstdio>
int main(){
  int agree=0,correct=0;
  for(int r=0;r<WAVE_NVEC;r++){
    BanknoteState f=banknote({WAVE_VEC[r][WAVE_COL_VARIANCE],WAVE_VEC[r][WAVE_COL_SKEWNESS],WAVE_VEC[r][WAVE_COL_CURTOSIS],WAVE_VEC[r][WAVE_COL_ENTROPY]});
    bool o=BanknoteNet::proc(f);
    agree+=o==WAVE_VEC[r][WAVE_COL_MODEL]; correct+=o==WAVE_VEC[r][WAVE_COL_LABEL];
  }
  printf("model agreement %d/%d, accuracy %d/%d\n",agree,WAVE_NVEC,correct,WAVE_NVEC);
  return agree!=WAVE_NVEC;
}
