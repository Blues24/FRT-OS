#ifndef RAGGEDYPINS_H
#define RAGGEDYPINS_H

#include <cstdlib>

// @example posisi_motor[2] = {pin maju, pin mundur}
const uint8_t FRONT_LEFT_MOTOR_PIN[2]  = {2, 0};
const uint8_t FRONT_RIGHT_MOTOR_PIN[2] = {17, 5};
const uint8_t BACK_LEFT_MOTOR_PIN[2]   = {4, 16};
const uint8_t BACK_RIGHT_MOTOR_PIN[2]  = {18, 19};

// Peripheral
const uint8_t FRONT_LEFT_GRIPPER_PIN  = 14;
const uint8_t FRONT_RIGHT_GRIPPER_PIN = 12;
const uint8_t FRONT_MIDDLE_SERVO      = 13;
const uint8_t BACK_MIDDLE_SERVO       = 27;
const uint8_t BACK_LEFT_GRIPER_PIN    = 25;
const uint8_t BACK_RIGHT_GRIPPER_PIN  = 26;
const uint8_t BUZZER_PIN              = 23;

#endif // RAGGEDYPINS_H