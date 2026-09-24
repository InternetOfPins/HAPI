#include "waveCell.h"
using namespace wave;
using Bad=Net<Cell<0,RefWave<0,0,0,0,0xff>>>;
int main(){Features<1> f{{0}}; return Bad::proc<0>(f);}
