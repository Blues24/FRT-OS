#include "DriveMgr.h"
#include "RaggedyPins.h"
#include <Arduino.h>

// --- PWM / kinematic constants ---
// All PWM-related limits assume 8-bit LEDC resolution.
// Change together: BIT_RESOLUTION != 8 requires updating MAX_MOTOR_SPEED.
const uint32_t MOTOR_FREQ = 10000;
const uint8_t BIT_RESOLUTION = 8;
const int16_t MAX_MOTOR_SPEED = 255;
// Acceleration limit, in PWM units per SECOND.
// 80 = full sweep 0..255 takes ~3.2s at constant ramp.
const float ACCEL_LIMIT = 80.0f;

// ============================================================================
// PWM channel tracking — one channel per motor direction pin pair.
// With core v2 API: ledcSetup + ledcAttachPin per pin, ledcWrite per channel.
// ============================================================================
// Channel assignments (arbitrary but consistent; must match ledcSetup below).
static constexpr uint8_t CHANNEL_FL_A = 0;
static constexpr uint8_t CHANNEL_FL_B = 1;
static constexpr uint8_t CHANNEL_FR_A = 2;
static constexpr uint8_t CHANNEL_FR_B = 3;
static constexpr uint8_t CHANNEL_BL_A = 4;
static constexpr uint8_t CHANNEL_BL_B = 5;
static constexpr uint8_t CHANNEL_BR_A = 6;
static constexpr uint8_t CHANNEL_BR_B = 7;

// ----------------------------------------------------------------------------
// MotorInit — initialise LEDC channels and zero motor speeds
// Using core v2 API: ledcSetup(channel, frequency, resolution)
//                   ledcAttachPin(pin, channel)
//                   ledcWrite(channel, value)
// ----------------------------------------------------------------------------
void DriveMgr::MotorInit(const MotorPins pins[MOTOR_COUNT],
                         uint32_t freq, uint8_t res)
{
    // Configure all PWM channels once.
    ledcSetup(CHANNEL_FL_A, freq, res);
    ledcSetup(CHANNEL_FL_B, freq, res);
    ledcSetup(CHANNEL_FR_A, freq, res);
    ledcSetup(CHANNEL_FR_B, freq, res);
    ledcSetup(CHANNEL_BL_A, freq, res);
    ledcSetup(CHANNEL_BL_B, freq, res);
    ledcSetup(CHANNEL_BR_A, freq, res);
    ledcSetup(CHANNEL_BR_B, freq, res);

    // Attach each physical pin to its respective channel.
    ledcAttachPin(pins[MOTOR_IDX_FL].pinPWM_A, CHANNEL_FL_A);
    ledcAttachPin(pins[MOTOR_IDX_FL].pinPWM_B, CHANNEL_FL_B);
    ledcAttachPin(pins[MOTOR_IDX_FR].pinPWM_A, CHANNEL_FR_A);
    ledcAttachPin(pins[MOTOR_IDX_FR].pinPWM_B, CHANNEL_FR_B);
    ledcAttachPin(pins[MOTOR_IDX_BL].pinPWM_A, CHANNEL_BL_A);
    ledcAttachPin(pins[MOTOR_IDX_BL].pinPWM_B, CHANNEL_BL_B);
    ledcAttachPin(pins[MOTOR_IDX_BR].pinPWM_A, CHANNEL_BR_A);
    ledcAttachPin(pins[MOTOR_IDX_BR].pinPWM_B, CHANNEL_BR_B);

    // Reset trim/gain to defaults (setters will clamp on first use).
    trim_fb = 0.0f;
    trim_lr = 0.0f;
    strafeGainMultiplier = 1.0f;
    rotateGainMultiplier = 1.0f;

    // Initialise cached motor speeds to zero.
    for (uint8_t i = 0; i < MOTOR_COUNT; i++)
    {
        lastMotorSpeeds[i] = 0.0f;
    }
}

