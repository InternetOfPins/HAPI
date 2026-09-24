#include "waveCell.h"
using namespace wave;
using Bad=Net<Cell<0,RefWave<1,0,0,0,0xff>>, Cell<0,RefWave<0,0,0,0,0xff>>>;
int main(){Features<1> f{{0}}; return Bad::proc<1>(f);}
