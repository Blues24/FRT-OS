#include <unity.h>
#include <cstdint>
#include <cstring>
#include <Arduino.h>

// Buka akses ke member private untuk verifikasi state internal
// (lastMotorSpeeds[], trim, gain, _coastRequested, dll) — pola umum
// untuk unit test embedded C++.
#define private public
#include "DriveMgr.h"
#undef private

//====================
// Fixture
//====================
static DriveMgr driveMgr;

static void fillDummyPins(MotorPins (&pins)[MOTOR_COUNT], uint8_t baseA = 10){
    for(uint8_t i = 0; i < MOTOR_COUNT; i++){
        pins[i].pinPWM_A = baseA + i * 2;
        pins[i].pinPWM_B = baseA + i * 2 + 1;
        pins[i].channel_A = -1;
        pins[i].channel_B = -1;
    }
}

static void initDriveMgr(DriveMgr& m, uint32_t freq = 10000, uint8_t res = 8){
    MotorPins pins[MOTOR_COUNT];
    fillDummyPins(pins);
    m.MotorInit(pins, freq, res);
}

//====================
// MotorInit
//====================
void test_MotorInit_stores_pins(void){
    MotorPins pins[MOTOR_COUNT];
    pins[MOTOR_IDX_FL] = {16, 2,  -1, -1};
    pins[MOTOR_IDX_FR] = {15, 4,  -1, -1};
    pins[MOTOR_IDX_BL] = {12, 3,  -1, -1};
    pins[MOTOR_IDX_BR] = {17, 6,  -1, -1};

    DriveMgr local;
    local.MotorInit(pins, 10000, 8);

    TEST_ASSERT_EQUAL_UINT8(16, local.motorPins[MOTOR_IDX_FL].pinPWM_A);
    TEST_ASSERT_EQUAL_UINT8(2,  local.motorPins[MOTOR_IDX_FL].pinPWM_B);
    TEST_ASSERT_EQUAL_UINT8(15, local.motorPins[MOTOR_IDX_FR].pinPWM_A);
    TEST_ASSERT_EQUAL_UINT8(4,  local.motorPins[MOTOR_IDX_FR].pinPWM_B);
    TEST_ASSERT_EQUAL_UINT8(12, local.motorPins[MOTOR_IDX_BL].pinPWM_A);
    TEST_ASSERT_EQUAL_UINT8(3,  local.motorPins[MOTOR_IDX_BL].pinPWM_B);
    TEST_ASSERT_EQUAL_UINT8(17, local.motorPins[MOTOR_IDX_BR].pinPWM_A);
    TEST_ASSERT_EQUAL_UINT8(6,  local.motorPins[MOTOR_IDX_BR].pinPWM_B);
}

void test_MotorInit_stores_pwm_config(void){
    DriveMgr local;
    initDriveMgr(local, 20000, 10);
    TEST_ASSERT_EQUAL_UINT32(20000, local._pwmFreq);
    TEST_ASSERT_EQUAL_UINT8(10,     local._pwmRes);
}

void test_MotorInit_resets_state(void){
    DriveMgr local;
    MotorPins dummy[MOTOR_COUNT];
    fillDummyPins(dummy, 50);
    local.MotorInit(dummy, 10000, 8);

    // Kotor-kotorin state internal
    local.lastMotorSpeeds[0]      = 123.0f;
    local.trim_fb                 = 0.4f;
    local.trim_lr                 = -0.3f;
    local.strafeGainMultiplier    = 1.2f;
    local.rotateGainMultiplier    = 0.8f;
    local._coastRequested         = true;

    // Init ulang dengan pin berbeda
    MotorPins pins[MOTOR_COUNT];
    fillDummyPins(pins, 70);
    local.MotorInit(pins, 5000, 8);

    TEST_ASSERT_EQUAL_FLOAT(0.0f, local.lastMotorSpeeds[0]);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, local.lastMotorSpeeds[1]);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, local.lastMotorSpeeds[2]);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, local.lastMotorSpeeds[3]);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, local.trim_fb);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, local.trim_lr);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, local.strafeGainMultiplier);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, local.rotateGainMultiplier);
    TEST_ASSERT_FALSE(local._coastRequested);
}

//====================
// setPowerTrim (clamp ±0.5)
//====================
void test_setPowerTrim_in_range(void){
    DriveMgr local;
    initDriveMgr(local);
    local.setPowerTrim(0.25f, -0.10f);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f,  0.25f, local.trim_fb);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, -0.10f, local.trim_lr);
}

