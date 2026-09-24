#include "roll_common.h"
#include <cstdio>
#include <cstdlib>
int main(){ int agree=0,agreeS=0; srand(3);
  for(int t=0;t<10000;t++){ wave::Features<60> f; for(int i=0;i<60;i++) f.v[i]=rand()&255;
    bool u=Unrolled::proc<0>(f), r=Rolled::proc<0>(f), s=Sparse::proc<0>(f); agree+=u==r; agreeS+=u==s; }
  printf("rolled==unrolled %d/10000, sparse-order rolled==unrolled %d/10000\n",agree,agreeS); }
