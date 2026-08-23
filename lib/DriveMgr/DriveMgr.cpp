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

void DriveMgr::MotorInit(const MotorPins pins[MOTOR_COUNT],
                         uint32_t freq, uint8_t res)
{
    for (uint8_t i = 0; i < MOTOR_COUNT; i++)
    {
        motorPins[i] = pins[i];
    }
    _pwmFreq = freq;
    _pwmRes = res;

    trim_fb = 0.0f;
    trim_lr = 0.0f;
    strafeGainMultiplier = 1.0f;
    rotateGainMultiplier = 1.0f;

    for (uint8_t i = 0; i < MOTOR_COUNT; i++)
    {
        ledcAttach(motorPins[i].pinPWM_A, freq, res);
        ledcAttach(motorPins[i].pinPWM_B, freq, res);
        ledcWrite(motorPins[i].pinPWM_A, 0);
        ledcWrite(motorPins[i].pinPWM_B, 0);
    }

    for (uint8_t i = 0; i < MOTOR_COUNT; i++)
    {
        lastMotorSpeeds[i] = 0.0f;
    }
}

void DriveMgr::SetMotorSpeed(uint8_t motorIdx, int16_t speed)
{
    if (motorIdx >= MOTOR_COUNT)
        return;

    // Internal clamp — caller must not be trusted.
    if (speed > MAX_MOTOR_SPEED)
    {
        speed = MAX_MOTOR_SPEED;
    }
    if (speed < -MAX_MOTOR_SPEED)
    {
        speed = -MAX_MOTOR_SPEED;
    }

    // Safe magnitude: avoid abs(INT16_MIN) UB by branching.
    uint16_t mag = (speed < 0)
                       ? static_cast<uint16_t>(-(speed + 1) + 1) // two's-complement safe
                       : static_cast<uint16_t>(speed);

    const MotorPins &p = motorPins[motorIdx];
    if (speed > 0)
    {
        ledcWrite(p.pinPWM_A, mag);
        ledcWrite(p.pinPWM_B, 0);
    }
    else if (speed < 0)
    {
        ledcWrite(p.pinPWM_A, 0);
        ledcWrite(p.pinPWM_B, mag);
    }
    else
    {
        ledcWrite(p.pinPWM_A, 0);
        ledcWrite(p.pinPWM_B, 0);
    }
}

void DriveMgr::emergencyStop()
{
    // Hard cut: bypass the ramp entirely. Used for estop button and
    // link-loss watchdog. Clears any pending coast request so the next
    // drive() call behaves normally (does not get stuck forcing 0).
    _coastRequested = false;
    for (uint8_t i = 0; i < MOTOR_COUNT; i++)
    {
        const MotorPins &p = motorPins[i];
        ledcWrite(p.pinPWM_A, 0);
        ledcWrite(p.pinPWM_B, 0);
        lastMotorSpeeds[i] = 0.0f;
    }
}

void DriveMgr::requestCoast()
{
    // Request a smooth ramp-down. The actual deceleration happens on the
    // next drive() call: target is forced to 0, lastMotorSpeeds[] moves
    // toward 0 at ACCEL_LIMIT rate per second.
    // If the caller never invokes drive() again, motors will keep their
    // last ramped value — by design. Use emergencyStop() for hard cut.
    _coastRequested = true;
}

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

