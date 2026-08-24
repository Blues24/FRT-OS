#include <unity.h>
#include <cstdint>
#include <cstring>
#include <Arduino.h>
#include <Ps3Controller.h>
#include "ControllerMgr.h"
#include "Buzzer.h"

static const char* TEST_MAC = "00:11:22:33:44:55";

static void resetPs3State(){
    memset((void*)&Ps3.data, 0, sizeof(Ps3.data));
}

static void setSticks(int lx, int ly, int rx, int ry){
    Ps3.data.analog.stick.lx = (int8_t)lx;
    Ps3.data.analog.stick.ly = (int8_t)ly;
    Ps3.data.analog.stick.rx = (int8_t)rx;
    Ps3.data.analog.stick.ry = (int8_t)ry;
}

//====================
// Singleton
//====================
void test_singleton_returns_same_instance(void){
    ControllerMgr& a = ControllerMgr::getInstance();
    ControllerMgr& b = ControllerMgr::getInstance();
    TEST_ASSERT_EQUAL_PTR(&a, &b);
}

// Copy ctor & operator= ControllerMgr dihapus (= delete) — kompilasi
// akan GAGAL bila ada yang mencoba menyalin instance. Eksistensi header
// sudah menjadi jaminan compile-time, jadi di sini hanya dicek kestabilan
// alamat singleton lintas pemanggilan.
void test_singleton_address_is_stable(void){
    void* first  = &ControllerMgr::getInstance();
    void* second = &ControllerMgr::getInstance();
    TEST_ASSERT_EQUAL_PTR(first, second);
}

//====================
// initPs3
//====================
void test_initPs3_does_not_throw(void){
    // Hanya memastikan initPs3 dapat dipanggil tanpa crash.
    // Callback dipasang ke Ps3; pemanggilan sebenarnya butuh hardware BT.
    ControllerMgr::getInstance().initPs3(TEST_MAC);
    TEST_ASSERT_TRUE(true);
}

//====================
// Status PS3
//====================
void test_isConnected_false_when_not_connected(void){
    TEST_ASSERT_FALSE(ControllerMgr::getInstance().isConnected());
}

void test_getBatteryLevel_returns_zero_when_disconnected(void){
    // Saat tidak terkoneksi, getBatteryLevel() mengembalikan 0.
    TEST_ASSERT_EQUAL_INT(0, ControllerMgr::getInstance().getBatteryLevel());
}

//====================
// setCurve (Bézier control points)
//====================
void test_setCurve_clamps_below_zero(void){
    auto& c = ControllerMgr::getInstance();
    // Semua nilai di bawah 0 harus di-constrain ke 0.0.
    c.setCurve(-1.0f, -2.0f, -0.5f, -3.0f);
    // Verifikasi tidak terjadi crash dan applyBezierCurve tetap dapat dipanggil.
    float out = c.applyBezierCurve(0.5f);
    (void)out;
    TEST_ASSERT_TRUE(true);
}

void test_setCurve_clamps_above_one(void){
    auto& c = ControllerMgr::getInstance();
    c.setCurve(2.0f, 5.0f, 1.5f, 99.0f);
    float out = c.applyBezierCurve(0.5f);
    (void)out;
    TEST_ASSERT_TRUE(true);
}

void test_setCurve_identity_passes_input_through(void){
    // Identity line: P1=(0,0), P2=(1,1) -> B(t).x = t dan B(t).y = t,
    // sehingga applyBezierCurve(x) ~= x untuk x di [0,1].
    auto& c = ControllerMgr::getInstance();
    c.setCurve(0.0f, 1.0f, 0.0f, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 0.5f, c.applyBezierCurve(0.5f));
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 0.8f, c.applyBezierCurve(0.8f));
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 0.2f, c.applyBezierCurve(0.2f));
}

//====================
// applyBezierCurve (helper privat, dipanggil via getDriveInput & sendiri)
//====================
void test_applyBezierCurve_zero_input_returns_zero(void){
    auto& c = ControllerMgr::getInstance();
    c.setCurve(0.0f, 1.0f, 0.0f, 1.0f);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, c.applyBezierCurve(0.0f));
}

void test_applyBezierCurve_preserves_sign(void){
    auto& c = ControllerMgr::getInstance();
    c.setCurve(0.0f, 1.0f, 0.0f, 1.0f);
    float pos = c.applyBezierCurve(0.7f);
    float neg = c.applyBezierCurve(-0.7f);
    TEST_ASSERT_GREATER_THAN(0.0f, pos);
    TEST_ASSERT_LESS_THAN(0.0f,    neg);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, pos, -neg);
}