void test_setPowerTrim_clamps_above_half(void){
    DriveMgr local;
    initDriveMgr(local);
    local.setPowerTrim(1.0f, 5.0f);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 0.5f, local.trim_fb);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 0.5f, local.trim_lr);
}

void test_setPowerTrim_clamps_below_minus_half(void){
    DriveMgr local;
    initDriveMgr(local);
    local.setPowerTrim(-2.0f, -99.0f);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, -0.5f, local.trim_fb);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, -0.5f, local.trim_lr);
}

void test_setPowerTrim_boundary_values(void){
    DriveMgr local;
    initDriveMgr(local);
    local.setPowerTrim(0.5f, -0.5f);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f,  0.5f, local.trim_fb);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, -0.5f, local.trim_lr);
}

//====================
// setGainMultipliers (clamp 0.5..1.5)
//====================
void test_setGainMultipliers_in_range(void){
    DriveMgr local;
    initDriveMgr(local);
    local.setGainMultipliers(0.8f, 1.3f);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 0.8f, local.strafeGainMultiplier);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 1.3f, local.rotateGainMultiplier);
}

void test_setGainMultipliers_clamps_below_half(void){
    DriveMgr local;
    initDriveMgr(local);
    local.setGainMultipliers(0.0f, -1.0f);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 0.5f, local.strafeGainMultiplier);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 0.5f, local.rotateGainMultiplier);
}

void test_setGainMultipliers_clamps_above_one_and_half(void){
    DriveMgr local;
    initDriveMgr(local);
    local.setGainMultipliers(2.0f, 10.0f);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 1.5f, local.strafeGainMultiplier);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 1.5f, local.rotateGainMultiplier);
}

void test_setGainMultipliers_boundary_values(void){
    DriveMgr local;
    initDriveMgr(local);
    local.setGainMultipliers(0.5f, 1.5f);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 0.5f, local.strafeGainMultiplier);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 1.5f, local.rotateGainMultiplier);
}

//====================
// SetMotorSpeed
//====================
void test_SetMotorSpeed_does_not_touch_lastMotorSpeeds(void){
    // SetMotorSpeed menulis ke LEDC langsung; lastMotorSpeeds[] hanya
    // dimutasi oleh drive(). Tes ini mengunci kontrak tersebut.
    DriveMgr local;
    initDriveMgr(local);
    local.lastMotorSpeeds[MOTOR_IDX_FL] = 42.0f;
    local.SetMotorSpeed(MOTOR_IDX_FL, 200);
    TEST_ASSERT_EQUAL_FLOAT(42.0f, local.lastMotorSpeeds[MOTOR_IDX_FL]);
}

void test_SetMotorSpeed_out_of_range_index_ignored(void){
    // idx >= MOTOR_COUNT harus diabaikan (no-op), tanpa crash dan tanpa
    // efek samping pada lastMotorSpeeds[].
    DriveMgr local;
    initDriveMgr(local);
    local.lastMotorSpeeds[0] = 42.0f;
    local.SetMotorSpeed(99, 100);
    TEST_ASSERT_EQUAL_FLOAT(42.0f, local.lastMotorSpeeds[0]);
}

void test_SetMotorSpeed_clamp_above_max_does_not_crash(void){
    DriveMgr local;
    initDriveMgr(local);
    local.SetMotorSpeed(MOTOR_IDX_FL, 1000);
    TEST_ASSERT_TRUE(true);
}

void test_SetMotorSpeed_clamp_below_min_does_not_crash(void){
    DriveMgr local;
    initDriveMgr(local);
    local.SetMotorSpeed(MOTOR_IDX_FL, -1000);
    TEST_ASSERT_TRUE(true);
}

void test_SetMotorSpeed_handles_int16_min_without_UB(void){
    // Regression: abs(INT16_MIN) adalah UB. Implementasi menggunakan
    // two's-complement-safe trick di baris 60-62 DriveMgr.cpp.
    DriveMgr local;
    initDriveMgr(local);
    local.SetMotorSpeed(MOTOR_IDX_FL, INT16_MIN);
    TEST_ASSERT_TRUE(true);
}

void test_SetMotorSpeed_all_indices_no_crash(void){
    DriveMgr local;
    initDriveMgr(local);
    for(uint8_t i = 0; i < MOTOR_COUNT; i++){
        local.SetMotorSpeed(i, 100);
        local.SetMotorSpeed(i, -100);
        local.SetMotorSpeed(i, 0);
    }
    TEST_ASSERT_TRUE(true);
}

