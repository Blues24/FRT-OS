#pragma once

#include <Arduino.h>
#include "Gripper.h"

const int SERVO_COUNT = 6;

enum ServoID {
    GRIP1_BIG = 0,
    GRIP1_SMALL1 = 1,
    GRIP1_SMALL2 = 2,
    GRIP2_BIG = 3,
    GRIP2_SMALL1 = 4,
    GRIP2_SMALL2 = 5
};

void servoInit();

Gripper& servoGet(int index);

void servoOpenAll();
void servoCloseAll();
void servoSetAllAngle(int angle);

void servoOpen(int index);
void servoClose(int index);
void servoToggle(int index);
void servoSetAngle(int index, int angle);
void servoSetConfig(int index, bool invert, int offset, int openAngle, int closeAngle);

// Set invert saja tanpa ubah open/close angle
void servoSetInvert(int index, bool invert);

void servoOpenGripper(int unit);
void servoCloseGripper(int unit);

int servoGetAngle(int index);
bool servoIsOpen(int index);
