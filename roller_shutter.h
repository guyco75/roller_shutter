#ifndef ROLLER_SHUTTER_H
#define ROLLER_SHUTTER_H

/*
 * Prerequisites (things that should be included/defined before including this file):
 *   - button2.h (https://github.com/guyco75/button2)
 *   - serial_parser.h (https://github.com/guyco75/serial_parser)
 *     - define sizes for SERIAL_CMD_BUF_SIZE and SERIAL_OUT_BUF_SIZE
 *   - #define RS_ARRAY_SIZE (<number of roller shutters to instantiate>)
 *
 *   Take a look at the example usage file
 */

enum rs_fsm_state {
  RS_FSM_IDLE,
  RS_FSM_MOVE_TO_TARGET,
  RS_FSM_STEP,
};
static const char *rs_fsm_state_names[] = {
  "RS_FSM_IDLE",
  "RS_FSM_MOVE_TO_TARGET",
  "RS_FSM_STEP",
};

enum rs_direction {
  RS_DIR_NONE,
  RS_DIR_UP,
  RS_DIR_DOWN,
};
static const char *rs_direction_names[] = {
  "NONE",
  "UP",
  "DOWN",
};

struct roller_shutter {
  struct button btn_up, btn_dn;
  uint8_t relay_pin_up, relay_pin_dn;
  uint8_t rs_id;
  uint32_t duration_up, duration_down;     // in milliseconds
  enum rs_direction dir;
  enum rs_fsm_state state;
  int16_t percentage;                      // 0-1000 (for 0.0% - 100.0%)
  int16_t start_percentage;
  bool percentage_known;
  int16_t minp, maxp;
  unsigned long last_percentage_update;    // in micros
  unsigned long start_move;                // in micros
  unsigned long time_to_move;              // in micros

  void setup(uint8_t id, uint8_t btn_up_pin, uint8_t btn_dn_pin, uint8_t relay_up, uint8_t relay_dn, uint32_t dur_up, uint32_t dur_down) {
    rs_id = id;
    duration_up = dur_up;
    duration_down = dur_down;
    state = RS_FSM_IDLE;
    dir = RS_DIR_NONE;

    btn_up.setup(btn_up_pin);
    btn_dn.setup(btn_dn_pin);

    relay_pin_up = relay_up;
    relay_pin_dn = relay_dn;

    rs_command(RS_DIR_NONE);
    pinMode(relay_pin_up, OUTPUT);
    pinMode(relay_pin_dn, OUTPUT);

    percentage = minp = maxp = 0;
    percentage_known = false;
    report_percentage();
  }

  void rs_command(enum rs_direction d) {
    digitalWrite(relay_pin_up, HIGH);
    digitalWrite(relay_pin_dn, HIGH);
    switch (d) {
      case RS_DIR_UP:
        digitalWrite(relay_pin_up, LOW);
        break;
      case RS_DIR_DOWN:
        digitalWrite(relay_pin_dn, LOW);
        break;
    }
    serial_out("${'msg':'rs-update','id':'%d','dir':'%s'}#", rs_id, rs_direction_names[d]);
  }

  void change_fsm_state(enum rs_fsm_state st, enum rs_direction d) {
#if 0
    if (state == RS_FSM_IDLE) {
      serial_out("\n%llu\t\t\t%s --> %s     %d", millis(), rs_fsm_state_names[state], rs_fsm_state_names[st], d);
    }
#endif

    if (state != RS_FSM_IDLE) {
      update_percentage(true);
    }

    if (st != RS_FSM_IDLE) {
      last_percentage_update = start_move = micros();
      start_percentage = percentage;
    }

    if (d != dir)
      rs_command(d);

    state = st;
    dir = d;
  }

  void report_percentage() {
    if (percentage_known) {
      serial_out("${'msg':'rs-update','id':'%d','p':'%d'}#", rs_id, percentage);
    } else {
      serial_out("${'msg':'rs-update','id':'%d','p':'unknown'}#", rs_id);
    }
  }

