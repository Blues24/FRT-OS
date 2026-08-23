#include "ServoMgr.h"
#include "Pins.h"

static Gripper grippers[SERVO_COUNT];

static bool isValidIndex(int index) {
    return index >= 0 && index < SERVO_COUNT;
}

static int unitOffset(int unit) {
    return (unit == 2) ? 3 : 0;
}

void servoInit() {
    for (int i = 0; i < SERVO_COUNT; ++i) {
        gripperInit(grippers[i], GRIPPER_PINS[i]);
    }
}

Gripper& servoGet(int index) {
    if (!isValidIndex(index)) {
        return grippers[0];
    }
    return grippers[index];
}

void servoOpenAll() {
    for (int i = 0; i < SERVO_COUNT; ++i) {
        if (gripperIsValid(grippers[i])) gripperOpen(grippers[i]);
    }
}

void servoCloseAll() {
    for (int i = 0; i < SERVO_COUNT; ++i) {
        if (gripperIsValid(grippers[i])) gripperClose(grippers[i]);
    }
}

void servoSetAllAngle(int angle) {
    for (int i = 0; i < SERVO_COUNT; ++i) {
        if (gripperIsValid(grippers[i])) gripperSetAngle(grippers[i], angle);
    }
}

void servoOpenGripper(int unit) {
    int base = unitOffset(unit);
    for (int i = 0; i < 3; ++i) {
        if (gripperIsValid(grippers[base + i])) gripperOpen(grippers[base + i]);
    }
}

void servoCloseGripper(int unit) {
    int base = unitOffset(unit);
    for (int i = 0; i < 3; ++i) {
        if (gripperIsValid(grippers[base + i])) gripperClose(grippers[base + i]);
    }
}

void servoOpen(int index) {
    if (isValidIndex(index)) gripperOpen(grippers[index]);
}

void servoClose(int index) {
    if (isValidIndex(index)) gripperClose(grippers[index]);
}

void servoToggle(int index) {
    if (isValidIndex(index)) gripperToggle(grippers[index]);
}

void servoSetAngle(int index, int angle) {
    if (isValidIndex(index)) gripperSetAngle(grippers[index], angle);
}

void servoSetConfig(int index, bool invert, int offset, int openAngle, int closeAngle) {
    if (isValidIndex(index)) {
        gripperSetConfig(grippers[index], invert, offset, openAngle, closeAngle);
    }
}

int servoGetAngle(int index) {
    return isValidIndex(index) ? gripperGetAngle(grippers[index]) : -1;
}

bool servoIsOpen(int index) {
    return isValidIndex(index) && gripperIsOpen(grippers[index]);
}