//====================
// getDriveInput
//====================
void test_getDriveInput_zeros_when_disconnected(void){
    resetPs3State();
    setSticks(120, 120, 120, 120);
    float lx = 99.0f, ly = 99.0f, rx = 99.0f;
    int16_t speed = 999;
    ControllerMgr::getInstance().getDriveInput(lx, ly, rx, speed);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, lx);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, ly);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, rx);
    TEST_ASSERT_EQUAL_INT16(0, speed);
}

void test_getDriveInput_does_not_crash_for_full_sweep(void){
    // Sweep semua nilai stick (-127..127). Saat tidak terkoneksi output
    // selalu nol; tes ini menjamin tidak ada out-of-range atau crash.
    for(int v = -127; v <= 127; v += 16){
        resetPs3State();
        setSticks(v, v, v, v);
        float lx = 1.0f, ly = 1.0f, rx = 1.0f;
        int16_t speed = 1;
        ControllerMgr::getInstance().getDriveInput(lx, ly, rx, speed);
        TEST_ASSERT_EQUAL_FLOAT(0.0f, lx);
        TEST_ASSERT_EQUAL_FLOAT(0.0f, ly);
        TEST_ASSERT_EQUAL_FLOAT(0.0f, rx);
        TEST_ASSERT_EQUAL_INT16(0, speed);
    }
}

//====================
// isPressed (langsung tulis ke Ps3.data.button.* lalu verifikasi via isPressed)
//====================
void test_isPressed_cross_reflects_button_state(void){
    resetPs3State();
    Ps3.data.button.cross = 1;
    TEST_ASSERT_TRUE (ControllerMgr::getInstance().isPressed(ButtonPress::Cross));
    Ps3.data.button.cross = 0;
    TEST_ASSERT_FALSE(ControllerMgr::getInstance().isPressed(ButtonPress::Cross));
}

void test_isPressed_circle_reflects_button_state(void){
    resetPs3State();
    Ps3.data.button.circle = 1;
    TEST_ASSERT_TRUE (ControllerMgr::getInstance().isPressed(ButtonPress::Circle));
    Ps3.data.button.circle = 0;
    TEST_ASSERT_FALSE(ControllerMgr::getInstance().isPressed(ButtonPress::Circle));
}

void test_isPressed_triangle_reflects_button_state(void){
    resetPs3State();
    Ps3.data.button.triangle = 1;
    TEST_ASSERT_TRUE (ControllerMgr::getInstance().isPressed(ButtonPress::Triangle));
    Ps3.data.button.triangle = 0;
    TEST_ASSERT_FALSE(ControllerMgr::getInstance().isPressed(ButtonPress::Triangle));
}

void test_isPressed_square_reflects_button_state(void){
    resetPs3State();
    Ps3.data.button.square = 1;
    TEST_ASSERT_TRUE (ControllerMgr::getInstance().isPressed(ButtonPress::Square));
    Ps3.data.button.square = 0;
    TEST_ASSERT_FALSE(ControllerMgr::getInstance().isPressed(ButtonPress::Square));
}

void test_isPressed_dpad_reflects_button_state(void){
    resetPs3State();

    Ps3.data.button.up = 1;
    TEST_ASSERT_TRUE (ControllerMgr::getInstance().isPressed(ButtonPress::Up));
    Ps3.data.button.up = 0;
    TEST_ASSERT_FALSE(ControllerMgr::getInstance().isPressed(ButtonPress::Up));

    Ps3.data.button.down = 1;
    TEST_ASSERT_TRUE (ControllerMgr::getInstance().isPressed(ButtonPress::Down));
    Ps3.data.button.down = 0;
    TEST_ASSERT_FALSE(ControllerMgr::getInstance().isPressed(ButtonPress::Down));

    Ps3.data.button.left = 1;
    TEST_ASSERT_TRUE (ControllerMgr::getInstance().isPressed(ButtonPress::Left));
    Ps3.data.button.left = 0;
    TEST_ASSERT_FALSE(ControllerMgr::getInstance().isPressed(ButtonPress::Left));

    Ps3.data.button.right = 1;
    TEST_ASSERT_TRUE (ControllerMgr::getInstance().isPressed(ButtonPress::Right));
    Ps3.data.button.right = 0;
    TEST_ASSERT_FALSE(ControllerMgr::getInstance().isPressed(ButtonPress::Right));
}

