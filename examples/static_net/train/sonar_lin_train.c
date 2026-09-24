// Trains the float pocket perceptron on Sonar, same fold split and input
// quantization as the original sonar.c of the waveCompose research code (rs=42 top-level shuffle,
// same 5-fold split, same per-train-fold min/max -> u8 quantization). Does NOT
// replay the frequency/masked wave search that sonar.c runs first in each
// fold's block -- that search's rnd() draws aren't needed to reproduce the
// fold split (which happens once, before any training) and reimplementing it
// adds nothing here (the perceptron already trains this cell). Because of
// that, the RNG state perceptron() sees here is NOT bit-identical to sonar.c's
// original run (there, freq+masked training consumed rnd() calls first) --
// so `rs` is reset to a fixed, documented seed right before each fold's
// perceptron() call instead, making this program's own output reproducible
// run-to-run even though it won't reproduce sonar.c's exact original weights.
// Prints, per fold: row/fold sizes, float train/test accuracy, the 61 trained
// weights (w0..w59, bias), then each held-out row's u8 features + label, for
// gen_lin_sonar.py to quantize and turn into a C++ header.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#define NF 60
typedef uint8_t u8;
int N; double X[260][NF]; int Y[260]; u8 Q[260][NF];
static unsigned rs=1; static int rnd(int k){rs=rs*1103515245+12345; return (rs>>16)%k;}

int perceptron(int*tr,int ntr,double*W){
  double w[NF+1]={0},bw[NF+1]={0}; int best=-1;
  for(int ep=0;ep<300;ep++){
    for(int i=0;i<ntr;i++){int r=tr[rnd(ntr)]; double a=w[NF];for(int j=0;j<NF;j++)a+=w[j]*Q[r][j]/255.0;
      int yp=a>0,t=Y[r]; if(yp!=t){double d=t?1:-1;for(int j=0;j<NF;j++)w[j]+=d*Q[r][j]/255.0;w[NF]+=d;}}
    int c=0;for(int i=0;i<ntr;i++){int r=tr[i];double a=w[NF];for(int j=0;j<NF;j++)a+=w[j]*Q[r][j]/255.0;c+=((a>0)==Y[r]);}
    if(c>best){best=c;memcpy(bw,w,sizeof w);}
  }
  memcpy(W,bw,sizeof bw); return best;
}
int main(){
  FILE*f=fopen("sonar.csv","r"); char lab;
  while(1){int j;for(j=0;j<NF;j++) if(fscanf(f,"%lf,",&X[N][j])!=1) break; if(j<NF) break;
    if(fscanf(f," %c",&lab)!=1) break; Y[N]=(lab=='M'); N++;}
  printf("rows=%d\n",N);
  int perm[260]; for(int i=0;i<N;i++)perm[i]=i; rs=42; for(int i=N-1;i>0;i--){int k=rnd(i+1),t=perm[i];perm[i]=perm[k];perm[k]=t;}
  for(int fold=0;fold<5;fold++){
    int tr[260],te[260],ntr=0,nte=0;
    for(int i=0;i<N;i++){ if(i*5/N==fold) te[nte++]=perm[i]; else tr[ntr++]=perm[i]; }
    double mn[NF],mx[NF]; for(int j=0;j<NF;j++){mn[j]=1e9;mx[j]=-1e9;for(int i=0;i<ntr;i++){double x=X[tr[i]][j];if(x<mn[j])mn[j]=x;if(x>mx[j])mx[j]=x;}}
    for(int r=0;r<N;r++)for(int j=0;j<NF;j++){double z=round((X[r][j]-mn[j])/(mx[j]-mn[j])*255);if(z<0)z=0;if(z>255)z=255;Q[r][j]=z;}
    rs=42; // fixed, documented seed for perceptron training specifically (see file header)
    double W[NF+1]; int ta=perceptron(tr,ntr,W);
    int ec=0;for(int i=0;i<nte;i++){int r=te[i];double a=W[NF];for(int j=0;j<NF;j++)a+=W[j]*Q[r][j]/255.0;ec+=((a>0)==Y[r]);}
    printf("FOLD %d ntr=%d nte=%d\n",fold,ntr,nte);
    printf("TRAIN_ACC %.5f\n",100.0*ta/ntr);
    printf("TEST_ACC %.5f\n",100.0*ec/nte);
    printf("W");for(int j=0;j<=NF;j++)printf(" %.10g",W[j]);printf("\n");
    // ROW: 60 quantized features, true label, float model's own prediction
    // (so the C++ side can compute float-vs-int agreement, not just accuracy)
    for(int i=0;i<nte;i++){int r=te[i];printf("ROW");for(int j=0;j<NF;j++)printf(" %d",Q[r][j]);
      double a=W[NF];for(int j=0;j<NF;j++)a+=W[j]*Q[r][j]/255.0;
      printf(" %d %d\n",Y[r],a>0);}
  }
}