//====================
// drive() — kinematics
//====================
void test_drive_straight_forward_full_throttle(void){
    // strafeX=0, forwardY=+1, rotationX=0 -> semua motor +baseSpeed.
    DriveMgr local;
    initDriveMgr(local);
    const int16_t BASE = 200;
    for(int i = 0; i < 200; i++){
        local.drive(0.0f, 1.0f, 0.0f, BASE);
        delay(20);
    }
    TEST_ASSERT_EQUAL_UINT16(BASE, (uint16_t)local.lastMotorSpeeds[MOTOR_IDX_FL]);
    TEST_ASSERT_EQUAL_UINT16(BASE, (uint16_t)local.lastMotorSpeeds[MOTOR_IDX_FR]);
    TEST_ASSERT_EQUAL_UINT16(BASE, (uint16_t)local.lastMotorSpeeds[MOTOR_IDX_BL]);
    TEST_ASSERT_EQUAL_UINT16(BASE, (uint16_t)local.lastMotorSpeeds[MOTOR_IDX_BR]);
}

void test_drive_straight_reverse_full_throttle(void){
    DriveMgr local;
    initDriveMgr(local);
    const int16_t BASE = 200;
    for(int i = 0; i < 200; i++){
        local.drive(0.0f, -1.0f, 0.0f, BASE);
        delay(20);
    }
    TEST_ASSERT_EQUAL_INT16(-BASE, (int16_t)local.lastMotorSpeeds[MOTOR_IDX_FL]);
    TEST_ASSERT_EQUAL_INT16(-BASE, (int16_t)local.lastMotorSpeeds[MOTOR_IDX_FR]);
    TEST_ASSERT_EQUAL_INT16(-BASE, (int16_t)local.lastMotorSpeeds[MOTOR_IDX_BL]);
    TEST_ASSERT_EQUAL_INT16(-BASE, (int16_t)local.lastMotorSpeeds[MOTOR_IDX_BR]);
}

void test_drive_pure_strafe_right(void){
    // strafeX=+1, forwardY=0, rotationX=0
    // FL = +strafe, FR = -strafe, BL = -strafe, BR = +strafe
    DriveMgr local;
    initDriveMgr(local);
    const int16_t BASE = 200;
    for(int i = 0; i < 200; i++){
        local.drive(1.0f, 0.0f, 0.0f, BASE);
        delay(20);
    }
    TEST_ASSERT_EQUAL_INT16( BASE, (int16_t)local.lastMotorSpeeds[MOTOR_IDX_FL]);
    TEST_ASSERT_EQUAL_INT16(-BASE, (int16_t)local.lastMotorSpeeds[MOTOR_IDX_FR]);
    TEST_ASSERT_EQUAL_INT16(-BASE, (int16_t)local.lastMotorSpeeds[MOTOR_IDX_BL]);
    TEST_ASSERT_EQUAL_INT16( BASE, (int16_t)local.lastMotorSpeeds[MOTOR_IDX_BR]);
}

void test_drive_pure_rotation_clockwise(void){
    // rotationX=+1 (X-config)
    // FL = +rot, FR = -rot, BL = +rot, BR = -rot
    DriveMgr local;
    initDriveMgr(local);
    const int16_t BASE = 200;
    for(int i = 0; i < 200; i++){
        local.drive(0.0f, 0.0f, 1.0f, BASE);
        delay(20);
    }
    TEST_ASSERT_EQUAL_INT16( BASE, (int16_t)local.lastMotorSpeeds[MOTOR_IDX_FL]);
    TEST_ASSERT_EQUAL_INT16(-BASE, (int16_t)local.lastMotorSpeeds[MOTOR_IDX_FR]);
    TEST_ASSERT_EQUAL_INT16( BASE, (int16_t)local.lastMotorSpeeds[MOTOR_IDX_BL]);
    TEST_ASSERT_EQUAL_INT16(-BASE, (int16_t)local.lastMotorSpeeds[MOTOR_IDX_BR]);
}

