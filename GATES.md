# Gates: Dual-Core Implementation Verification

OWNS: src/**, lib/**, include/**, platformio.ini

Scope: Verify the FRT-OS dual-core implementation is complete, builds, and has all required synchronization primitives

- [ ] G1: All core source files exist and are non-empty
  CHECK: find src lib include -type f \( -name "*.cpp" -o -name "*.h" \) -size +0 | wc -l
  EXPECT: 14

- [ ] G2: Main.cpp contains dual-core task spawn with correct pinning
  CHECK: grep -c "xTaskCreatePinnedToCore" src/main.cpp
  EXPECT: 2

- [ ] G3: Core 0 task (net-loop) pinned to core 0, Core 1 task (hw-loop) pinned to core 1
  CHECK: grep -A2 "xTaskCreatePinnedToCore" src/main.cpp | grep -E "core.*[01]" | sed 's/.*//;s/,.*//' | sort
  EXPECT: 0
  1

- [ ] G4: Watchdog (TWDT) initialized with both tasks subscribed
  CHECK: grep -c "esp_task_wdt_add" src/main.cpp
  EXPECT: 2

- [ ] G5: Atomic connection flag in ControllerMgr for lock-free cross-core access
  CHECK: grep -c "std::atomic<bool> _connectedCache" lib/ControllerMgr/ControllerMgr.h
  EXPECT: 1

- [ ] G6: Buzzer has static mutex for thread-safe ring buffer access
  CHECK: grep -c "StaticSemaphore_t _mutexBuffer" lib/Buzzer/Buzzer.h
  EXPECT: 1

- [ ] G7: DriveMgr uses LEDC core v2 API (ledcAttach, ledcWrite)
  CHECK: grep -c "ledcAttach\|ledcWrite" lib/DriveMgr/DriveMgr.cpp
  EXPECT: 2

- [ ] G8: No TODO/FIXME/XXX markers in application code
  CHECK: grep -ri "TODO\|FIXME\|XXX" src lib include --include="*.cpp" --include="*.h" | grep -v ".pio" | wc -l
  EXPECT: 0

- [ ] G9: PlatformIO config has all required dependencies
  CHECK: grep -c "lib_deps" platformio.ini
  EXPECT: 1

- [ ] G10: WSMgr provides full web UI with 7 tabs
  CHECK: grep -c "tab-" lib/WSMgr/WSMgr.cpp
  EXPECT: 7

- [ ] G11: Hardware loop runs at 50Hz (20ms period) with vTaskDelayUntil
  CHECK: grep -c "vTaskDelayUntil" src/main.cpp
  EXPECT: 2

- [ ] G12: Both tasks feed watchdog periodically
  CHECK: grep -c "esp_task_wdt_reset" src/main.cpp
  EXPECT: 2

- [ ] G13: Motor pins mapping from RaggedyPins.h used in main.cpp
  CHECK: grep -c "MOTOR_PINS" src/main.cpp
  EXPECT: 4

- [ ] G14: PS3 controller initialized with MAC address
  CHECK: grep -c "initPs3" src/main.cpp
  EXPECT: 1