#include "Gripper.h"

void gripperInit(Gripper& g, int pin) {
    if (pin >= 0) {
        g.pin = pin;
    }
    if (g.pin >= 0) {
        g.servo.setPeriodHertz(50);
        g.servo.attach(g.pin, 500, 2500);
    }
    gripperSetAngle(g, 90);
}

void gripperSetConfig(Gripper& g, bool invert, int offset, int openAngle, int closeAngle) {
    g.invert = invert;
    g.offset = offset;
    g.openAngle = constrain(openAngle, 0, 180);
    g.closeAngle = constrain(closeAngle, 0, 180);
    gripperSetAngle(g, g.currentAngle);
}

void gripperSetAngle(Gripper& g, int angle) {
    g.currentAngle = constrain(angle, 0, 180);

    int target = g.invert ? (180 - g.currentAngle) : g.currentAngle;
    target = constrain(target + g.offset, 0, 180);

    if (g.pin >= 0 && g.servo.attached()) {
        g.servo.write(target);
    }
}

void gripperOpen(Gripper& g) {
    gripperSetAngle(g, g.openAngle);
}

void gripperClose(Gripper& g) {
    gripperSetAngle(g, g.closeAngle);
}

void gripperToggle(Gripper& g) {
    if (abs(g.currentAngle - g.closeAngle) < abs(g.currentAngle - g.openAngle)) {
        gripperOpen(g);
    } else {
        gripperClose(g);
    }
}

bool gripperIsOpen(const Gripper& g) {
    return abs(g.currentAngle - g.openAngle) <= abs(g.currentAngle - g.closeAngle);
}

bool gripperIsValid(const Gripper& g) {
    return g.pin >= 0;
}

int gripperGetAngle(const Gripper& g) {
    return g.currentAngle;
}
