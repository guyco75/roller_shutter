
#include "button2/button2.h"

#define RS_ARRAY_SIZE (2)
#include "roller_shutter.h"

void setup_rs() {
  DDRC = 0;    // set all analog pins to INPUT
  PORTC = 0xFF; // pull up

  //pin port, btn_up, btn_dn, relay_up, relay_dn
  rs[0].setup(0, &PINC, 0, 1, 7, 8);
  rs[1].setup(1, &PINC, 2, 3, 9, 10);
}
