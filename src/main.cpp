#include <Arduino.h>
#include <WiFi.h>
#include "ServoMgr.h"
#include "WSMgr.h"

void setup() {
    Serial.begin(115200);

    // Inisialisasi servo
    servoInit();

    // Hotspot WiFi ESP32
    WiFi.softAP("RTOS-setup", "");
    Serial.print("Web Controller siap di: http://");
    Serial.println(WiFi.softAPIP());

    // Jalankan Web Server
    wsMgrBegin();
}

void loop() {
}