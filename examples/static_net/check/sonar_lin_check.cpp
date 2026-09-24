// Runs the quantized int32-accumulator lin cell (via snet) over each fold's
// held-out rows, and reports: int-model accuracy, the float model's own
// accuracy (from sonar_lin_train.c, passed through as macros), and agreement
// between the two models' predictions on the same rows (not the same thing as
// accuracy -- two models can each be ~75% accurate while disagreeing on
// different rows).
#include "waveCell.h"
#include "sonar_lin_params.h"
#include "sonar_lin_vectors.h"
#include <cstdio>

template<typename Cell,size_t NVEC>
void check(const char* name,const uint8_t (&vec)[NVEC][62],double floatTrain,double floatTest){
  int correct=0, agree=0;
  for(size_t i=0;i<NVEC;i++){
    wave::Features<60> f; for(int j=0;j<60;j++) f.v[j]=vec[i][j];
    bool trueLabel=vec[i][60], floatPred=vec[i][61];
    bool intPred=Cell::proc(f);
    correct+=(intPred==trueLabel);
    agree+=(intPred==floatPred);
  }
  printf("%s  n=%zu  float(train %.2f test %.2f)  int8 test %.2f  agreement %.2f\n",
    name,NVEC,floatTrain,floatTest,100.0*correct/NVEC,100.0*agree/NVEC);
}

int main(){
  check<SonarLinFold0>("fold0",SONAR_LIN_FOLD0_VEC,SONAR_LIN_FOLD0_TRAIN_ACC,SONAR_LIN_FOLD0_TEST_ACC);
  check<SonarLinFold1>("fold1",SONAR_LIN_FOLD1_VEC,SONAR_LIN_FOLD1_TRAIN_ACC,SONAR_LIN_FOLD1_TEST_ACC);
  check<SonarLinFold2>("fold2",SONAR_LIN_FOLD2_VEC,SONAR_LIN_FOLD2_TRAIN_ACC,SONAR_LIN_FOLD2_TEST_ACC);
  check<SonarLinFold3>("fold3",SONAR_LIN_FOLD3_VEC,SONAR_LIN_FOLD3_TRAIN_ACC,SONAR_LIN_FOLD3_TEST_ACC);
  check<SonarLinFold4>("fold4",SONAR_LIN_FOLD4_VEC,SONAR_LIN_FOLD4_TRAIN_ACC,SONAR_LIN_FOLD4_TEST_ACC);
}
