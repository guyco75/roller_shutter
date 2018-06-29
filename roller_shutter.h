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
  button btn_up;
  button btn_dn;
  enum rs_direction dir;
  enum rs_fsm_state state;
  int16_t percentage;
  int16_t start_percentage;
  bool percentage_known;
  unsigned long last_percentage_update;
  int16_t minp, maxp;
  unsigned long start_move;
  unsigned long time_to_move;

  void setup(volatile uint8_t *pin_port, uint8_t up_bit, uint8_t dn_bit) {
    btn_up.setup(pin_port, up_bit);
    btn_dn.setup(pin_port, dn_bit);
    dir = RS_DIR_NONE;
    state = RS_FSM_IDLE;
    percentage = minp = maxp = 0;
    percentage_known = false;
  }

  void rs_command() {
    //TODO
  }

  void change_fsm_state(enum rs_fsm_state st, enum rs_direction d) {
    if (state != RS_FSM_IDLE && st != RS_FSM_IDLE) {
      change_fsm_state(RS_FSM_IDLE, RS_DIR_NONE);
    }

#if 1
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

    rs_command(); // in place...
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
#if 1
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
};

#endif

