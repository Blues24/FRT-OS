/**
 * @file main.cpp
 * @brief Entry point FRT-OS dengan arsitektur dual-core ESP32.
 *
 * Distribusi pekerjaan:
 *   - Core 0 (PRO CPU):  WiFi AP + AsyncWebServer (WSMgr). Ringan, I/O-bound,
 *                         menyerahkan event loop ke AsyncTCP.
 *   - Core 1 (APP CPU):  Hardware loop 50 Hz - PS3 -> ControllerMgr
 *                         -> DriveMgr (mecanum) + Buzzer non-blocking.
 *
 * Sinkronisasi lintas-core:
 *   - ControllerMgr::_connectedCache : atomic flag status PS3 connection (lock-free).
 *   - Buzzer mutex (internal)        : melindungi ring buffer nada.
 *
 * Watchdog:
 *   - ESP-IDF Task Watchdog Timer (TWDT): kedua task harus feed dalam 5 detik,
 *     jika tidak ESP32 auto-restart.
 *
 * Modul yang dipakai: Buzzer, ControllerMgr, DriveMgr, WSMgr, ServoMgr, Gripper.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_task_wdt.h>

#include "RaggedyPins.h"
#include "Buzzer.h"
#include "DriveMgr.h"
#include "ControllerMgr.h"
#include "WSMgr.h"
#include "ServoMgr.h"
#include "Gripper.h"

// ============================================================================
// Konfigurasi Task: Priorities & Stacks
// ============================================================================
static constexpr UBaseType_t HW_LOOP_PRIO  = 6;    // Di atas IDF system tasks (timer=2, wifi=3..5)
static constexpr UBaseType_t NET_LOOP_PRIO = 3;    // Di atas idle, cukup untuk periodic logging
static constexpr uint32_t    HW_STACK      = 4096; // 4KB stack untuk hardware loop
static constexpr uint32_t    NET_STACK     = 4096; // 4KB stack untuk WiFi/Serial calls (mencegah overflow)

// ============================================================================
// Konfigurasi WiFi (Access Point mode)
// ============================================================================
static const char* AP_SSID = "FRT-OS";
static const char* AP_PASS = "frt12345";

// ============================================================================
// Konfigurasi PS3 MAC default
// ============================================================================
static const char* PS3_MAC_DEFAULT = "34:f6:4b:30:3c:f6";

// ============================================================================
// Watchdog (TWDT) configuration
// ============================================================================
static constexpr uint32_t WDT_TIMEOUT_S      = 5;    // 5 detik tanpa feed -> reset
static constexpr uint32_t WDT_FEED_PERIOD_MS = 1000; // feed periodik tiap 1 detik

// ============================================================================
// State hardware (instance singleton / static)
// ============================================================================
static DriveMgr        driveInstance;
static DriveMgr&       drive   = driveInstance;
static ControllerMgr&  pad     = ControllerMgr::getInstance();
static Buzzer&         buzzer  = Buzzer::getBuzzerInstance();

// Tabel pin motor (urut FL, FR, BL, BR - sesuai kontrak DriveMgr).
static MotorPins MOTOR_PINS[MOTOR_COUNT];

// Konstanta PWM motor (sesuai DriveMgr.cpp).
static constexpr uint32_t MOTOR_PWM_FREQ = 10000;
static constexpr uint8_t  MOTOR_PWM_RES  = 8;

// ============================================================================
// Task: hardware loop di Core 1 (Application & Hardware Control)
// ============================================================================
static void taskHardwareLoop(void* pvParameters) {
    (void)pvParameters;
    TickType_t lastWake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(20);  // 50 Hz

    bool wasConnected = false;

    for (;;) {
        // ---- Baca status PS3 (thread-safe atomic) & drive motor ----
        bool connected = pad.isConnected();

        if (connected) {
            float lx = 0.0f, ly = 0.0f, rx = 0.0f;
            int16_t speed = 0;
            pad.getDriveInput(lx, ly, rx, speed);

            // Konvensi: ly positif = maju. strafeX = rx, rotationX = lx.
            drive.drive(rx, ly, lx, speed);
            wasConnected = true;
        } else {
            if (wasConnected) {
                drive.requestCoast();
                wasConnected = false;
            }
            drive.drive(0.0f, 0.0f, 0.0f, 0);
        }

        // ---- Tick buzzer non-blocking ----
        buzzer.loopPlayNote();

        // ---- Drift guard: resync jika terlambat >2 siklus ----
        TickType_t now = xTaskGetTickCount();
        if (now - lastWake > period * 2) {
            lastWake = now;
        }

        // ---- Feed watchdog ----
        esp_task_wdt_reset();

        vTaskDelayUntil(&lastWake, period);
    }
}

// ============================================================================
// Task: network & web di Core 0 (System & Communication)
// ============================================================================
static void taskNetworkLoop(void* pvParameters) {
    (void)pvParameters;
    TickType_t lastWake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(WDT_FEED_PERIOD_MS);  // 1 Hz

    uint32_t logCounter = 0;

    for (;;) {
        // Watchdog sederhana: jika SoftAP mati, nyalakan ulang.
        if (WiFi.getMode() == WIFI_OFF || WiFi.softAPgetStationNum() < 0) {
            WiFi.softAP(AP_SSID, AP_PASS);
        }

        // Serial logger setiap 5 detik: status AP & PS3
        if ((logCounter++ % 5) == 0) {
            bool ps3 = pad.isConnected();
            Serial.print("[NET] AP=");
            Serial.print(WiFi.softAPIP());
            Serial.print(" sta=");
            Serial.print(WiFi.softAPgetStationNum());
            Serial.print(" PS3=");
            Serial.println(ps3 ? "connected" : "idle");
        }

        // ---- Feed watchdog ----
        esp_task_wdt_reset();

        vTaskDelayUntil(&lastWake, period);
    }
}

// ============================================================================
// Arduino entry points
// ============================================================================
void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println();
    Serial.println("[BOOT] FRT-OS starting...");

    // ---- Buzzer (Core 1 - di-init di main, diakses dari kedua core) ----
    buzzer.init();

    // ---- Motor (LEDC via core v2 API) ----
    {
        MotorPins fl; fl.pinPWM_A = FRONT_LEFT_MOTOR_PIN[0];  fl.pinPWM_B = FRONT_LEFT_MOTOR_PIN[1];  MOTOR_PINS[MOTOR_IDX_FL] = fl;
        MotorPins fr; fr.pinPWM_A = FRONT_RIGHT_MOTOR_PIN[0]; fr.pinPWM_B = FRONT_RIGHT_MOTOR_PIN[1]; MOTOR_PINS[MOTOR_IDX_FR] = fr;
        MotorPins bl; bl.pinPWM_A = BACK_LEFT_MOTOR_PIN[0];   bl.pinPWM_B = BACK_LEFT_MOTOR_PIN[1];   MOTOR_PINS[MOTOR_IDX_BL] = bl;
        MotorPins br; br.pinPWM_A = BACK_RIGHT_MOTOR_PIN[0];  br.pinPWM_B = BACK_RIGHT_MOTOR_PIN[1];  MOTOR_PINS[MOTOR_IDX_BR] = br;
    }

    drive.MotorInit(MOTOR_PINS, MOTOR_PWM_FREQ, MOTOR_PWM_RES);
    Serial.println("[BOOT] MotorInit done.");

    // ---- Servo + Gripper ----
    servoInit();
    Serial.println("[BOOT] ServoMgr init done.");

    // ---- WiFi AP (Core 0) ----
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    Serial.print("[BOOT] AP siap di: http://");
    Serial.println(WiFi.softAPIP());
    Serial.print("[BOOT] Password: ");
    Serial.println(AP_PASS);

    // ---- Web server + WS (Core 0) ----
    wsMgrBegin();
    Serial.println("[BOOT] Web Controller siap.");

    // ---- PS3 controller (BT task) ----
    pad.initPs3(PS3_MAC_DEFAULT);
    Serial.print("[BOOT] PS3 init, MAC=");
    Serial.println(PS3_MAC_DEFAULT);

    // ---- Spawn task FreeRTOS ----
    TaskHandle_t hwHandle = nullptr;
    TaskHandle_t netHandle = nullptr;

    BaseType_t hwRes = xTaskCreatePinnedToCore(
        taskHardwareLoop, "hw-loop", HW_STACK, nullptr, HW_LOOP_PRIO, &hwHandle, 1
    );
    if (hwRes != pdPASS) {
        Serial.println("[FATAL] Failed to create hw-loop task!");
        ESP.restart();
    }

    BaseType_t netRes = xTaskCreatePinnedToCore(
        taskNetworkLoop, "net-loop", NET_STACK, nullptr, NET_LOOP_PRIO, &netHandle, 0
    );
    if (netRes != pdPASS) {
        Serial.println("[FATAL] Failed to create net-loop task!");
        ESP.restart();
    }

    // ---- Init Watchdog Timer (TWDT) & Subscribe Tasks ----
    esp_task_wdt_init(WDT_TIMEOUT_S, true);  // panic_on_timeout = true
    if (hwHandle)  esp_task_wdt_add(hwHandle);
    if (netHandle) esp_task_wdt_add(netHandle);

    Serial.println("[BOOT] Dual-core tasks spawned + TWDT armed.");
}

void loop() {
    // Idle loop: FreeRTOS tasks handle all work.
    vTaskDelay(pdMS_TO_TICKS(1000));
}
