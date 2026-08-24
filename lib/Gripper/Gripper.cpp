#include "Gripper.h"

// Inisialisasi pin servo
void gripperInit(Gripper& g, int pin) {
    if (pin >= 0) {
        g.pin = pin;
        g.servo.setPeriodHertz(50);
        g.servo.attach(g.pin, 500, 2500);
    }
    // Posisi awal di tengah (90 derajat)
    gripperSetAngle(g, 90);
}

// Konfigurasi lengkap servo
void gripperSetConfig(Gripper& g, bool invert, int offset, int openAngle, int closeAngle) {
    g.invert = invert;
    g.offset = offset;
    
    // Batasi sudut buka dan tutup agar selalu 0 - 180 derajat
    if (openAngle < 0) openAngle = 0;
    if (openAngle > 180) openAngle = 180;
    g.openAngle = openAngle;

    if (closeAngle < 0) closeAngle = 0;
    if (closeAngle > 180) closeAngle = 180;
    g.closeAngle = closeAngle;

    // Terapkan ke sudut saat ini
    gripperSetAngle(g, g.currentAngle);
}

// Ubah arah putar (invert) saja
void gripperSetInvert(Gripper& g, bool invert) {
    g.invert = invert;
    gripperSetAngle(g, g.currentAngle); // Perbarui posisi servo dengan arah baru
}

// Gerakkan servo ke sudut tertentu (0 sampai 180)
void gripperSetAngle(Gripper& g, int angle) {
    // 1. Batasi sudut input antara 0 - 180 derajat
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;
    g.currentAngle = angle;

    // 2. Rumus arah: jika dibalik (invert), target = 180 - sudut
    int target = g.currentAngle;
    if (g.invert == true) {
        target = 180 - g.currentAngle;
    }

    // 3. Tambahkan offset kalibrasi
    target = target + g.offset;

    // 4. Pastikan target akhir tidak melebihi 0 - 180 derajat
    if (target < 0) target = 0;
    if (target > 180) target = 180;

    // 5. Kirim perintah gerak ke servo
    if (g.pin >= 0 && g.servo.attached()) {
        g.servo.write(target);
    }
}

// Buka gripper ke sudut openAngle
void gripperOpen(Gripper& g) {
    gripperSetAngle(g, g.openAngle);
}

// Tutup gripper ke sudut closeAngle
void gripperClose(Gripper& g) {
    gripperSetAngle(g, g.closeAngle);
}

// Cek apakah kondisi saat ini lebih dekat ke buka atau tutup
bool gripperIsOpen(const Gripper& g) {
    int selisihBuka = abs(g.currentAngle - g.openAngle);
    int selisihTutup = abs(g.currentAngle - g.closeAngle);
    return (selisihBuka <= selisihTutup);
}

// Tukar posisi: jika buka -> tutup, jika tutup -> buka
void gripperToggle(Gripper& g) {
    if (gripperIsOpen(g)) {
        gripperClose(g);
    } else {
        gripperOpen(g);
    }
}

// Cek apakah pin servo sudah valid (terpasang)
bool gripperIsValid(const Gripper& g) {
    return (g.pin >= 0);
}

// Ambil sudut saat ini
int gripperGetAngle(const Gripper& g) {
    return g.currentAngle;
}