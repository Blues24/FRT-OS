#include "ServoMgr.h"
#include "RaggedyPins.h"
#include "Gripper.h"

// Default servo pins from RaggedyPins.h
static const uint8_t DEFAULT_SERVO_PINS[SERVO_COUNT] = {
    FRONT_LEFT_GRIPPER_PIN,   // 14
    FRONT_MIDDLE_SERVO,       // 13
    FRONT_RIGHT_GRIPPER_PIN,  // 12
    BACK_MIDDLE_SERVO,        // 27
    BACK_LEFT_GRIPER_PIN,     // 25
    BACK_RIGHT_GRIPPER_PIN    // 26
};

// Array untuk menyimpan 6 struct Gripper
static Gripper grippers[SERVO_COUNT];
static uint8_t currentServoPins[SERVO_COUNT];

// Fungsi bantuan untuk memeriksa apakah nomor index (0 - 5) valid
static bool isValidIndex(int index) {
    if (index >= 0 && index < SERVO_COUNT) {
        return true;
    }
    return false;
}

// Menentukan indeks awal untuk gripper depan (unit 1) atau belakang (unit 2)
static int unitOffset(int unit) {
    if (unit == 1) {
        return 0; // Gripper depan: servo index 0, 1, 2
    }
    if (unit == 2) {
        return 3; // Gripper belakang: servo index 3, 4, 5
    }
    return -1; // Nilai selain 1 atau 2 tidak valid
}

// Inisialisasi semua 6 servo dengan pin masing-masing
void servoInit() {
    for (int i = 0; i < SERVO_COUNT; i++) {
        currentServoPins[i] = DEFAULT_SERVO_PINS[i];
        gripperInit(grippers[i], DEFAULT_SERVO_PINS[i]);
    }
}

// Mengambil data gripper berdasarkan index
Gripper& servoGet(int index) {
    if (!isValidIndex(index)) {
        return grippers[0];
    }
    return grippers[index];
}

// Buka semua 6 servo
void servoOpenAll() {
    for (int i = 0; i < SERVO_COUNT; i++) {
        if (gripperIsValid(grippers[i])) {
            gripperOpen(grippers[i]);
        }
    }
}

// Tutup semua 6 servo
void servoCloseAll() {
    for (int i = 0; i < SERVO_COUNT; i++) {
        if (gripperIsValid(grippers[i])) {
            gripperClose(grippers[i]);
        }
    }
}

// Set sudut semua servo ke nilai yang sama
void servoSetAllAngle(int angle) {
    for (int i = 0; i < SERVO_COUNT; i++) {
        if (gripperIsValid(grippers[i])) {
            gripperSetAngle(grippers[i], angle);
        }
    }
}

// Buka gripper berdasarkan unit (1 = depan, 2 = belakang)
void servoOpenGripper(int unit) {
    int base = unitOffset(unit);
    if (base < 0) return; // Jika unit salah, abaikan

    for (int i = 0; i < 3; i++) {
        int idx = base + i;
        if (gripperIsValid(grippers[idx])) {
            gripperOpen(grippers[idx]);
        }
    }
}

// Tutup gripper berdasarkan unit (1 = depan, 2 = belakang)
void servoCloseGripper(int unit) {
    int base = unitOffset(unit);
    if (base < 0) return; // Jika unit salah, abaikan

    for (int i = 0; i < 3; i++) {
        int idx = base + i;
        if (gripperIsValid(grippers[idx])) {
            gripperClose(grippers[idx]);
        }
    }
}

// Buka servo tertentu (0 - 5)
void servoOpen(int index) {
    if (isValidIndex(index)) {
        gripperOpen(grippers[index]);
    }
}

// Tutup servo tertentu (0 - 5)
void servoClose(int index) {
    if (isValidIndex(index)) {
        gripperClose(grippers[index]);
    }
}

// Toggle buka/tutup servo tertentu
void servoToggle(int index) {
    if (isValidIndex(index)) {
        gripperToggle(grippers[index]);
    }
}

// Set sudut servo tertentu
void servoSetAngle(int index, int angle) {
    if (isValidIndex(index)) {
        gripperSetAngle(grippers[index], angle);
    }
}

// Atur konfigurasi lengkap per servo
void servoSetConfig(int index, bool invert, int offset, int openAngle, int closeAngle) {
    if (isValidIndex(index)) {
        gripperSetConfig(grippers[index], invert, offset, openAngle, closeAngle);
    }
}

// Atur invert arah servo saja
void servoSetInvert(int index, bool invert) {
    if (isValidIndex(index)) {
        gripperSetInvert(grippers[index], invert);
    }
}

// Ambil data sudut servo tertentu
int servoGetAngle(int index) {
    if (isValidIndex(index)) {
        return gripperGetAngle(grippers[index]);
    }
    return -1;
}

// Cek apakah servo tertentu sedang terbuka
bool servoIsOpen(int index) {
    if (isValidIndex(index)) {
        return gripperIsOpen(grippers[index]);
    }
    return false;
}

// Reinitialize servo pins at runtime
bool servoReinitPins(const uint8_t pins[SERVO_COUNT]) {
    for (int i = 0; i < SERVO_COUNT; i++) {
        if (gripperIsValid(grippers[i])) {
            grippers[i].servo.detach();
        }
        gripperInit(grippers[i], pins[i]);
        currentServoPins[i] = pins[i];
    }
    return true;
}