// ============================================================================
// SetMotorSpeed — write signed PWM to a single motor
// Using core v2 API: ledcWrite(channel, value)
// ============================================================================
void DriveMgr::SetMotorSpeed(uint8_t motorIdx, int16_t speed)
{
    // Guard against invalid motor index — silently no-op.
    if (motorIdx >= MOTOR_COUNT)
    {
        return;
    }

    // Internal clamp — caller must not be trusted with out-of-range values.
    // Clamp to [−MAX_MOTOR_SPEED, +MAX_MOTOR_SPEED].
    if (speed > MAX_MOTOR_SPEED)
    {
        speed = MAX_MOTOR_SPEED;
    }
    if (speed < -MAX_MOTOR_SPEED)
    {
        speed = -MAX_MOTOR_SPEED;
    }

    // Safe magnitude calculation: avoid undefined behavior from
    // std::abs(INT16_MIN) by using two's-complement-safe branching.
    uint16_t mag;
    if (speed < 0)
    {
        mag = static_cast<uint16_t>(-(speed + 1) + 1);
    }
    else
    {
        mag = static_cast<uint16_t>(speed);
    }

    // Map motor index to LEDC channel pair.
    // Each motor has two direction pins (A/B) sharing one PWM channel each.
    const uint8_t chA = (motorIdx == MOTOR_IDX_FL) ? CHANNEL_FL_A
                        : (motorIdx == MOTOR_IDX_FR) ? CHANNEL_FR_A
                        : (motorIdx == MOTOR_IDX_BL) ? CHANNEL_BL_A
                                                     : CHANNEL_BR_A;
    const uint8_t chB = (motorIdx == MOTOR_IDX_FL) ? CHANNEL_FL_B
                        : (motorIdx == MOTOR_IDX_FR) ? CHANNEL_FR_B
                        : (motorIdx == MOTOR_IDX_BL) ? CHANNEL_BL_B
                                                     : CHANNEL_BR_B;

    if (speed > 0)
    {
        // Forward direction: PWM on channel A, channel B = 0 (write 0 to pin B via its channel)
        ledcWrite(chA, mag);
        ledcWrite(chB, 0);
    }
    else if (speed < 0)
    {
        // Reverse direction: PWM on channel B, channel A = 0
        ledcWrite(chA, 0);
        ledcWrite(chB, mag);
    }
    else
    {
        // Brake/coast: both channels = 0
        ledcWrite(chA, 0);
        ledcWrite(chB, 0);
    }
}

// ============================================================================
// emergencyStop — hard cut to 0 on all motors, bypass ramp
// ============================================================================
void DriveMgr::emergencyStop()
{
    // Clear any pending coast request so the next drive() call behaves
    // normally (does not get stuck forcing 0).
    _coastRequested = false;

    // Write 0 to all PWM channels.
    ledcWrite(CHANNEL_FL_A, 0);
    ledcWrite(CHANNEL_FL_B, 0);
    ledcWrite(CHANNEL_FR_A, 0);
    ledcWrite(CHANNEL_FR_B, 0);
    ledcWrite(CHANNEL_BL_A, 0);
    ledcWrite(CHANNEL_BL_B, 0);
    ledcWrite(CHANNEL_BR_A, 0);
    ledcWrite(CHANNEL_BR_B, 0);

    // Reset cached speed so the next drive() starts from clean baseline.
    for (uint8_t i = 0; i < MOTOR_COUNT; i++)
    {
        lastMotorSpeeds[i] = 0.0f;
    }
}

// ============================================================================
// requestCoast — request smooth ramp-down (target 0 on next drive())
// ============================================================================
void DriveMgr::requestCoast()
{
    // Set flag; actual deceleration happens on the next drive() call:
    // target speeds are forced to 0, and lastMotorSpeeds[] moves toward 0
    // at ACCELIMIT_RATE per second using the delta-time ramp.
    _coastRequested = true;
}

// ============================================================================
// setPowerTrim — set front-back / left-right trim with clamping
// ============================================================================
void DriveMgr::setPowerTrim(float trim_frontback, float trim_leftright)
{
    // Clamp ±0.5f — anything larger distorts kinematics more than it
    // compensates for mechanical imbalance. Hard limit, not warning.
    if (trim_frontback > 0.5f)
        trim_frontback = 0.5f;
    if (trim_frontback < -0.5f)
        trim_frontback = -0.5f;
    if (trim_leftright > 0.5f)
        trim_leftright = 0.5f;
    if (trim_leftright < -0.5f)
        trim_leftright = -0.5f;
    trim_fb = trim_frontback;
    trim_lr = trim_leftright;
}