void DriveMgr::drive(float strafeX, float forwardY, float rotationX,
                     int16_t baseSpeed)
{
    // NOTE: strafeX, forwardY, rotationX are expected in [-1, 1] and
    // already pre-deadzoned by ControllerMgr. DriveMgr does NOT clamp
    // or re-deadzone these — see header contract.

    strafeX *= strafeGainMultiplier;
    rotationX *= rotateGainMultiplier;

    // Mecanum X-config kinematics — single source of truth for per-wheel
    // assignment. Do NOT add "if strafe / else forward" branches: the
    // formula already degenerates to FL==FR / BL==BR when strafeX==0.
    float frontLeftSpeed = forwardY + strafeX + rotationX;
    float frontRightSpeed = forwardY - strafeX - rotationX;
    float backLeftSpeed = forwardY - strafeX + rotationX;
    float backRightSpeed = forwardY + strafeX - rotationX;

    float maxSpeed = absMax4(frontLeftSpeed, frontRightSpeed,
                             backLeftSpeed, backRightSpeed);
    if (maxSpeed > 1.0f)
    {
        float inv = 1.0f / maxSpeed;
        frontLeftSpeed *= inv;
        frontRightSpeed *= inv;
        backLeftSpeed *= inv;
        backRightSpeed *= inv;
    }

    float targetSpeed[MOTOR_COUNT] = {
        frontLeftSpeed * baseSpeed,
        frontRightSpeed * baseSpeed,
        backLeftSpeed * baseSpeed,
        backRightSpeed * baseSpeed,
    };

    // Per-wheel trim (independent — no pair shortcut).
    float frontTrim = (trim_fb < 0.0f) ? -trim_fb : 0.0f;
    float backTrim = (trim_fb > 0.0f) ? trim_fb : 0.0f;
    float leftTrim = (trim_lr < 0.0f) ? -trim_lr : 0.0f;
    float rightTrim = (trim_lr > 0.0f) ? trim_lr : 0.0f;

    float scales[MOTOR_COUNT] = {
        1.0f + frontTrim + leftTrim,
        1.0f + frontTrim + rightTrim,
        1.0f + backTrim + leftTrim,
        1.0f + backTrim + rightTrim,
    };

    for (uint8_t i = 0; i < MOTOR_COUNT; i++)
    {
        targetSpeed[i] *= scales[i];
    }

    // --- Coast handling ---
    // If coast() was called, force target 0 so the ramp can bring speeds
    // down gradually. Cleared here — any subsequent drive() with non-zero
    // input will resume normal kinematics automatically.
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

    // --- Delta-time acceleration ramp ---
    // ACCEL_LIMIT is in PWM units per SECOND, so we need actual elapsed
    // time between drive() calls — not call count, since loop rate is
    // not constant (PS3 callbacks, AsyncWebServer tasks share the CPU).
    //
    // Nominal loop rate assumption: 100 Hz (10 ms per call). Used as the
    // safe fallback for the first call after boot, after a long stall,
    // or after a watchdog reset where dt would otherwise be 0 or huge.
    constexpr float NOMINAL_DT_S = 1.0f / 100.0f;
    constexpr float MAX_DT_S     = 0.5f;   // 500 ms — beyond this treat as stall

    uint32_t nowMs = millis();
    float dt;
    if (_lastDriveMs == 0)
    {
        dt = NOMINAL_DT_S;
    }
    else
    {
        uint32_t elapsedMs = nowMs - _lastDriveMs;
        dt = static_cast<float>(elapsedMs) * 0.001f;
        if (dt <= 0.0f || dt > MAX_DT_S)
        {
            dt = NOMINAL_DT_S;
        }
    }
    _lastDriveMs = nowMs;

    const float maxStep = ACCEL_LIMIT * dt;

    for (uint8_t i = 0; i < MOTOR_COUNT; i++)
    {
        float cur  = lastMotorSpeeds[i];
        float want = targetSpeed[i];
        float step = want - cur;

        if (step >  maxStep) step =  maxStep;
        if (step < -maxStep) step = -maxStep;

        lastMotorSpeeds[i] = cur + step;

        // Cast float -> int16_t is safe: SetMotorSpeed clamps to
        // ±MAX_MOTOR_SPEED, and any overflow at the cast boundary is
        // bounded by trim/gain clamps in their setters (±0.5 trim,
        // 0.5..1.5 gain, baseSpeed ≤ MAX_MOTOR_SPEED).
        SetMotorSpeed(i, static_cast<int16_t>(lastMotorSpeeds[i]));
    }
}