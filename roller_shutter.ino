
#define SERIAL_CMD_BUF_SIZE (80)
#include "serial_parser/serial_parser.h"
serial_parser ser_parser;

#include "roller_shutter.h"

#include "profile_test1.h"

void handle_serial_cmd() {
  uint32_t cmd, i, percentage;

  cmd = ser_parser.get_next_token_int();
  switch (cmd) {
#ifdef RS_ARRAY_SIZE
    case 0: // roller_shutter
      i = ser_parser.get_next_token_int();
      if (0 > i || i >= RS_ARRAY_SIZE) {Serial.println("${\"status\":\"ERR roller shutter id\"}#");return;}
      percentage = ser_parser.get_next_token_int();
      if (!rs[i].move_to_target(percentage)) {Serial.println("${\"status\":\"ERR percentage\"}#");return;}
      break;
#endif
    default:
      Serial.println("${\"status\":\"ERR roller shutter cmd\"}#");return;
  }
}

void setup() {
  Serial.begin(57600);
  Serial.println("--Ready--");

#ifdef RS_ARRAY_SIZE
  setup_rs();
#endif
}

void loop() {
  if (ser_parser.process_serial()) {
    handle_serial_cmd();
  }
#ifdef RS_ARRAY_SIZE
  for (int i=0; i<RS_ARRAY_SIZE; ++i)
    rs[i].fsm();
#endif
  delay(10);
}