void test_drive_idle_zeros(void){
    DriveMgr local;
    initDriveMgr(local);
    local.lastMotorSpeeds[MOTOR_IDX_FL] = 100.0f;
    local.lastMotorSpeeds[MOTOR_IDX_FR] = -50.0f;
    for(int i = 0; i < 100; i++){
        local.drive(0.0f, 0.0f, 0.0f, 0);
        delay(20);
    }
    TEST_ASSERT_EQUAL_UINT16(0, (uint16_t)local.lastMotorSpeeds[MOTOR_IDX_FL]);
    TEST_ASSERT_EQUAL_UINT16(0, (uint16_t)local.lastMotorSpeeds[MOTOR_IDX_FR]);
    TEST_ASSERT_EQUAL_UINT16(0, (uint16_t)local.lastMotorSpeeds[MOTOR_IDX_BL]);
    TEST_ASSERT_EQUAL_UINT16(0, (uint16_t)local.lastMotorSpeeds[MOTOR_IDX_BR]);
}

//====================
// drive() — normalization (max abs > 1)
//====================
void test_drive_normalizes_combined_inputs(void){
    // forwardY=1 + strafeX=1 + rotationX=1 -> magnitude 3 dinormalisasi.
    // FL = +1, FR = -1, BL = +1, BR = +1 (setelah normalisasi semua = ±BASE).
    DriveMgr local;
    initDriveMgr(local);
    const int16_t BASE = 200;
    for(int i = 0; i < 250; i++){
        local.drive(1.0f, 1.0f, 1.0f, BASE);
        delay(20);
    }
    int16_t fl = (int16_t)local.lastMotorSpeeds[MOTOR_IDX_FL];
    int16_t fr = (int16_t)local.lastMotorSpeeds[MOTOR_IDX_FR];
    int16_t bl = (int16_t)local.lastMotorSpeeds[MOTOR_IDX_BL];
    int16_t br = (int16_t)local.lastMotorSpeeds[MOTOR_IDX_BR];
    TEST_ASSERT_GREATER_THAN(100, fl);
    TEST_ASSERT_LESS_THAN(-100, fr);
    TEST_ASSERT_GREATER_THAN(100, bl);
    TEST_ASSERT_GREATER_THAN(100, br);
}

//====================
// drive() — ramp
//====================
void test_drive_ramp_first_step_uses_nominal_dt(void){
    // Panggilan pertama: _lastDriveMs == 0 -> dt = NOMINAL_DT_S = 0.01s
    // maxStep = 80 * 0.01 = 0.8 PWM units per call (DriveMgr.cpp:257).
    DriveMgr local;
    initDriveMgr(local);
    local._lastDriveMs = 0;
    local.drive(0.0f, 1.0f, 0.0f, 255);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.8f, local.lastMotorSpeeds[MOTOR_IDX_FL]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.8f, local.lastMotorSpeeds[MOTOR_IDX_FR]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.8f, local.lastMotorSpeeds[MOTOR_IDX_BL]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.8f, local.lastMotorSpeeds[MOTOR_IDX_BR]);
}

void test_drive_ramp_progresses_over_calls(void){
    DriveMgr local;
    initDriveMgr(local);
    const int16_t BASE = 200;

    local.drive(0.0f, 1.0f, 0.0f, BASE);
    float first = local.lastMotorSpeeds[MOTOR_IDX_FL];

    delay(20);
    local.drive(0.0f, 1.0f, 0.0f, BASE);
    float second = local.lastMotorSpeeds[MOTOR_IDX_FL];

    delay(20);
    local.drive(0.0f, 1.0f, 0.0f, BASE);
    float third = local.lastMotorSpeeds[MOTOR_IDX_FL];

    TEST_ASSERT_GREATER_THAN(first,  second);
    TEST_ASSERT_GREATER_THAN(second, third);
}

void test_drive_ramp_decelerates_when_target_lower(void){
    DriveMgr local;
    initDriveMgr(local);
    for(int i = 0; i < 300; i++){
        local.drive(0.0f, 1.0f, 0.0f, 200);
        delay(20);
    }
    float saturated = local.lastMotorSpeeds[MOTOR_IDX_FL];
    TEST_ASSERT_GREATER_THAN(150.0f, saturated);

    local.drive(0.0f, 0.0f, 0.0f, 0);
    float afterOneStep = local.lastMotorSpeeds[MOTOR_IDX_FL];

    TEST_ASSERT_LESS_THAN(saturated, afterOneStep);
    TEST_ASSERT_GREATER_THAN(0.0f, afterOneStep);
}

