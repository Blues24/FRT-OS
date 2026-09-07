# Plan: Dynamic Motor & Servo Pin Configuration via Web UI

## Goal
Add web UI controls in WSMgr to dynamically change motor pins (4 motors × 2 pins each) and servo pins (6 servos) with defaults from `RaggedyPins.h`, persisting changes to NVS/Preferences and applying them at runtime without reboot.

## Current Context / Assumptions
- **Platform**: ESP32 Arduino (PlatformIO), dual-core FreeRTOS
- **Existing**: WSMgr serves HTML UI at `/` and handles `/command` GET endpoint
- **Pin definitions**: `RaggedyPins.h` has 8 motor pins + 6 servo pins + buzzer
- **Motor control**: `DriveMgr::MotorInit()` called once in `setup()` with pins from `RaggedyPins.h`
- **Servo control**: `servoInit()` called once in `setup()` with pins from `RaggedyPins.h` via `GRIPPER_PINS[]`
- **Storage**: ESP32 Preferences (NVS) available for persistence
- **Concurrency**: Motor/servo changes must be thread-safe (Core 1 hardware loop + Core 0 web server)

## Architecture / Approach
1. **Add NVS storage layer** — Preferences namespace `pin-config` for motor/servo pins
2. **Extend DriveMgr** — Add `reinitPins()` method to safely reinitialize motor pins at runtime (mutex-protected)
3. **Extend ServoMgr** — Add `reinitPins()` method to safely reinitialize servo pins at runtime
4. **Extend WSMgr** — Add `/api/pins` GET (current config) and POST (update config) endpoints
5. **Update Web UI** — Add "Pin Config" tab with editable fields for all 14 pins, load/save buttons
6. **Thread safety** — Use FreeRTOS mutex for pin reinit; briefly pause hardware loop during motor reinit

## Step-by-Step Tasks

### Task 1: Add NVS Pin Config Storage (`lib/PinConfig/`)
Create new module for pin configuration persistence.

**Files to create:**
- `lib/PinConfig/PinConfig.h`
- `lib/PinConfig/PinConfig.cpp`

**PinConfig.h:**
```cpp
#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "RaggedyPins.h"

struct MotorPinConfig {
    uint8_t fl_a, fl_b;
    uint8_t fr_a, fr_b;
    uint8_t bl_a, bl_b;
    uint8_t br_a, br_b;
};

struct ServoPinConfig {
    uint8_t pins[6];  // GRIP1_BIG, GRIP1_SMALL1, GRIP1_SMALL2, GRIP2_BIG, GRIP2_SMALL1, GRIP2_SMALL2
};

struct PinConfig {
    MotorPinConfig motors;
    ServoPinConfig servos;
    bool valid = false;
};

class PinConfigMgr {
public:
    static PinConfigMgr& getInstance();
    
    // Load from NVS, fallback to RaggedyPins.h defaults
    PinConfig load();
    
    // Save to NVS
    bool save(const PinConfig& config);
    
    // Reset to defaults (RaggedyPins.h values)
    PinConfig getDefaults();
    
    // Validate pin numbers (0-39, no duplicates for motors)
    bool validate(const PinConfig& config, String& errorMsg);

private:
    PinConfigMgr() = default;
    Preferences prefs;
    static constexpr const char* NS = "pin-config";
    static constexpr const char* KEY_MOTORS = "motors";
    static constexpr const char* KEY_SERVOS = "servos";
    static constexpr const char* KEY_VALID = "valid";
};
```

**PinConfig.cpp:** Implement load/save/validate with Preferences API. Defaults from `RaggedyPins.h`.

---

### Task 2: Extend DriveMgr for Runtime Pin Reinit
**File:** `lib/DriveMgr/DriveMgr.h` — add to public section:
```cpp
// Reinitialize motor pins at runtime (thread-safe, pauses drive loop briefly)
bool reinitPins(const MotorPins pins[MOTOR_COUNT], uint32_t freq, uint8_t res);
```

