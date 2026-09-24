#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#define NF 4
typedef uint8_t u8;
int N; double X[1400][NF]; int Y[1400];
u8 Q[1400][NF];
typedef struct {int n,s,p,m;} P;
static inline u8 g(u8 x,P q){u8 t=q.n?(u8)~x:x; t=q.s>=0?(u8)(t<<q.s):(u8)(t>>(-q.s)); return (u8)(t+q.p)&q.m;}
static unsigned rs=1; static int rnd(int k){rs=rs*1103515245+12345; return (rs>>16)%k;}
// best K given v[] over idx rows; returns correct count, sets *K
int bestK(int*idx,int n,u8*v,int*K){
  int d[512]={0},tot0=0;
  for(int i=0;i<n;i++){int r=idx[i]; if(Y[r]) d[v[i]]++; else {d[v[i]]--; tot0++;}}
  for(int i=0;i<256;i++) d[i+256]=d[i];
  int w=0; for(int i=0;i<128;i++) w+=d[i];
  int best=-1e9,bk=0;
  for(int st=0;st<256;st++){ if(w>best){best=w;bk=(256-st)&255;} w+=d[st+128]-d[st]; }
  *K=bk; return tot0+best;
}
int evalset(int*idx,int n,P*q,int K){int c=0;for(int i=0;i<n;i++){int r=idx[i];u8 a=K;for(int j=0;j<NF;j++)a+=g(Q[r][j],q[j]);c+=((a<128)==Y[r]);}return c;}
// coordinate descent; masked=0 -> frequency only
int train(int*idx,int n,int masked,P*out,int*outK,P*seed){
  static u8 part[1400],v[1400]; int bestAll=-1;
  for(int rest=0;rest<4;rest++){
    P q[NF]; for(int j=0;j<NF;j++){q[j].n=rnd(2);q[j].s=rnd(15)-7;q[j].p=masked?rnd(256):0;q[j].m=masked?rnd(256):255;}
    if(seed&&rest==0) memcpy(q,seed,sizeof q);
    int K,cur=0;
    for(int sweep=0;sweep<8;sweep++){
      int improved=0;
      for(int j=0;j<NF;j++){
        for(int i=0;i<n;i++){int r=idx[i];u8 a=0;for(int k=0;k<NF;k++) if(k!=j) a+=g(Q[r][k],q[k]); part[i]=a;}
        P bq=q[j]; int bs=-1;
        for(int nn=0;nn<2;nn++)for(int s=-7;s<=7;s++)
         for(int p=0;p<(masked?256:1);p+=8)for(int m=(masked?1:255);m<256;m++){
          P c={nn,s,p,m}; for(int i=0;i<n;i++) v[i]=part[i]+g(Q[idx[i]][j],c);
          int k,sc=bestK(idx,n,v,&k); if(sc>bs){bs=sc;bq=c;}
        }
        if(masked){ // refine phase at step 1
          for(int p=0;p<256;p++){P c=bq;c.p=p; for(int i=0;i<n;i++) v[i]=part[i]+g(Q[idx[i]][j],c);
            int k,sc=bestK(idx,n,v,&k); if(sc>bs){bs=sc;bq=c;}}
        }
        if(bs>cur){cur=bs;improved=1;} q[j]=bq;
      }
      if(!improved) break;
    }
    for(int i=0;i<n;i++){u8 a=0;int r=idx[i];for(int k=0;k<NF;k++)a+=g(Q[r][k],q[k]);v[i]=a;}
    int sc=bestK(idx,n,v,&K);
    if(sc>bestAll){bestAll=sc;memcpy(out,q,sizeof q);*outK=K;} // pocket, strict improvement
  }
  return bestAll;
}
// float pocket perceptron baseline on same quantized inputs
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
  FILE*f=fopen("bn.csv","r"); while(fscanf(f,"%lf,%lf,%lf,%lf,%d",&X[N][0],&X[N][1],&X[N][2],&X[N][3],&Y[N])==5)N++;
  printf("rows=%d\n",N);
  int perm[1400]; for(int i=0;i<N;i++)perm[i]=i; rs=42; for(int i=N-1;i>0;i--){int k=rnd(i+1),t=perm[i];perm[i]=perm[k];perm[k]=t;}
  double sA=0,sB=0,sC=0,tA=0,tB=0,tC=0;
  for(int fold=0;fold<5;fold++){
    int tr[1400],te[1400],ntr=0,nte=0;
    for(int i=0;i<N;i++){ if(i*5/N==fold) te[nte++]=perm[i]; else tr[ntr++]=perm[i]; }
    double mn[NF],mx[NF]; for(int j=0;j<NF;j++){mn[j]=1e9;mx[j]=-1e9;for(int i=0;i<ntr;i++){double x=X[tr[i]][j];if(x<mn[j])mn[j]=x;if(x>mx[j])mx[j]=x;}}
    for(int r=0;r<N;r++)for(int j=0;j<NF;j++){double z=round((X[r][j]-mn[j])/(mx[j]-mn[j])*255);if(z<0)z=0;if(z>255)z=255;Q[r][j]=z;}
    P qa[NF],qb[NF]; int Ka,Kb; double W[NF+1];
    int ta=train(tr,ntr,0,qa,&Ka,0), tb=train(tr,ntr,1,qb,&Kb,qa), tc=perceptron(tr,ntr,W);
    int ea=evalset(te,nte,qa,Ka), eb=evalset(te,nte,qb,Kb);
    int ec=0;for(int i=0;i<nte;i++){int r=te[i];double a=W[NF];for(int j=0;j<NF;j++)a+=W[j]*Q[r][j]/255.0;ec+=((a>0)==Y[r]);}
    { char fn[64]; snprintf(fn,sizeof fn,"fold%d.csv",fold); FILE*o=fopen(fn,"w"); fprintf(o,"split,q0,q1,q2,q3,y\n");
      for(int i=0;i<ntr;i++){int r=tr[i]; fprintf(o,"train,%d,%d,%d,%d,%d\n",Q[r][0],Q[r][1],Q[r][2],Q[r][3],Y[r]);}
      for(int i=0;i<nte;i++){int r=te[i]; fprintf(o,"test,%d,%d,%d,%d,%d\n",Q[r][0],Q[r][1],Q[r][2],Q[r][3],Y[r]);}
      fclose(o);
      FILE*w=fopen("perceptron.csv",fold?"a":"w"); if(!fold) fprintf(w,"fold,w0,w1,w2,w3,bias\n");
      fprintf(w,"%d,%.17g,%.17g,%.17g,%.17g,%.17g\n",fold,W[0],W[1],W[2],W[3],W[4]); fclose(w); }
    printf("fold %d | freq-only  train %.2f test %.2f | masked wave train %.2f test %.2f | float perceptron train %.2f test %.2f\n",fold,
      100.0*ta/ntr,100.0*ea/nte,100.0*tb/ntr,100.0*eb/nte,100.0*tc/ntr,100.0*ec/nte);
    printf("   masked params K=%d:",Kb); for(int j=0;j<NF;j++) printf(" [n%d s%+d p%d m%02x]",qb[j].n,qb[j].s,qb[j].p,qb[j].m); 
    printf("\n   freq params   K=%d:",Ka); for(int j=0;j<NF;j++) printf(" [n%d s%+d]",qa[j].n,qa[j].s); printf("\n"); fflush(stdout);
    if(fold==1){FILE*h=fopen("wave_params.h","w");
      fprintf(h,"// generated by bn2.c, Banknote 5-fold CV, fold 1 (seed 42): train %.2f%% test %.2f%%\n",100.0*tb/ntr,100.0*eb/nte);
      fprintf(h,"#pragma once\n#define WAVE_K %d\n",Kb);
      for(int j=0;j<NF;j++) fprintf(h,"#define WAVE_F%d_N %d\n#define WAVE_F%d_S %d\n#define WAVE_F%d_P %d\n#define WAVE_F%d_M 0x%02x\n",j,qb[j].n,j,qb[j].s,j,qb[j].p,j,qb[j].m);
      fprintf(h,"// quantization q=round((x-min)/(max-min)*255), clamped\nstatic const double WAVE_MIN[4]={%.17g,%.17g,%.17g,%.17g};\nstatic const double WAVE_MAX[4]={%.17g,%.17g,%.17g,%.17g};\n",mn[0],mn[1],mn[2],mn[3],mx[0],mx[1],mx[2],mx[3]);
      fclose(h);
      FILE*v=fopen("wave_vectors.h","w");
      fprintf(v,"// fold-1 held-out rows, quantized with fold-1 train min/max: {q0,q1,q2,q3,label,expected_model_out}\n#pragma once\n#define WAVE_NVEC %d\nstatic const unsigned char WAVE_VEC[%d][6] WAVE_VEC_ATTR={\n",nte,nte);
      for(int i=0;i<nte;i++){int r=te[i];u8 a=Kb;for(int j=0;j<NF;j++)a+=g(Q[r][j],qb[j]);fprintf(v," {%d,%d,%d,%d,%d,%d},\n",Q[r][0],Q[r][1],Q[r][2],Q[r][3],Y[r],a<128);}
      fprintf(v,"};\n");fclose(v);}
    sA+=100.0*ea/nte;sB+=100.0*eb/nte;sC+=100.0*ec/nte;tA+=100.0*ta/ntr;tB+=100.0*tb/ntr;tC+=100.0*tc/ntr;
  }
  printf("MEAN | freq-only train %.2f test %.2f | masked train %.2f test %.2f | perceptron train %.2f test %.2f\n",tA/5,sA/5,tB/5,sB/5,tC/5,sC/5);
}
