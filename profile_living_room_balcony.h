
#include "button2/button2.h"

#define RS_ARRAY_SIZE (2)
#include "roller_shutter.h"

void setup_rs() {
  //id, btn_up, btn_dn, relay_up, relay_dn
  rs[0].setup(0, A0, A1, 7, 8);
  rs[1].setup(1, A2, A3, 9, 10);
}