//====================
// drive() — trim & gain efek pada target
//====================
void test_drive_trim_frontback_positive_boosts_front(void){
    DriveMgr local;
    initDriveMgr(local);
    local.setPowerTrim(0.5f, 0.0f); // trim_fb = +0.5 -> boost roda depan

    const int16_t BASE = 200;
    for(int i = 0; i < 200; i++){
        local.drive(0.0f, 1.0f, 0.0f, BASE);
        delay(20);
    }
    TEST_ASSERT_GREATER_THAN(local.lastMotorSpeeds[MOTOR_IDX_BL],
                             local.lastMotorSpeeds[MOTOR_IDX_FL]);
    TEST_ASSERT_GREATER_THAN(local.lastMotorSpeeds[MOTOR_IDX_BR],
                             local.lastMotorSpeeds[MOTOR_IDX_FR]);
}

void test_drive_trim_leftright_boosts_left(void){
    DriveMgr local;
    initDriveMgr(local);
    local.setPowerTrim(0.0f, -0.5f); // trim_lr = -0.5 -> boost roda kiri

    const int16_t BASE = 200;
    for(int i = 0; i < 200; i++){
        local.drive(0.0f, 1.0f, 0.0f, BASE);
        delay(20);
    }
    TEST_ASSERT_GREATER_THAN(local.lastMotorSpeeds[MOTOR_IDX_FR],
                             local.lastMotorSpeeds[MOTOR_IDX_FL]);
    TEST_ASSERT_GREATER_THAN(local.lastMotorSpeeds[MOTOR_IDX_BR],
                             local.lastMotorSpeeds[MOTOR_IDX_BL]);
}

void test_drive_strafe_gain_increases_lateral(void){
    DriveMgr local;
    initDriveMgr(local);
    local.setGainMultipliers(1.5f, 1.0f); // boost strafe
    for(int i = 0; i < 200; i++){
        local.drive(1.0f, 0.0f, 0.0f, 200);
        delay(20);
    }
    float boosted = fabsf(local.lastMotorSpeeds[MOTOR_IDX_FL]);

    DriveMgr local2;
    initDriveMgr(local2);
    local2.setGainMultipliers(0.5f, 1.0f); // kurangi strafe
    for(int i = 0; i < 200; i++){
        local2.drive(1.0f, 0.0f, 0.0f, 200);
        delay(20);
    }
    float lowered = fabsf(local2.lastMotorSpeeds[MOTOR_IDX_FL]);

    TEST_ASSERT_GREATER_THAN(lowered, boosted);
}

//====================
// emergencyStop
//====================
void test_emergencyStop_zeros_lastMotorSpeeds(void){
    DriveMgr local;
    initDriveMgr(local);
    for(int i = 0; i < 300; i++){
        local.drive(0.0f, 1.0f, 0.0f, 200);
        delay(20);
    }
    TEST_ASSERT_GREATER_THAN(50.0f, local.lastMotorSpeeds[MOTOR_IDX_FL]);

    local.emergencyStop();

    TEST_ASSERT_EQUAL_FLOAT(0.0f, local.lastMotorSpeeds[MOTOR_IDX_FL]);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, local.lastMotorSpeeds[MOTOR_IDX_FR]);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, local.lastMotorSpeeds[MOTOR_IDX_BL]);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, local.lastMotorSpeeds[MOTOR_IDX_BR]);
}

void test_emergencyStop_clears_coast_request(void){
    DriveMgr local;
    initDriveMgr(local);
    local.requestCoast();
    TEST_ASSERT_TRUE(local._coastRequested);

    local.emergencyStop();
    TEST_ASSERT_FALSE(local._coastRequested);
}

void test_emergencyStop_allows_resume_normal_drive(void){
    // Setelah estop, drive() berikutnya dengan input non-nol harus bisa
    // men-drive lagi (ramp bekerja dari 0).
    DriveMgr local;
    initDriveMgr(local);
    local.emergencyStop();
    TEST_ASSERT_EQUAL_FLOAT(0.0f, local.lastMotorSpeeds[MOTOR_IDX_FL]);

    local.drive(0.0f, 1.0f, 0.0f, 200);
    TEST_ASSERT_GREATER_THAN(0.0f, local.lastMotorSpeeds[MOTOR_IDX_FL]);
}

//====================
// requestCoast
//====================
void test_requestCoast_sets_flag(void){
    DriveMgr local;
    initDriveMgr(local);
    TEST_ASSERT_FALSE(local._coastRequested);
    local.requestCoast();
    TEST_ASSERT_TRUE(local._coastRequested);
}