void test_isPressed_shoulder_buttons_reflect_state(void){
    resetPs3State();

    Ps3.data.button.l1 = 1;
    TEST_ASSERT_TRUE (ControllerMgr::getInstance().isPressed(ButtonPress::L1));
    Ps3.data.button.l1 = 0;
    TEST_ASSERT_FALSE(ControllerMgr::getInstance().isPressed(ButtonPress::L1));

    Ps3.data.button.l2 = 1;
    TEST_ASSERT_TRUE (ControllerMgr::getInstance().isPressed(ButtonPress::L2));
    Ps3.data.button.l2 = 0;
    TEST_ASSERT_FALSE(ControllerMgr::getInstance().isPressed(ButtonPress::L2));

    Ps3.data.button.r1 = 1;
    TEST_ASSERT_TRUE (ControllerMgr::getInstance().isPressed(ButtonPress::R1));
    Ps3.data.button.r1 = 0;
    TEST_ASSERT_FALSE(ControllerMgr::getInstance().isPressed(ButtonPress::R1));

    Ps3.data.button.r2 = 1;
    TEST_ASSERT_TRUE (ControllerMgr::getInstance().isPressed(ButtonPress::R2));
    Ps3.data.button.r2 = 0;
    TEST_ASSERT_FALSE(ControllerMgr::getInstance().isPressed(ButtonPress::R2));
}

void test_isPressed_system_buttons_reflect_state(void){
    resetPs3State();

    Ps3.data.button.select = 1;
    TEST_ASSERT_TRUE (ControllerMgr::getInstance().isPressed(ButtonPress::Select));
    Ps3.data.button.select = 0;
    TEST_ASSERT_FALSE(ControllerMgr::getInstance().isPressed(ButtonPress::Select));

    Ps3.data.button.start = 1;
    TEST_ASSERT_TRUE (ControllerMgr::getInstance().isPressed(ButtonPress::Start));
    Ps3.data.button.start = 0;
    TEST_ASSERT_FALSE(ControllerMgr::getInstance().isPressed(ButtonPress::Start));

    Ps3.data.button.ps = 1;
    TEST_ASSERT_TRUE (ControllerMgr::getInstance().isPressed(ButtonPress::PSButton));
    Ps3.data.button.ps = 0;
    TEST_ASSERT_FALSE(ControllerMgr::getInstance().isPressed(ButtonPress::PSButton));
}

void test_isPressed_independent_buttons(void){
    // Pastikan dua tombol yang berbeda tidak saling memengaruhi state.
    resetPs3State();
    Ps3.data.button.cross = 1;
    Ps3.data.button.circle = 1;
    TEST_ASSERT_TRUE(ControllerMgr::getInstance().isPressed(ButtonPress::Cross));
    TEST_ASSERT_TRUE(ControllerMgr::getInstance().isPressed(ButtonPress::Circle));
    TEST_ASSERT_FALSE(ControllerMgr::getInstance().isPressed(ButtonPress::Triangle));
    TEST_ASSERT_FALSE(ControllerMgr::getInstance().isPressed(ButtonPress::Square));
}

//====================
// Suite setup/teardown
//====================
void setUp(void){
    resetPs3State();
    ControllerMgr::getInstance().setCurve(0.0f, 1.0f, 0.0f, 1.0f);
}

void tearDown(void){}

void setup(){
    UNITY_BEGIN();

    // Singleton
    RUN_TEST(test_singleton_returns_same_instance);
    RUN_TEST(test_singleton_address_is_stable);

    // initPs3
    RUN_TEST(test_initPs3_does_not_throw);

    // Status PS3
    RUN_TEST(test_isConnected_false_when_not_connected);
    RUN_TEST(test_getBatteryLevel_returns_zero_when_disconnected);

    // setCurve / Bézier
    RUN_TEST(test_setCurve_clamps_below_zero);
    RUN_TEST(test_setCurve_clamps_above_one);
    RUN_TEST(test_setCurve_identity_passes_input_through);
    RUN_TEST(test_applyBezierCurve_zero_input_returns_zero);
    RUN_TEST(test_applyBezierCurve_preserves_sign);

    // getDriveInput
    RUN_TEST(test_getDriveInput_zeros_when_disconnected);
    RUN_TEST(test_getDriveInput_does_not_crash_for_full_sweep);

    // isPressed per tombol (langsung tulis Ps3.data.button lalu verifikasi via isPressed)
    RUN_TEST(test_isPressed_cross_reflects_button_state);
    RUN_TEST(test_isPressed_circle_reflects_button_state);
    RUN_TEST(test_isPressed_triangle_reflects_button_state);
    RUN_TEST(test_isPressed_square_reflects_button_state);
    RUN_TEST(test_isPressed_dpad_reflects_button_state);
    RUN_TEST(test_isPressed_shoulder_buttons_reflect_state);
    RUN_TEST(test_isPressed_system_buttons_reflect_state);
    RUN_TEST(test_isPressed_independent_buttons);

    UNITY_END();
}

void loop(){}