// ============================================================================
// setGainMultipliers — set strafe/rotate gain with clamping
// ============================================================================
void DriveMgr::setGainMultipliers(float strafeGain, float rotateGain)
{
    // Clamp 0.5..1.5:
    //   - Lower bound 0.5: prevents strafe/rotate axes from starving
    //     forward motion when combined (e.g. ly=1 + lx=0.5 must still
    //     leave headroom for the ly component on every wheel).
    //   - Upper bound 1.5: limits maximum torque bias per axis to a
    //     range that, combined with the 0.5 trim cap and PWM clamp at
    //     MAX_MOTOR_SPEED, never lets one motor exceed 1.875× another
    //     before PWM saturation — well within mecanum friction limits.
    if (strafeGain < 0.5f)
        strafeGain = 0.5f;
    if (strafeGain > 1.5f)
        strafeGain = 1.5f;
    if (rotateGain < 0.5f)
        rotateGain = 0.5f;
    if (rotateGain > 1.5f)
        rotateGain = 1.5f;
    strafeGainMultiplier = strafeGain;
    rotateGainMultiplier = rotateGain;
}

// ============================================================================
// absMax4 — helper: find float with largest absolute value
// ============================================================================
static inline float absMax4(float a, float b, float c, float d)
{
    float m = fabsf(a);
    if (fabsf(b) > m)
        m = fabsf(b);
    if (fabsf(c) > m)
        m = fabsf(c);
    if (fabsf(d) > m)
        m = fabsf(d);
    return m;
}

