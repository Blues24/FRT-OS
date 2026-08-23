#ifndef RAGGEDYPINS_H
#define RAGGEDYPINS_H

#include <cstdint>

// @example posisi_motor[2] = {pin maju, pin mundur}
const uint8_t FRONT_LEFT_MOTOR_PIN[2] = {16, 2};
const uint8_t FRONT_RIGHT_MOTOR_PIN[2] = {15, 4};
const uint8_t BACK_LEFT_MOTOR_PIN[2] = {12, 3};
const uint8_t BACK_RIGHT_MOTOR_PIN[2] = {17, 6};

// Buzzer pin
const uint8_t BUZZER_PIN = 4; // dipindah agar tidak tabrakan dengan servo/motor

// --- Servo & Gripper ---
// Gripper 1 (unit 1 - Depan): 3 servo
const uint8_t GRIP1_BIG_PIN    = 13;   // Servo Elbow Front
const uint8_t GRIP1_SMALL1_PIN = 12;   // Gripper Front Right
const uint8_t GRIP1_SMALL2_PIN = 14;   // Gripper Front Left

// Gripper 2 (unit 2 - Belakang): 3 servo
const uint8_t GRIP2_BIG_PIN    = 27;   // Servo Elbow Back
const uint8_t GRIP2_SMALL1_PIN = 26;   // Gripper Back Right
const uint8_t GRIP2_SMALL2_PIN = 25;   // Gripper Back Left

// Array untuk akses berdasarkan index (0-5)
// Urutan: [Elbow Front, Front Right, Front Left, Elbow Back, Back Right, Back Left]
const uint8_t GRIPPER_PINS[6] = {
    GRIP1_BIG_PIN,       // [0] Servo Elbow Front
    GRIP1_SMALL1_PIN,    // [1] Gripper Front Right
    GRIP1_SMALL2_PIN,    // [2] Gripper Front Left
    GRIP2_BIG_PIN,       // [3] Servo Elbow Back
    GRIP2_SMALL1_PIN,    // [4] Gripper Back Right
    GRIP2_SMALL2_PIN     // [5] Gripper Back Left
};

// --- OLED Display (I2C) ---
const uint8_t OLED_SDA_PIN = 21;
const uint8_t OLED_SCL_PIN = 23;

#endif // RAGGEDYPINS_H