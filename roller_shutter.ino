
#define SERIAL_CMD_BUF_SIZE (80)
#define SERIAL_OUT_BUF_SIZE (120)
#include "serial_parser/serial_parser.h"

#include "button2/button2.h"

#define RS_ARRAY_SIZE (2)
#include "roller_shutter.h"

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

  //id, btn_up, btn_dn, relay_up, relay_dn
  rs[0].setup(0, A0, A1, 7, 8);
  rs[1].setup(1, A2, A3, 9, 10);
}

void loop() {
  if (ser_parser.process_serial()) {
    handle_serial_cmd();
  }

  for (int i=0; i<RS_ARRAY_SIZE; ++i)
    rs[i].fsm();

  delay(10);
}

