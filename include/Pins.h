#pragma once

#include <cstdint>

// Motor pins: {maju, mundur}
const uint8_t FRONT_LEFT_MOTOR_PIN[2]  = {0, 0};
const uint8_t FRONT_RIGHT_MOTOR_PIN[2] = {0, 0};
const uint8_t BACK_LEFT_MOTOR_PIN[2]   = {0, 0};
const uint8_t BACK_RIGHT_MOTOR_PIN[2]  = {0, 0};

// Gripper 1
const uint8_t GRIP1_BIG_PIN    = 0;
const uint8_t GRIP1_SMALL1_PIN = 0;
const uint8_t GRIP1_SMALL2_PIN = 0;

// Gripper 2
const uint8_t GRIP2_BIG_PIN    = 0;
const uint8_t GRIP2_SMALL1_PIN = 0;
const uint8_t GRIP2_SMALL2_PIN = 0;

const uint8_t BUZZER_PIN = 2;

const int GRIPPER_PINS[6] = {
    GRIP1_BIG_PIN, GRIP1_SMALL1_PIN, GRIP1_SMALL2_PIN,
    GRIP2_BIG_PIN, GRIP2_SMALL1_PIN, GRIP2_SMALL2_PIN
};

// PWM channels
const uint8_t FRONT_LEFT_MOTOR_CH  = 1;
const uint8_t FRONT_RIGHT_MOTOR_CH = 2;
const uint8_t BACK_LEFT_MOTOR_CH   = 3;
const uint8_t BACK_RIGHT_MOTOR_CH  = 4;

const uint32_t MOTOR_FREQ = 10000;