  void update_percentage(bool force) {
    unsigned long now_micros = micros();
    if (force || now_micros - last_percentage_update >= 1000000) { // report if forced or every 1 sec
      last_percentage_update = now_micros;
      int16_t p = (now_micros - start_move) / (dir==RS_DIR_UP ? duration_up : duration_down);   // (time elapsed) / (time for full move)

      if (percentage_known) {
        if (dir == RS_DIR_UP) {
          percentage = min(1000, start_percentage + p);
        } else {
          percentage = max(0, start_percentage - p);
        }
        report_percentage();
      } else {
        if (dir == RS_DIR_UP) {
          percentage = start_percentage + p;
          maxp = max(maxp, percentage);
        } else {
          percentage = start_percentage - p;
          minp = min(minp, percentage);
        }
        if (maxp - minp >= 1010) {
          percentage = start_percentage = (dir == RS_DIR_UP ? 1000 : 0);
          percentage_known = true;
          report_percentage();
        }
      }
#if 0
      if (percentage_known) {
        for (int i = 0; i < percentage/10; i++)
          Serial.print("*");
        serial_out(" %d%s", percentage, final?" (f)":"");
      } else {
        serial_out("percentage = %d\t\tminp = %d\t\tmaxp = %d", percentage, minp, maxp);
      }
#endif
    }
  }

  void fsm() {
    enum scene btn_up_state = btn_up.read_state();
    enum scene btn_dn_state = btn_dn.read_state();

    if (state == RS_FSM_IDLE && btn_up_state == SCENE_NONE && btn_dn_state == SCENE_NONE)
      return;

    if (btn_up_state == SCENE_DOUBLE_CLICK_DONE) {
      move_to_target(1000);

    } else if (btn_up_state == SCENE_LONG_CLICK) {
      if (state != RS_FSM_STEP) {
        change_fsm_state(RS_FSM_STEP, RS_DIR_UP);
      }
      update_percentage(false);

    } else if (btn_dn_state == SCENE_DOUBLE_CLICK_DONE) {
      move_to_target(0);

    } else if (btn_dn_state == SCENE_LONG_CLICK) {
      if (state != RS_FSM_STEP) {
        change_fsm_state(RS_FSM_STEP, RS_DIR_DOWN);
      }
      update_percentage(false);

    } else if (state == RS_FSM_MOVE_TO_TARGET && btn_up_state == SCENE_NONE && btn_dn_state == SCENE_NONE) {
      if (micros() - start_move < time_to_move) {
        update_percentage(false);
      } else {
        change_fsm_state(RS_FSM_IDLE, RS_DIR_NONE);
      }

    } else {
      if (state != RS_FSM_IDLE) {
        change_fsm_state(RS_FSM_IDLE, RS_DIR_NONE);
      }
    }
  }

  bool move_to_target(int32_t p) {
      int16_t percent_delta;
      enum rs_direction d;

      if (state != RS_FSM_IDLE)
        update_percentage(true);

      if (percentage_known) {
        if ((percentage < p && p < 1000) || p == 1000) {
          d = RS_DIR_UP;
          percent_delta = p - percentage;
        } else if ((0 < p && p < percentage) || p == 0) {
          d = RS_DIR_DOWN;
          percent_delta = percentage - p;
        } else {
          return false;       // out of range or percentage==p already
        }
      } else {
        if (p == 1000) {
          d = RS_DIR_UP;
          percent_delta = 1000;
        } else if (p == 0) {
          d = RS_DIR_DOWN;
          percent_delta = 1000;
        } else {
          return false;       // only 0 or 1000 are allowed
        }
      }

      time_to_move = (unsigned long)percent_delta * (d==RS_DIR_UP ? duration_up : duration_down);

      if (p == 1000 || p == 0) {
        time_to_move += 2000000;        // add extra 2 seconds
      }

      change_fsm_state(RS_FSM_MOVE_TO_TARGET, d);
      return true;
  }

  static void handle_serial_cmd();
};

struct roller_shutter rs[RS_ARRAY_SIZE];

void roller_shutter::handle_serial_cmd() {
  int32_t percentage, id;

  if (!ser_parser.get_next_token_int(&id) || id < 0 || RS_ARRAY_SIZE <= id) {
    Serial.println("${\"status\":\"ERR roller shutter: id\"}#");
    return;
  }

  if (rs[id].state != RS_FSM_IDLE && rs[id].state != RS_FSM_MOVE_TO_TARGET) {
    Serial.println("${\"status\":\"ERR roller shutter: state\"}#");
    return;
  }

  if (!ser_parser.get_next_token_int(&percentage) || !rs[id].move_to_target(percentage)) {
    Serial.println("${\"status\":\"ERR roller shutter: percentage\"}#");
    return;
  }
}

#endif

