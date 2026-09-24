// two engines in one static net: wave cells and integer linear cells reference each other in place
#include "waveCell.h"
#include "linCell.h"
#include <cstdio>
using Mixed=snet::Net<
  /*0 wave NAND  */ wave::Cell<0,  wave::Threshold, wave::Wave<0,0,6,0,0xff>, wave::Wave<1,0,6,0,0xff>>,
  /*1 lin  OR    */ lin::Cell<-1,  lin::Sign, lin::In<0,2>, lin::In<1,2>>,
  /*2 wave AND(Ref0,Ref1) = XOR */
                    wave::Cell<128,wave::Threshold, wave::RefWave<0,0,6,0,0xff>, wave::RefWave<1,0,6,0,0xff>>,
  /*3 lin raw (int16, no readout): 10*xor - 3*nand + 7*a */
                    lin::Cell<0,   lin::RefIn<2,10>, lin::RefIn<0,-3>, lin::In<0,7>>
>;
int main(){
  int ok=1;
  for(int a=0;a<2;a++)for(int b=0;b<2;b++){
    wave::Features<2> f{{(wave::u8)a,(wave::u8)b}};
    int nand=Mixed::proc<0>(f), orr=Mixed::proc<1>(f), x=Mixed::proc<2>(f), r=Mixed::proc<3>(f);
    int rexp=10*(a^b)-3*!(a&&b)+7*a;
    printf("a=%d b=%d nand=%d or=%d xor=%d raw=%d (exp %d)\n",a,b,nand,orr,x,r,rexp);
    ok&=nand==!(a&&b)&&orr==(a||b)&&x==(a^b)&&r==rexp;
  }
  printf("sizeof(Mixed)=%zu %s\n",sizeof(Mixed),ok?"ALL OK":"FAIL");
  return !ok;
}