**File:** `lib/DriveMgr/DriveMgr.cpp` — implement:
```cpp
#include <freertos/semphr.h>
static SemaphoreHandle_t driveMutex = nullptr;

void DriveMgr::MotorInit(...) {
    if (!driveMutex) driveMutex = xSemaphoreCreateMutex();
    // ... existing code ...
}

bool DriveMgr::reinitPins(const MotorPins pins[MOTOR_COUNT], uint32_t freq, uint8_t res) {
    if (!driveMutex) return false;
    if (xSemaphoreTake(driveMutex, pdMS_TO_TICKS(100)) != pdTRUE) return false;
    
    // Stop all motors first
    emergencyStop();
    
    // Re-run LEDC setup/attach with new pins
    ledcSetup(CHANNEL_FL_A, freq, res);  // ... all 8 channels
    ledcAttachPin(pins[MOTOR_IDX_FL].pinPWM_A, CHANNEL_FL_A);  // ... all 8 pins
    
    // Update internal pin table
    memcpy(motorPins, pins, sizeof(MotorPins) * MOTOR_COUNT);
    _pwmFreq = freq; _pwmRes = res;
    
    xSemaphoreGive(driveMutex);
    return true;
}
```

Also modify `drive()` and `SetMotorSpeed()` to take the mutex briefly.

---

### Task 3: Extend ServoMgr for Runtime Pin Reinit
**File:** `lib/ServoMgr/ServoMgr.h` — add:
```cpp
bool reinitPins(const uint8_t pins[SERVO_COUNT]);
```

**File:** `lib/ServoMgr/ServoMgr.cpp` — implement:
```cpp
bool servoReinitPins(const uint8_t pins[SERVO_COUNT]) {
    for (int i = 0; i < SERVO_COUNT; i++) {
        if (gripperIsValid(grippers[i])) {
            grippers[i].servo.detach();
        }
        gripperInit(grippers[i], pins[i]);
    }
    return true;
}
```

---

### Task 4: Add WSMgr REST API Endpoints
**File:** `lib/WSMgr/WSMgr.h` — add declarations:
```cpp
void wsMgrBegin();
void handlePinsGet(AsyncWebServerRequest* request);
void handlePinsPost(AsyncWebServerRequest* request);
```

**File:** `lib/WSMgr/WSMgr.cpp` — implement:
```cpp
#include "PinConfig.h"
#include "DriveMgr.h"
#include "ServoMgr.h"

void handlePinsGet(AsyncWebServerRequest* request) {
    PinConfig cfg = PinConfigMgr::getInstance().load();
    String json = "{";
    json += "\"motors\":{\"fl_a\":" + String(cfg.motors.fl_a) + ",\"fl_b\":" + String(cfg.motors.fl_b) + 
            ",\"fr_a\":" + String(cfg.motors.fr_a) + ",\"fr_b\":" + String(cfg.motors.fr_b) +
            ",\"bl_a\":" + String(cfg.motors.bl_a) + ",\"bl_b\":" + String(cfg.motors.bl_b) +
            ",\"br_a\":" + String(cfg.motors.br_a) + ",\"br_b\":" + String(cfg.motors.br_b) + "}";
    json += ",\"servos\":[";  // array of 6
    for (int i = 0; i < 6; i++) {
        json += String(cfg.servos.pins[i]);
        if (i < 5) json += ",";
    }
    json += "]}";
    request->send(200, "application/json", json);
}

void handlePinsPost(AsyncWebServerRequest* request) {
    if (!request->hasParam("data", true)) {
        request->send(400, "text/plain", "missing data");
        return;
    }
    String body = request->getParam("data", true)->value();
    // Parse JSON (use ArduinoJson or manual parse for simple struct)
    PinConfig cfg;
    // ... parse body into cfg ...
    
    String errorMsg;
    if (!PinConfigMgr::getInstance().validate(cfg, errorMsg)) {
        request->send(400, "text/plain", errorMsg);
        return;
    }
    
    // Apply to hardware
    MotorPins motorPins[MOTOR_COUNT];
    motorPins[MOTOR_IDX_FL] = {cfg.motors.fl_a, cfg.motors.fl_b};
    motorPins[MOTOR_IDX_FR] = {cfg.motors.fr_a, cfg.motors.fr_b};
    motorPins[MOTOR_IDX_BL] = {cfg.motors.bl_a, cfg.motors.bl_b};
    motorPins[MOTOR_IDX_BR] = {cfg.motors.br_a, cfg.motors.br_b};
    
    bool ok = true;
    ok &= drive.reinitPins(motorPins, 10000, 8);
    ok &= servoReinitPins(cfg.servos.pins);
    
    if (ok) {
        PinConfigMgr::getInstance().save(cfg);
        request->send(200, "text/plain", "ok");
    } else {
        request->send(500, "text/plain", "hardware reinit failed");
    }
}
```

Register in `wsMgrBegin()`:
```cpp
server.on("/api/pins", HTTP_GET, handlePinsGet);
server.on("/api/pins", HTTP_POST, handlePinsPost);
```

