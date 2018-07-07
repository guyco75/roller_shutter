#ifndef ROLLER_SHUTTER_H
#define ROLLER_SHUTTER_H

char rs_str[120];

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
  uint8_t  rs_id;
  enum rs_direction dir;
  enum rs_fsm_state state;
  int16_t percentage;
  int16_t start_percentage;
  bool percentage_known;
  unsigned long last_percentage_update;
  int16_t minp, maxp;
  unsigned long start_move;
  unsigned long time_to_move;

  void setup(uint8_t id, uint8_t btn_up_pin, uint8_t btn_dn_pin, uint8_t relay_up, uint8_t relay_dn) {
    rs_id = id;
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

    snprintf(rs_str, sizeof(rs_str), "${'msg':'rs-update','id':'%d','dir':'%s'}#", rs_id, rs_direction_names[d]);
    Serial.println(rs_str);
  }

  void change_fsm_state(enum rs_fsm_state st, enum rs_direction d) {
#if 0
    if (state == RS_FSM_IDLE) {
      snprintf(rs_str, sizeof(rs_str), "\n%lu\t\t\t%s --> %s     %d", millis(), rs_fsm_state_names[state], rs_fsm_state_names[st], d);
      Serial.println(rs_str);
    }
#endif

    if (state != RS_FSM_IDLE) {
      update_percentage(true);
    }

    if (st != RS_FSM_IDLE) {
      start_move = millis();
      start_percentage = percentage;
      last_percentage_update = start_move;
    }

    if (d != dir)
      rs_command(d);

    state = st;
    dir = d;
  }

  void report_percentage() {
    if (percentage_known) {
      snprintf(rs_str, sizeof(rs_str), "${'msg':'rs-update','id':'%d','p':'%d'}#", rs_id, percentage);
    } else {
      snprintf(rs_str, sizeof(rs_str), "${'msg':'rs-update','id':'%d','p':'unknown'}#", rs_id);
    }
    Serial.println(rs_str);
  }

  void update_percentage(bool final) {
    unsigned long now = millis();
    if (final || now - last_percentage_update >= 1000) {
      int16_t p = (now - start_move) / 10;
      last_percentage_update = now;

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
        snprintf(rs_str, sizeof(rs_str), " %d%s", percentage, final?" (f)":"");
        Serial.println(rs_str);
      } else {
        snprintf(rs_str, sizeof(rs_str), "percentage = %d\t\tminp = %d\t\tmaxp = %d", percentage, minp, maxp);
        Serial.println(rs_str);
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
      time_to_move = 1100 * 10;   //TODO
      change_fsm_state(RS_FSM_MOVE_TO_TARGET, RS_DIR_UP);

    } else if (btn_up_state == SCENE_LONG_CLICK) {
      if (state != RS_FSM_STEP) {
        change_fsm_state(RS_FSM_STEP, RS_DIR_UP);
      }
      update_percentage(false);

    } else if (btn_dn_state == SCENE_DOUBLE_CLICK_DONE) {
      time_to_move = 1100 * 10;   //TODO
      change_fsm_state(RS_FSM_MOVE_TO_TARGET, RS_DIR_DOWN);

    } else if (btn_dn_state == SCENE_LONG_CLICK) {
      if (state != RS_FSM_STEP) {
        change_fsm_state(RS_FSM_STEP, RS_DIR_DOWN);
      }
      update_percentage(false);

    } else if (state == RS_FSM_MOVE_TO_TARGET && btn_up_state == SCENE_NONE && btn_dn_state == SCENE_NONE) {
      if (millis() - start_move < time_to_move) {
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
      if (state != RS_FSM_IDLE && state != RS_FSM_MOVE_TO_TARGET)
        return true;

      if (state != RS_FSM_IDLE)
        update_percentage(true);

      if (percentage_known) {
        if (p > percentage) {
          if (p == 1000) {
            time_to_move = 1100 * 10;   //TODO
          } else if (0 < p && p < 1000) {
            time_to_move = (p - percentage) * 10;
          } else {
            return false;
          }
          change_fsm_state(RS_FSM_MOVE_TO_TARGET, RS_DIR_UP);
        } else if (p < percentage) {
          if (p == 0) {
            time_to_move = 1100 * 10;   //TODO
          } else if (0 < p && p < 1000) {
            time_to_move = (percentage - p) * 10;
          } else {
            return false;
          }
          change_fsm_state(RS_FSM_MOVE_TO_TARGET, RS_DIR_DOWN);
        }
      } else {
        if (p == 1000) {
          time_to_move = 1100 * 10;   //TODO
          change_fsm_state(RS_FSM_MOVE_TO_TARGET, RS_DIR_UP);
        } else if (p == 0) {
          time_to_move = 1100 * 10;   //TODO
          change_fsm_state(RS_FSM_MOVE_TO_TARGET, RS_DIR_DOWN);
        } else {
          return false;
        }
      }
      return true;
  }

  static void handle_serial_cmd(roller_shutter *rs, uint8_t arr_size) {
    int32_t percentage, id;

    if (!ser_parser.get_next_token_int(&id) || id < 0 || arr_size <= id) {
      Serial.println("${\"status\":\"ERR roller shutter id\"}#");
      return;
    }

    if (!ser_parser.get_next_token_int(&percentage) || !rs[id].move_to_target(percentage)) {
      Serial.println("${\"status\":\"ERR percentage\"}#");
      return;
    }
  }
};

struct roller_shutter rs[RS_ARRAY_SIZE];

#endif

