#include "waveCell.h"
#include "inputs.h"
using namespace wave;
using Bad=Net<Cell<0,RefWave<0,0,0,0,0xff>>>;
int main(){auto f=inp::one({0}); return Bad::proc<0>(f);}
