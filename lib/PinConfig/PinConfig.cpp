#include "PinConfig.h"

PinConfigMgr& PinConfigMgr::getInstance() {
    static PinConfigMgr instance;
    return instance;
}

PinConfig PinConfigMgr::getDefaults() {
    PinConfig cfg;
    
    // Motor pins from RaggedyPins.h
    cfg.motors.fl_a = FRONT_LEFT_MOTOR_PIN[0];
    cfg.motors.fl_b = FRONT_LEFT_MOTOR_PIN[1];
    cfg.motors.fr_a = FRONT_RIGHT_MOTOR_PIN[0];
    cfg.motors.fr_b = FRONT_RIGHT_MOTOR_PIN[1];
    cfg.motors.bl_a = BACK_LEFT_MOTOR_PIN[0];
    cfg.motors.bl_b = BACK_LEFT_MOTOR_PIN[1];
    cfg.motors.br_a = BACK_RIGHT_MOTOR_PIN[0];
    cfg.motors.br_b = BACK_RIGHT_MOTOR_PIN[1];
    
    // Servo pins from RaggedyPins.h
    cfg.servos.pins[0] = FRONT_LEFT_GRIPPER_PIN;    // GRIP1_BIG (0)
    cfg.servos.pins[1] = FRONT_MIDDLE_SERVO;        // GRIP1_SMALL1 (1)
    cfg.servos.pins[2] = FRONT_RIGHT_GRIPPER_PIN;   // GRIP1_SMALL2 (2)
    cfg.servos.pins[3] = BACK_MIDDLE_SERVO;         // GRIP2_BIG (3)
    cfg.servos.pins[4] = BACK_LEFT_GRIPER_PIN;      // GRIP2_SMALL1 (4)
    cfg.servos.pins[5] = BACK_RIGHT_GRIPPER_PIN;    // GRIP2_SMALL2 (5)
    
    cfg.valid = true;
    return cfg;
}

PinConfig PinConfigMgr::load() {
    prefs.begin(NS, true);  // read-only
    
    PinConfig cfg = getDefaults();
    
    if (prefs.getBool(KEY_VALID, false)) {
        // Load motor pins
        uint32_t motorData = prefs.getUInt(KEY_MOTORS, 0);
        if (motorData != 0) {
            cfg.motors.fl_a = (motorData >> 0) & 0xFF;
            cfg.motors.fl_b = (motorData >> 8) & 0xFF;
            cfg.motors.fr_a = (motorData >> 16) & 0xFF;
            cfg.motors.fr_b = (motorData >> 24) & 0xFF;
            
            uint32_t motorData2 = prefs.getUInt("motors2", 0);
            cfg.motors.bl_a = (motorData2 >> 0) & 0xFF;
            cfg.motors.bl_b = (motorData2 >> 8) & 0xFF;
            cfg.motors.br_a = (motorData2 >> 16) & 0xFF;
            cfg.motors.br_b = (motorData2 >> 24) & 0xFF;
        }
        
        // Load servo pins
        uint32_t servoData1 = prefs.getUInt("servos1", 0);
        uint32_t servoData2 = prefs.getUInt("servos2", 0);
        if (servoData1 != 0 || servoData2 != 0) {
            cfg.servos.pins[0] = (servoData1 >> 0) & 0xFF;
            cfg.servos.pins[1] = (servoData1 >> 8) & 0xFF;
            cfg.servos.pins[2] = (servoData1 >> 16) & 0xFF;
            cfg.servos.pins[3] = (servoData1 >> 24) & 0xFF;
            cfg.servos.pins[4] = (servoData2 >> 0) & 0xFF;
            cfg.servos.pins[5] = (servoData2 >> 8) & 0xFF;
        }
        
        cfg.valid = true;
    }
    
    prefs.end();
    return cfg;
}