---

### Task 5: Add Pin Config Tab to Web UI
**File:** `lib/WSMgr/WSMgr.cpp` — in `UI_INDEX_HTML`:
- Add tab button: `<button onclick="switchTab('pins')">Pin Config</button>`
- Add tab content section with:
  - Motor pins: 8 number inputs (FL_A, FL_B, FR_A, FR_B, BL_A, BL_B, BR_A, BR_B)
  - Servo pins: 6 number inputs (Servo 0-5)
  - "Load Defaults" button (fills from `RaggedyPins.h` constants)
  - "Load Current" button (fetches `/api/pins`)
  - "Save & Apply" button (POSTs to `/api/pins`)
- Add JS functions: `loadPins()`, `savePins()`, `loadDefaults()`

---

### Task 6: Update main.cpp to Use PinConfig on Boot
**File:** `src/main.cpp` — modify `setup()`:
```cpp
#include "PinConfig.h"

void setup() {
    // ... existing code ...
    
    // Load saved pin config or use defaults
    PinConfig cfg = PinConfigMgr::getInstance().load();
    
    // Build motor pins array
    MotorPins motorPins[MOTOR_COUNT];
    motorPins[MOTOR_IDX_FL] = {cfg.motors.fl_a, cfg.motors.fl_b};
    motorPins[MOTOR_IDX_FR] = {cfg.motors.fr_a, cfg.motors.fr_b};
    motorPins[MOTOR_IDX_BL] = {cfg.motors.bl_a, cfg.motors.bl_b};
    motorPins[MOTOR_IDX_BR] = {cfg.motors.br_a, cfg.motors.br_b};
    
    drive.MotorInit(motorPins, MOTOR_PWM_FREQ, MOTOR_PWM_RES);
    
    // Servo pins
    uint8_t servoPins[SERVO_COUNT] = {
        cfg.servos.pins[0], cfg.servos.pins[1], cfg.servos.pins[2],
        cfg.servos.pins[3], cfg.servos.pins[4], cfg.servos.pins[5]
    };
    // Need to modify servoInit to accept pin array, or call reinit after
    servoInit();  // uses defaults first
    servoReinitPins(servoPins);  // then override
    
    // ... rest of setup ...
}
```

---

### Task 7: Add PlatformIO Dependency for ArduinoJson
**File:** `platformio.ini` — add to `lib_deps`:
```
    bblanchon/ArduinoJson@^7.0.0
```

---

## Tests / Validation

| Test | Command | Expected |
|------|---------|----------|
| Compile | `pio run -e esp32dev` | Success, no errors |
| PinConfig load/save | Unit test on native env | Round-trip preserves values |
| DriveMgr reinit | Unit test | Motors respond after reinit |
| ServoMgr reinit | Unit test | Servos move after reinit |
| Web API GET | `curl http://<ip>/api/pins` | Valid JSON with 14 pins |
| Web API POST | `curl -X POST -d 'data=...' http://<ip>/api/pins` | Returns "ok", pins changed |
| UI loads | Open browser to `http://<ip>/` | Pin Config tab visible, fields populated |
| Persistence | Reboot ESP32 | Custom pins retained |

---

## Risks, Tradeoffs, Open Questions

| Risk | Mitigation |
|------|------------|
| Pin conflict (duplicate pins) | Validate in `PinConfigMgr::validate()` — reject if any motor pin duplicates |
| Hardware glitch during reinit | Take mutex, `emergencyStop()` first, reinit atomically |
| Servo detach/attach timing | `servoReinitPins()` detaches all before re-attaching |
| NVS wear | Preferences handles wear leveling; writes only on explicit save |
| Core 0/1 race on driveMutex | Mutex protects both `drive()` and `reinitPins()` |

**Open questions:**
1. Should buzzer pin also be configurable? (Currently fixed at `BUZZER_PIN`)
2. Should pin changes require reboot? (Current plan: no, live reinit)
3. Validate pin numbers against ESP32 GPIO matrix (input-only pins 34-39 can't be output)?

---

## Dependencies Between Tasks
```
Task 1 (PinConfig) 
    → Task 2 (DriveMgr reinit)
    → Task 3 (ServoMgr reinit)
    → Task 4 (WSMgr API) ← needs Task 1,2,3
    → Task 5 (Web UI) ← needs Task 4
    → Task 6 (main.cpp boot) ← needs Task 1
    → Task 7 (platformio.ini) ← independent
```

Can parallelize: Task 2 & 3 after Task 1; Task 7 anytime.