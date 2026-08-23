#ifndef RAGGEDYPINS_H
#define RAGGEDYPINS_H

#include <cstdlib>

// @example posisi_motor[2] = {pin maju, pin mundur}
const uint8_t FRONT_LEFT_MOTOR_PIN[2] = {16, 2};
const uint8_t FRONT_RIGHT_MOTOR_PIN[2] = {15, 4};
const uint8_t BACK_LEFT_MOTOR_PIN[2] = {12, 3};
const uint8_t BACK_RIGHT_MOTOR_PIN[2] = {17, 6};

// Peripheral
const uint8_t FRONT_LEFT_GRIPPER_PIN = 0;
const uint8_t FRONT_RIGHT_GRIPPER_PIN = 1;
const uint8_t BACK_LEFT_GRIPER_PIN = 1;
const uint8_t BACK_RIGHT_GRIPPER_PIN = 2;
const uint8_t BUZZER_PIN = 2;

#endif // RAGGEDYPINS_H