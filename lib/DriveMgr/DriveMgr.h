#ifndef DRIVEMGR_H
#define DRIVEMGR_H

#include <Arduino.h>

// --- Motor index convention (used by all DriveMgr APIs) ---
// Index 0 = Front-Left, 1 = Front-Right, 2 = Back-Left, 3 = Back-Right.
// Order MUST match the order filled by caller in MotorInit() and the order
// written to lastMotorSpeeds[] by drive(). Changing this is a breaking
// change for the whole kinematics pipeline.
static constexpr uint8_t MOTOR_IDX_FL = 0;
static constexpr uint8_t MOTOR_IDX_FR = 1;
static constexpr uint8_t MOTOR_IDX_BL = 2;
static constexpr uint8_t MOTOR_IDX_BR = 3;
static constexpr uint8_t MOTOR_COUNT = 4;

// Static assertion to ensure MOTOR_COUNT is consistent with array sizes
static_assert(MOTOR_COUNT == 4, "MOTOR_COUNT must be 4 for mecanum X-config");

/**
 * Per-motor pin mapping.
 * - pinPWM_A / pinPWM_B : the two direction-input pins of the BTS7960
 *   (or equivalent H-bridge). DriveMgr writes speed to one, 0 to the other.
 * - channel_A / channel_B : reserved for explicit LEDC channel allocation.
 *   Currently -1 (auto-assigned by ledcAttach). Will be used when the
 *   channel map is finalised.
 */
struct MotorPins
{
    uint8_t pinPWM_A;
    uint8_t pinPWM_B;
    int channel_A = -1;
    int channel_B = -1;
};

// Compile-time check: MotorPins must have all required fields
static_assert(offsetof(MotorPins, pinPWM_A) == 0, "MotorPins pinPWM_A must be first field");
static_assert(offsetof(MotorPins, pinPWM_B) == 1, "MotorPins pinPWM_B must be second field");

class DriveMgr
{
private:
    // Motor pin table — indexed by MOTOR_IDX_FL/FR/BL/BR.
    MotorPins motorPins[MOTOR_COUNT];

    // Cached PWM config (filled by MotorInit).
    uint32_t _pwmFreq = 0;
    uint8_t _pwmRes = 0;

    // Power trim & gain (set via setters, clamped inside setters).
    float trim_fb = 0.0f;
    float trim_lr = 0.0f;
    float strafeGainMultiplier = 1.0f;
    float rotateGainMultiplier = 1.0f;

    // Last applied per-motor speed (PWM units, signed). Used by Task 4
    // acceleration ramp and by coast()/emergencyStop().
    float lastMotorSpeeds[MOTOR_COUNT] = {0.0f, 0.0f, 0.0f, 0.0f};

    // Timestamp (millis) of previous drive() call — drives the delta-time
    // acceleration ramp. Initialised to 0; first drive() call falls back
    // to the nominal dt.
    uint32_t _lastDriveMs = 0;

    // When true, drive() requests target 0 on every motor and lets the
    // ramp bring lastMotorSpeeds[] down gradually. Cleared on the first
    // drive() call where the caller passes non-zero input. Set by coast().
    bool _coastRequested = false;

public:
    /**
     * Initialise LEDC channels for all 4 motors and zero their PWM.
     * Caller fills `pins[MOTOR_COUNT]` in order FL, FR, BL, BR using values
     * from the pin-map header. `freq` and `res` apply to all channels.
     * @note After this call, all motors are at 0 speed.
     */
    void MotorInit(const MotorPins pins[MOTOR_COUNT],
                   uint32_t freq, uint8_t res);

    /**
     * Set power trim values for front-back and left-right imbalance.
     * Values are clamped to ±0.5f — larger values would distort kinematics
     * more than they compensate for mechanical imbalance.
     * @param trim_frontback Positive values boost front motors, negative boost rear.
     * @param trim_leftright Positive values boost right motors, negative boost left.
     */
    void setPowerTrim(float trim_frontback, float trim_leftright);

    /**
     * Set gain multipliers for strafe and rotate axes.
     * Clamped to [0.5f, 1.5f] range to prevent axis starvation and excessive torque bias.
     * @param strafeGain Multiplier for strafe axis (sideways movement).
     * @param rotateGain Multiplier for rotate axis (rotation).
     */
    void setGainMultipliers(float strafeGain, float rotateGain);

    /**
     * Compute mecanum X-config kinematics and write per-motor PWM.
     *
     * Contract:
     *   - strafeX / forwardY / rotationX MUST already be in [-1, 1]
     *     and pre-deadzoned by ControllerMgr. DriveMgr does NOT re-clamp
     *     or re-deadzone these.
     *   - baseSpeed is the global throttle cap in PWM units (typically
     *     [0, MAX_MOTOR_SPEED]).
     *   - All four motors are written independently per kinematics; there
     *     is no "pair shortcut". The mecanum X-config formula naturally
     *     makes FL==FR and BL==BR when strafeX==0 (straight drive).
     *   - Input values outside [-1, 1] may produce unexpected results;
     *     caller is responsible for pre-processing.
     */
    void drive(float strafeX, float forwardY, float rotationX,
               int16_t baseSpeed);

    /**
     * Write `speed` (signed PWM units) to motor at `motorIdx`.
     * Clamps internally to ±MAX_MOTOR_SPEED. Safe against INT16_MIN edge case.
     * @param motorIdx Motor index [0..3], matching MOTOR_IDX_* constants.
     * @param speed Signed PWM value; will be clamped to ±MAX_MOTOR_SPEED.
     * @note Uses two's-complement-safe magnitude calculation to avoid
     *       undefined behavior from abs(INT16_MIN).
     */
    void SetMotorSpeed(uint8_t motorIdx, int16_t speed);

    /**
     * Hard cut to 0 on all channels. Resets lastMotorSpeeds[] to 0
     * so the next drive() call starts from a clean ramp baseline.
     * Bypasses the acceleration ramp — for estop / link-loss only.
     * Also clears any pending coast request.
     */
    void emergencyStop();

    /**
     * Smooth stop: requests target 0 and lets the existing acceleration
     * ramp (Task 4) bring lastMotorSpeeds[] down gradually.
     * Next drive() call with non-zero input will immediately override.
     * If caller never invokes drive() again, motors will keep their
     * last ramped value — by design.
     */
    void requestCoast();
};

#endif // DRIVEMGR_H