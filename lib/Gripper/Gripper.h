#pragma once

#include <Arduino.h>
#include <ESP32Servo.h>

// Struktur data sederhana untuk menyimpan status 1 servo gripper
struct Gripper {
    int pin = -1;
    Servo servo;
    bool invert = false;
    int offset = 0;
    int openAngle = 180;
    int closeAngle = 0;
    int currentAngle = 90;
};

// Fungsi-fungsi operasi gripper (gaya C dasar tanpa ::)
void gripperInit(Gripper& g, int pin = -1);
void gripperSetConfig(Gripper& g, bool invert, int offset, int openAngle, int closeAngle);
void gripperSetInvert(Gripper& g, bool invert);
void gripperSetAngle(Gripper& g, int angle);
void gripperOpen(Gripper& g);
void gripperClose(Gripper& g);
void gripperToggle(Gripper& g);
bool gripperIsOpen(const Gripper& g);
bool gripperIsValid(const Gripper& g);
int gripperGetAngle(const Gripper& g);