// ============================================================================
// drive — compute mecanum X-config kinematics, apply trim/gain, ramp speeds
// ============================================================================
void DriveMgr::drive(float strafeX, float forwardY, float rotationX,
                     int16_t baseSpeed)
{
    // NOTE: strafeX, forwardY, rotationX are expected in [-1, 1] and
    // already pre-deadzoned by ControllerMgr. DriveMgr does NOT clamp
    // or re-deadzone these — see header contract.

    // Apply gain multipliers (already clamped by setter).
    strafeX *= strafeGainMultiplier;
    rotationX *= rotateGainMultiplier;

    // ====================================================================
    // Mecanum X-config kinematics — single source of truth for per-wheel
    // assignment. The formula naturally degenerates to FL==FR / BL==BR
    // when strafeX==0 (straight drive). No "if strafe / else forward"
    // branches required.
    // ====================================================================
    float frontLeftSpeed     = forwardY + strafeX + rotationX;
    float frontRightSpeed    = forwardY - strafeX - rotationX;
    float backLeftSpeed      = forwardY - strafeX + rotationX;
    float backRightSpeed     = forwardY + strafeX - rotationX;

    // Normalisation: if the combined vector magnitude exceeds 1.0, scale
    // all four motors down proportionally so no motor exceeds MAX_MOTOR_SPEED.
    float maxSpeed = absMax4(frontLeftSpeed, frontRightSpeed,
                              backLeftSpeed, backRightSpeed);
    if (maxSpeed > 1.0f)
    {
        float inv = 1.0f / maxSpeed;
        frontLeftSpeed     *= inv;
        frontRightSpeed    *= inv;
        backLeftSpeed      *= inv;
        backRightSpeed     *= inv;
    }

    // Multiply by baseSpeed (global throttle cap).
    float targetSpeed[MOTOR_COUNT] = {
        frontLeftSpeed     * baseSpeed,
        frontRightSpeed    * baseSpeed,
        backLeftSpeed      * baseSpeed,
        backRightSpeed     * baseSpeed,
    };

    // ====================================================================
    // Per-wheel trim (independent — no pair shortcut).
    //   - frontTrim  : +trim_fb boosts front, −trim_fb cuts front
    //   - backTrim   : +trim_fb boosts rear,  −trim_fb cuts rear
    //   - leftTrim   : −trim_lr boosts left,  +trim_lr cuts left
    //   - rightTrim  : +trim_lr boosts right, −trim_lr cuts right
    // ====================================================================
    float frontTrim  = (trim_fb < 0.0f) ? -trim_fb : 0.0f;
    float backTrim   = (trim_fb > 0.0f) ? trim_fb : 0.0f;
    float leftTrim   = (trim_lr < 0.0f) ? -trim_lr : 0.0f;
    float rightTrim  = (trim_lr > 0.0f) ? trim_lr : 0.0f;

    float scales[MOTOR_COUNT] = {
        1.0f + frontTrim + leftTrim,       // FL
        1.0f + frontTrim + rightTrim,      // FR
        1.0f + backTrim + leftTrim,        // BL
        1.0f + backTrim + rightTrim,       // BR
    };

    for (uint8_t i = 0; i < MOTOR_COUNT; i++)
    {
        targetSpeed[i] *= scales[i];
    }

    // ====================================================================
    // Coast handling — if coast() was called, force target 0 so the ramp
    // can bring speeds down gradually. Cleared here — any subsequent
    // drive() with non-zero input will resume normal kinematics automatically.
    // ====================================================================
    if (_coastRequested)
    {
        for (uint8_t i = 0; i < MOTOR_COUNT; i++)
        {
            targetSpeed[i] = 0.0f;
        }
        // Only auto-clear if caller is actually sending non-zero command.
        // Keeps coast sticky if the operator is genuinely idle.
        if (strafeX != 0.0f || forwardY != 0.0f || rotationX != 0.0f)
        {
            _coastRequested = false;
        }
    }

    // ====================================================================
    // Delta-time acceleration ramp.
    // ACCEL_LIMIT is in PWM units per SECOND, so we need actual elapsed
    // time between drive() calls — not call count, since loop rate is
    // not constant (PS3 callbacks, AsyncWebServer tasks share the CPU).
    //
    // Nominal loop rate assumption: 100 Hz (10 ms per call). Used as the
    // safe fallback for the first call after boot, after a long stall,
    // or after a watchdog reset where dt would otherwise be 0 or huge.
    // ====================================================================
    constexpr float NOMINAL_DT_S = 1.0f / 100.0f;
    constexpr float MAX_DT_S     = 0.5f;   // 500 ms — beyond this treat as stall

    uint32_t nowMs = millis();
    float dt;
    if (_lastDriveMs == 0)
    {
        // First call ever (or after reset): use nominal interval.
        dt = NOMINAL_DT_S;
    }
    else
    {
        uint32_t elapsedMs = nowMs - _lastDriveMs;
        dt = static_cast<float>(elapsedMs) * 0.001f;
        if (dt <= 0.0f || dt > MAX_DT_S)
        {
            // Stalled or absurd dt: fall back to nominal.
            dt = NOMINAL_DT_S;
        }
    }
    _lastDriveMs = nowMs;

    const float maxStep = ACCEL_LIMIT * dt;

    // ====================================================================
    // Per-wheel ramp: bring lastMotorSpeeds[] toward targetSpeed[] at
    // most maxStep PWM units per call. Cast float→int16_t is safe because
    // SetMotorSpeed clamps to ±MAX_MOTOR_SPEED and the trim/gain limits
    // already prevent overflow at the cast boundary.
    // ====================================================================
    for (uint8_t i = 0; i < MOTOR_COUNT; i++)
    {
        float cur  = lastMotorSpeeds[i];
        float want = targetSpeed[i];
        float step = want - cur;

        // Clamp step size to acceleration limit.
        if (step >  maxStep)
            step =  maxStep;
        if (step < -maxStep)
            step = -maxStep;

        lastMotorSpeeds[i] = cur + step;

        // Cast float → int16_t is safe: SetMotorSpeed clamps to
        // ±MAX_MOTOR_SPEED, and any overflow at the cast boundary is
        // bounded by trim/gain clamps in their setters (±0.5 trim,
        // 0.5..1.5 gain, baseSpeed ≤ MAX_MOTOR_SPEED).
        SetMotorSpeed(i, static_cast<int16_t>(lastMotorSpeeds[i]));
    }
}