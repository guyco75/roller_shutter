#ifndef ROLLER_SHUTTER_H
#define ROLLER_SHUTTER_H

#include "button2/button2.h"

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

struct roller_shutter {
  button btn_up, btn_dn;
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

  void setup(uint8_t id, volatile uint8_t *pin_port, uint8_t up_bit, uint8_t dn_bit, uint8_t relay_up, uint8_t relay_dn) {
    rs_id = id;

    btn_up.setup(pin_port, up_bit);
    btn_dn.setup(pin_port, dn_bit);

    relay_pin_up = relay_up;
    relay_pin_dn = relay_dn;
    digitalWrite(relay_pin_up, HIGH);
    digitalWrite(relay_pin_dn, HIGH);
    pinMode(relay_pin_up, OUTPUT);
    pinMode(relay_pin_dn, OUTPUT);

    dir = RS_DIR_NONE;
    state = RS_FSM_IDLE;
    percentage = minp = maxp = 0;
    percentage_known = false;
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
  }

  void change_fsm_state(enum rs_fsm_state st, enum rs_direction d) {
    if (state != RS_FSM_IDLE && st != RS_FSM_IDLE) {
      change_fsm_state(RS_FSM_IDLE, RS_DIR_NONE);
    }

    rs_command(d);

#if 0
    if (state == RS_FSM_IDLE) {
      snprintf(rs_str, sizeof(rs_str), "\n%lu\t\t\t%s --> %s     %d", millis(), rs_fsm_state_names[state], rs_fsm_state_names[st], d);
      Serial.println(rs_str);
    }
#endif
    if (st == RS_FSM_IDLE) {
      update_percentage(true);
    } else {
      start_move = millis();
      start_percentage = percentage;
      last_percentage_update = start_move;
    }
    state = st;
    dir = d;
  }

  update_percentage(bool final) {
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
        }
      }
      //TODO: report in setup too
      if (percentage_known) {
        snprintf(rs_str, sizeof(rs_str), "${'msg':'rs-update','id':'%d','p':'%d'}#", 0, percentage);
      } else {
        snprintf(rs_str, sizeof(rs_str), "${'msg':'rs-update','id':'%d','p':'unknown'}#", 0);
      }
      Serial.println(rs_str);
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
    //read serial
    //  if RS_FSM_STEP ignore action commands
  }

  bool move_to_target(int32_t p) {
      if (state != RS_FSM_IDLE && state != RS_FSM_MOVE_TO_TARGET)
        return true;

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
};

#endif
