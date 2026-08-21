#include <unity.h>
#include <cstdint>
//#include <Arduino.h>

// Fungsi yang ingin diuji (atau include header class kamu)
void test_pins_array_initialization(void)
{
    uint8_t ain1 = 12, ain2 = 13, bin1 = 14, bin2 = 15;
    uint8_t PinsList[4] = {ain1, ain2, bin1, bin2};

    // Pengecekan nilai menggunakan makro Unity
    TEST_ASSERT_EQUAL_UINT8(12, PinsList[0]);
    TEST_ASSERT_EQUAL_UINT8(13, PinsList[1]);
    TEST_ASSERT_EQUAL_UINT8(14, PinsList[2]);
    TEST_ASSERT_EQUAL_UINT8(15, PinsList[3]);
}

void setup()
{
    // Beri jeda sebentar untuk Serial Monitor
    //delay(2000);

    UNITY_BEGIN(); // Mulai pengujian

    // Jalankan fungsi tes
    RUN_TEST(test_pins_array_initialization);

    UNITY_END(); // Selesai pengujian
}

void loop()
{
    // Kosongkan loop untuk unit test
}