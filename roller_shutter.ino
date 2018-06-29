
#define SERIAL_CMD_BUF_SIZE (80)
#include "serial_parser/serial_parser.h"
serial_parser ser_parser;

#include "roller_shutter.h"

roller_shutter rs;

unsigned long uu[] = {0x1};
unsigned long dd[] = {0x2};

unsigned long single_click[] = {4000,(100),0};
unsigned long double_click[] = {100,(100),100,(100),0};
unsigned long triple_click[] = {100,(100),100,(100),100,(100),0};
unsigned long long_click[] = {4000,(7000),0};

static unsigned long last_action;
unsigned long *test_vector[][2] = {
  {uu, long_click},
  {uu, double_click},
  {dd, long_click},
  {uu, double_click},
  {uu, single_click},
  {uu, double_click},
  NULL,
};

void setup() {
  Serial.begin(57600);
  Serial.println("--Ready--");
  uint8_t p = 3;
  int test = 0;

  //pin port, btn_up, btn_dn, relay_up, relay_dn
  rs.setup(0, &p, 0, 1, 7, 8);

  for (;;) {
    if (test_vector[test]) {
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

void handle_serial_cmd() {
  int32_t cmd, i, percentage;
  roller_shutter *r;

  cmd = ser_parser.get_next_token_int();
  switch (cmd) {
    case 0: // roller_shutter
      i = ser_parser.get_next_token_int();
      if (i != 0) {Serial.println("${\"status\":\"ERR roller shutter id\"}#");return;}
      r = &rs;
      percentage = ser_parser.get_next_token_int();
      if (!r->move_to_target(percentage)) {Serial.println("${\"status\":\"ERR percentage\"}#");return;}
      break;
    default:
      Serial.println("${\"status\":\"ERR roller shutter cmd\"}#");return;
  }
}

void loop() {
  if (ser_parser.process_serial()) {
    handle_serial_cmd();
  }
  rs.fsm();
  delay(10);
}

