#include <Romi32U4.h>
#include <PololuRPiSlave.h>

#define I2C_ADDRESS 0x14

struct Commands {
  int16_t left_speed, right_speed;
  int16_t left_dist, right_dist;
  bool r_led, g_led, y_led;
} __attribute__((packed));

struct Telemetry {
  int16_t l_enc, r_enc;
  int16_t rem_left;
  int16_t rem_right;
  int16_t cmd_left_dist;
  int16_t cmd_right_dist;
  int16_t cmd_left_speed;
  int16_t cmd_right_speed;
  int16_t set_left_speed;
  int16_t set_right_speed;
  uint16_t batteryMillivolts;
  bool button_A;
  bool button_B;
  bool button_C;
} __attribute__((packed));

struct Data {
  Commands cmd;
  Telemetry telem;
} __attribute__((packed));

PololuRPiSlave<Data,5> slave;
Romi32U4Motors motors;
Romi32U4Encoders encoders;
Romi32U4ButtonA button_A;
Romi32U4ButtonB button_B;
Romi32U4ButtonC button_C;

int16_t prev_left_dist = 0;
int16_t prev_right_dist = 0;
int16_t start_l_enc = 0;
int16_t start_r_enc = 0;
int16_t rem_l_dist = 0;
int16_t rem_r_dist = 0;

int16_t update_left_motor(int16_t dist, int16_t speed) {
  int16_t set_speed = speed;

  if(dist != prev_left_dist) {
    start_l_enc = encoders.getCountsLeft();
    rem_l_dist = dist;
    prev_left_dist = dist;
  }

  if(rem_l_dist != 0) {
    int16_t traveled = encoders.getCountsLeft() - start_l_enc;

    if(dist >= 0) {
      rem_l_dist = max((int16_t)(dist - traveled), (int16_t) 0);
    }
    else {
      rem_l_dist = min((int16_t)(dist - traveled), (int16_t) 0);
    }
  }

  if(set_speed == 0) {
    int16_t abs_dist = abs(rem_l_dist);

    if(abs_dist <= 0) {
        set_speed = 0;
    }
    else if(abs_dist < 10) {
        set_speed = 30;
    }
    else if(abs_dist < 25) {
        set_speed = 50;
    }
    else if(abs_dist < 100) {
        set_speed = 100;
    }
    else if(abs_dist < 300) {
        set_speed = abs_dist;
    }
    else {
        set_speed = 300;
    }
  }

  if(speed == 0) {
    if(rem_l_dist == 0) {
      set_speed = 0;
    }
    else {
      set_speed = (dist >= 0 ? abs(set_speed) : -abs(set_speed));
    }
  }


  motors.setLeftSpeed(set_speed);
  return set_speed;
}

int16_t update_right_motor(int16_t dist, int16_t speed) {
  int16_t set_speed = speed;

  if(dist != prev_right_dist) {
    start_r_enc = encoders.getCountsRight();
    rem_r_dist = dist;
    prev_right_dist = dist;
  }

  if(rem_r_dist != 0) {
    int16_t traveled = encoders.getCountsRight() - start_r_enc;

    if(dist >= 0) {
      rem_r_dist = max((int16_t)(dist - traveled), (int16_t) 0);
    }
    else {
      rem_r_dist = min((int16_t)(dist - traveled), (int16_t) 0);
    }
  }

  if(set_speed == 0) {
    int16_t abs_dist = abs(rem_r_dist);

    if(abs_dist <= 0) {
        set_speed = 0;
    }
    else if(abs_dist < 10) {
        set_speed = 30;
    }
    else if(abs_dist < 25) {
        set_speed = 50;
    }
    else if(abs_dist < 100) {
        set_speed = 100;
    }
    else if(abs_dist < 300) {
        set_speed = abs_dist;
    }
    else {
        set_speed = 300;
    }
  }

  if(speed == 0) {
    if(rem_r_dist == 0) {
      set_speed = 0;
    }
    else {
      set_speed = (dist >= 0 ? abs(set_speed) : -abs(set_speed));
    }
  }


  motors.setRightSpeed(set_speed);
  return set_speed;
}

void setup() {
  slave.init(I2C_ADDRESS);
}

void loop() {
  slave.updateBuffer();
  auto &c = slave.buffer.cmd;
  auto &t = slave.buffer.telem;

  int16_t set_left = update_left_motor( c.left_dist, c.left_speed);
  int16_t set_right = update_right_motor(c.right_dist, c.right_speed);

  ledRed(c.r_led);
  ledGreen(c.g_led);
  ledYellow(c.y_led);

  t.l_enc = encoders.getCountsLeft();
  t.r_enc = encoders.getCountsRight();
  t.rem_left = rem_l_dist;
  t.rem_right = rem_r_dist;

  t.cmd_left_dist = c.left_dist;
  t.cmd_right_dist = c.right_dist;
  t.cmd_left_speed = c.left_speed;
  t.cmd_right_speed = c.right_speed;
  t.set_left_speed = set_left;
  t.set_right_speed = set_right;

  t.batteryMillivolts = readBatteryMillivolts();
  t.button_A = button_A.getSingleDebouncedPress();
  t.button_B = button_B.getSingleDebouncedPress();
  t.button_C = button_C.getSingleDebouncedPress();

  slave.finalizeWrites();
}