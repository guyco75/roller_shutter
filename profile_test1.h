#define RS_ARRAY_SIZE (2)
struct roller_shutter rs[RS_ARRAY_SIZE];

unsigned long xx[] = {0x0};
unsigned long uu[] = {0x1};
unsigned long dd[] = {0x2};

unsigned long single_click[] = {4000,(100),0};
unsigned long double_click[] = {100,(100),100,(100),0};
unsigned long triple_click[] = {100,(100),100,(100),100,(100),0};
unsigned long quadruple_click[] = {100,(100),100,(100),100,(100),100,(100),0};
unsigned long pentuple_click[] = {100,(100),100,(100),100,(100),100,(100),100,(100),0};

unsigned long triple_long_click[] = {100,(100),100,(100),100,(100),100,(1000),0};
unsigned long quadruple_long_click[] = {100,(100),100,(100),100,(100),100,(100),100,(1000),0};
unsigned long pentuple_long_click[] = {100,(100),100,(100),100,(100),100,(100),100,(100),100,(1000),0};

unsigned long long_click[] = {4000,(7000),0};

static unsigned long last_action;
unsigned long *test_vector[][2] = {
  {uu, long_click},
  {uu, double_click},
  {dd, long_click},
  {uu, double_click},
  {uu, single_click},
  {uu, double_click},
  {xx, NULL},
};

uint8_t p;

void loop();
void unit_test1() {
  int test = 0;
  for (;;) {
    if (test_vector[test][0][0]) {
      last_action = millis();
      int i = 0;
      while (test_vector[test][1][i]) {
        if (millis() - last_action >= test_vector[test][1][i]) {
          p ^= test_vector[test][0][0];
          last_action = millis();
          ++i;
        }
        loop();
      }
      for (int t=0; t<100; ++t) {
        loop();
      }
      Serial.println("---------");
      ++test;
    } else {
      break;
    }
  }
}

void setup_rs() {
  p = 0xF;

  //pin port, btn_up, btn_dn, relay_up, relay_dn
  rs[0].setup(0, &p, 0, 1, 7, 8);
  rs[1].setup(1, &p, 2, 3, 9, 10);

  unit_test1();
}