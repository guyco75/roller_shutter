#include "button2/button2.h"

#define RS_ARRAY_SIZE (2)
#include "roller_shutter.h"

unsigned long single_click[] = {(100),0};
unsigned long double_click[] = {(100),100,(100),0};
unsigned long triple_click[] = {(100),100,(100),100,(100),0};
unsigned long quadruple_click[] = {(100),100,(100),100,(100),100,(100),0};
unsigned long pentuple_click[] = {(100),100,(100),100,(100),100,(100),100,(100),0};

unsigned long triple_long_click[] = {(100),100,(100),100,(100),100,(1000),0};
unsigned long quadruple_long_click[] = {(100),100,(100),100,(100),100,(100),100,(1000),0};
unsigned long pentuple_long_click[] = {(100),100,(100),100,(100),100,(100),100,(100),100,(1000),0};

unsigned long long_click[] = {(7000),0};

struct test_step {
  uint8_t button_mask;
  unsigned long *click_sequence;
  unsigned long w;
};

static unsigned long long int last_action;
struct test_step test_vector[] = {
  {0x1, long_click,   4000},
  {0x1, double_click, 7000},
  {0x2, long_click,   1000},
  {0x1, double_click, 4000},
  {0x1, single_click, 1000},
  {0x1, double_click, 8000},
  {0x0, NULL,         0},
};

void loop();
void unit_test1() {
  int test = 0;
  for (;;) {
    if (!test_vector[test].click_sequence)
      break;

    analog_pins ^= test_vector[test].button_mask;
    last_action = millis();
    int i = 0;
    while (test_vector[test].click_sequence[i]) {
      if (millis() - last_action >= test_vector[test].click_sequence[i]) {
        analog_pins ^= test_vector[test].button_mask;
        last_action = millis();
        ++i;
      }
      loop();
    }
    while (millis() - last_action < test_vector[test].w)
      loop();
    Serial.println("---------");
    ++test;
  }
}

void unit_test2() {
  rs[0].move_to_target(0);
  Serial.println("---------");
  for (int i=0; i<1300; ++i) {
    loop();
  }
  rs[0].move_to_target(800);
  Serial.println("---------");
  for (int i=0; i<450; ++i) {
    loop();
  }
  Serial.println("---------");
  rs[0].move_to_target(800);
}

void setup_rs() {
  //id, btn_up, btn_dn, relay_up, relay_dn
  rs[0].setup(0, A0, A1, 7, 8);
  rs[1].setup(1, A2, A3, 9, 10);

  unit_test1();
  unit_test2();
}