void test_requestCoast_drives_target_to_zero(void){
    // Coast memaksa target ke 0 pada panggilan drive() berikutnya.
    DriveMgr local;
    initDriveMgr(local);
    for(int i = 0; i < 300; i++){
        local.drive(0.0f, 1.0f, 0.0f, 200);
        delay(20);
    }
    float saturated = local.lastMotorSpeeds[MOTOR_IDX_FL];
    TEST_ASSERT_GREATER_THAN(150.0f, saturated);

    local.requestCoast();
    local.drive(0.0f, 0.0f, 0.0f, 200);

    TEST_ASSERT_LESS_THAN(saturated, local.lastMotorSpeeds[MOTOR_IDX_FL]);
    TEST_ASSERT_GREATER_THAN(0.0f, local.lastMotorSpeeds[MOTOR_IDX_FL]);
}

void test_requestCoast_sticky_with_idle_input(void){
    // Jika operator diam setelah requestCoast(), _coastRequested harus
    // tetap true sampai drive() dipanggil dengan input non-nol.
    DriveMgr local;
    initDriveMgr(local);
    local.requestCoast();
    local.drive(0.0f, 0.0f, 0.0f, 0);
    TEST_ASSERT_TRUE(local._coastRequested);

    local.drive(0.0f, 0.0f, 0.0f, 0);
    TEST_ASSERT_TRUE(local._coastRequested);
}

void test_requestCoast_clears_on_nonzero_input(void){
    DriveMgr local;
    initDriveMgr(local);
    local.requestCoast();
    TEST_ASSERT_TRUE(local._coastRequested);

    local.drive(0.0f, 1.0f, 0.0f, 200);
    TEST_ASSERT_FALSE(local._coastRequested);
}

//====================
// Suite
//====================
void setUp(void){
    initDriveMgr(driveMgr);
}

void tearDown(void){}

void setup(){
    UNITY_BEGIN();

    // MotorInit
    RUN_TEST(test_MotorInit_stores_pins);
    RUN_TEST(test_MotorInit_stores_pwm_config);
    RUN_TEST(test_MotorInit_resets_state);

    // setPowerTrim
    RUN_TEST(test_setPowerTrim_in_range);
    RUN_TEST(test_setPowerTrim_clamps_above_half);
    RUN_TEST(test_setPowerTrim_clamps_below_minus_half);
    RUN_TEST(test_setPowerTrim_boundary_values);

    // setGainMultipliers
    RUN_TEST(test_setGainMultipliers_in_range);
    RUN_TEST(test_setGainMultipliers_clamps_below_half);
    RUN_TEST(test_setGainMultipliers_clamps_above_one_and_half);
    RUN_TEST(test_setGainMultipliers_boundary_values);

    // SetMotorSpeed
    RUN_TEST(test_SetMotorSpeed_does_not_touch_lastMotorSpeeds);
    RUN_TEST(test_SetMotorSpeed_out_of_range_index_ignored);
    RUN_TEST(test_SetMotorSpeed_clamp_above_max_does_not_crash);
    RUN_TEST(test_SetMotorSpeed_clamp_below_min_does_not_crash);
    RUN_TEST(test_SetMotorSpeed_handles_int16_min_without_UB);
    RUN_TEST(test_SetMotorSpeed_all_indices_no_crash);

    // drive kinematics
    RUN_TEST(test_drive_straight_forward_full_throttle);
    RUN_TEST(test_drive_straight_reverse_full_throttle);
    RUN_TEST(test_drive_pure_strafe_right);
    RUN_TEST(test_drive_pure_rotation_clockwise);
    RUN_TEST(test_drive_idle_zeros);

    // drive normalization
    RUN_TEST(test_drive_normalizes_combined_inputs);

    // drive ramp
    RUN_TEST(test_drive_ramp_first_step_uses_nominal_dt);
    RUN_TEST(test_drive_ramp_progresses_over_calls);
    RUN_TEST(test_drive_ramp_decelerates_when_target_lower);

    // drive trim & gain
    RUN_TEST(test_drive_trim_frontback_positive_boosts_front);
    RUN_TEST(test_drive_trim_leftright_boosts_left);
    RUN_TEST(test_drive_strafe_gain_increases_lateral);

    // emergencyStop
    RUN_TEST(test_emergencyStop_zeros_lastMotorSpeeds);
    RUN_TEST(test_emergencyStop_clears_coast_request);
    RUN_TEST(test_emergencyStop_allows_resume_normal_drive);

    // requestCoast
    RUN_TEST(test_requestCoast_sets_flag);
    RUN_TEST(test_requestCoast_drives_target_to_zero);
    RUN_TEST(test_requestCoast_sticky_with_idle_input);
    RUN_TEST(test_requestCoast_clears_on_nonzero_input);

    UNITY_END();
}

void loop(){}
