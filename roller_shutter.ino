
#define SERIAL_CMD_BUF_SIZE (80)
#include "serial_parser/serial_parser.h"
serial_parser ser_parser;

#include "button2/button2.h"
#include "roller_shutter.h"

//#include "profile_test1.h"
#include "profile_living_room_balcony.h"

void handle_serial_cmd() {
  char *cmd_str;

  cmd_str = ser_parser.get_next_token();
  if (!strcmp(cmd_str, "rs")) {
    roller_shutter::handle_serial_cmd(rs, RS_ARRAY_SIZE);

  } else if (!strcmp(cmd_str, "status_req")) {
    for (int i=0; i<RS_ARRAY_SIZE; ++i)
      rs[i].report_percentage();
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

