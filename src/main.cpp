#include <Arduino.h>
#include <WiFi.h>
#include "ServoMgr.h"
#include "WSMgr.h"

// Password WPA2 untuk hotspot (minimal 8 karakter)
static const char* AP_SSID = "FRT-OS";
static const char* AP_PASS = "frt12345";

void setup() {
    Serial.begin(115200);

    // Inisialisasi servo
    servoInit();

    // Hotspot WiFi ESP32 dengan password WPA2
    WiFi.softAP(AP_SSID, AP_PASS);
    Serial.print("Web Controller siap di: http://");
    Serial.println(WiFi.softAPIP());
    Serial.print("Password WiFi: ");
    Serial.println(AP_PASS);

    // Jalankan Web Server
    wsMgrBegin();
}

void loop() {
}