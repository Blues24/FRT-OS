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
    PinConfigMgr(const PinConfigMgr&) = delete;
    PinConfigMgr& operator=(const PinConfigMgr&) = delete;
    
    Preferences prefs;
    static constexpr const char* NS = "pin-config";
    static constexpr const char* KEY_MOTORS = "motors";
    static constexpr const char* KEY_SERVOS = "servos";
    static constexpr const char* KEY_VALID = "valid";
};