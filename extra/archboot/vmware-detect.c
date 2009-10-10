#include <stdio.h>
int main() {
  unsigned char idtr[6];
  asm("sidt %0" : "=m" (idtr));
  if(0xff==idtr[5])
  {
    return 0;
  }
  else
  {
    return 1;
  }
}