bool PinConfigMgr::save(const PinConfig& config) {
    prefs.begin(NS, false);  // read-write
    
    // Pack motor pins into two 32-bit values
    uint32_t motorData1 = 0;
    motorData1 |= (uint32_t(config.motors.fl_a) & 0xFF) << 0;
    motorData1 |= (uint32_t(config.motors.fl_b) & 0xFF) << 8;
    motorData1 |= (uint32_t(config.motors.fr_a) & 0xFF) << 16;
    motorData1 |= (uint32_t(config.motors.fr_b) & 0xFF) << 24;
    
    uint32_t motorData2 = 0;
    motorData2 |= (uint32_t(config.motors.bl_a) & 0xFF) << 0;
    motorData2 |= (uint32_t(config.motors.bl_b) & 0xFF) << 8;
    motorData2 |= (uint32_t(config.motors.br_a) & 0xFF) << 16;
    motorData2 |= (uint32_t(config.motors.br_b) & 0xFF) << 24;
    
    // Pack servo pins into two 32-bit values
    uint32_t servoData1 = 0;
    servoData1 |= (uint32_t(config.servos.pins[0]) & 0xFF) << 0;
    servoData1 |= (uint32_t(config.servos.pins[1]) & 0xFF) << 8;
    servoData1 |= (uint32_t(config.servos.pins[2]) & 0xFF) << 16;
    servoData1 |= (uint32_t(config.servos.pins[3]) & 0xFF) << 24;
    
    uint32_t servoData2 = 0;
    servoData2 |= (uint32_t(config.servos.pins[4]) & 0xFF) << 0;
    servoData2 |= (uint32_t(config.servos.pins[5]) & 0xFF) << 8;
    
    bool ok = true;
    ok &= prefs.putBool(KEY_VALID, true);
    ok &= prefs.putUInt(KEY_MOTORS, motorData1);
    ok &= prefs.putUInt("motors2", motorData2);
    ok &= prefs.putUInt("servos1", servoData1);
    ok &= prefs.putUInt("servos2", servoData2);
    
    prefs.end();
    return ok;
}

bool PinConfigMgr::validate(const PinConfig& config, String& errorMsg) {
    // Check pin range (0-39 for ESP32)
    auto checkPin = [&](uint8_t pin, const char* name) -> bool {
        if (pin > 39) {
            errorMsg = String(name) + " pin " + pin + " out of range (0-39)";
            return false;
        }
        // Pins 34-39 are input-only on ESP32
        if (pin >= 34 && pin <= 39) {
            errorMsg = String(name) + " pin " + pin + " is input-only (34-39)";
            return false;
        }
        return true;
    };
    
    // Validate motor pins
    if (!checkPin(config.motors.fl_a, "FL_A")) return false;
    if (!checkPin(config.motors.fl_b, "FL_B")) return false;
    if (!checkPin(config.motors.fr_a, "FR_A")) return false;
    if (!checkPin(config.motors.fr_b, "FR_B")) return false;
    if (!checkPin(config.motors.bl_a, "BL_A")) return false;
    if (!checkPin(config.motors.bl_b, "BL_B")) return false;
    if (!checkPin(config.motors.br_a, "BR_A")) return false;
    if (!checkPin(config.motors.br_b, "BR_B")) return false;
    
    // Check for duplicate motor pins
    uint8_t motorPins[8] = {
        config.motors.fl_a, config.motors.fl_b,
        config.motors.fr_a, config.motors.fr_b,
        config.motors.bl_a, config.motors.bl_b,
        config.motors.br_a, config.motors.br_b
    };
    
    for (int i = 0; i < 8; i++) {
        for (int j = i + 1; j < 8; j++) {
            if (motorPins[i] == motorPins[j]) {
                errorMsg = "Duplicate motor pin: " + String(motorPins[i]);
                return false;
            }
        }
    }
    
    // Validate servo pins
    for (int i = 0; i < 6; i++) {
        char name[16];
        snprintf(name, sizeof(name), "Servo%d", i);
        if (!checkPin(config.servos.pins[i], name)) return false;
    }
    
    // Check for duplicate servo pins (pairwise comparison across all 6 servo pins)
    for (int i = 0; i < 6; i++) {
        for (int j = i + 1; j < 6; j++) {
            if (config.servos.pins[i] == config.servos.pins[j]) {
                errorMsg = "Duplicate servo pin: Servo" + String(i) + " and Servo" + String(j) + " both use pin " + String(config.servos.pins[i]);
                return false;
            }
        }
    }
    
    // Check servo pins don't conflict with motor pins
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 8; j++) {
            if (config.servos.pins[i] == motorPins[j]) {
                errorMsg = "Servo " + String(i) + " pin conflicts with motor pin " + String(config.servos.pins[i]);
                return false;
            }
        }
    }
    
    return true;
}