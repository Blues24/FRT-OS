#ifndef CONTROLLERMGR_H
#define CONTROLLERMGR_H

#include <Arduino.h>
#include <cmath>
#include <atomic>
#include <mutex>
#include <Ps3Controller.h>

/**
 * @struct BezierSample
 * @brief Menyimpan satu sampel hasil evaluasi kurva Bézier kubik.
 *
 * Field:
 *  - x        : nilai x pada parameter t (sumbu input, biasanya 0.0 - 1.0).
 *  - y        : nilai y pada parameter t (sumbu output / hasil kurva).
 *  - dx       : turunan pertama dx/dt pada parameter t, dipakai untuk
 *               metode Newton-Raphson saat melakukan inverse-mapping.
 */
struct BezierSample
{
    float x;
    float y;
    float dx; // derivative dx/dt
    
};

/**
 * @struct AnalogSnapshot
 * @brief Thread-safe snapshot of all four analog stick values.
 *
 * Captured atomically in the PS3 callback, read by getDriveInput()
 * to prevent torn reads across cores.
 */
struct AnalogSnapshot {
    int lx = 0;
    int ly = 0;
    int rx = 0;
    int ry = 0;
    uint32_t timestamp = 0;
};

/**
 * @enum ButtonPress
 * @brief Enumerasi tombol PS3 yang dapat dicek status tekannya lewat
 *        ControllerMgr::isPressed(). R2 sengaja tidak dipetakan karena
 *        R2 pada library ini dibaca sebagai analog (bukan digital).
 */
enum class ButtonPress{
    Cross,
    Circle,
    Triangle,
    Square,
    Up,
    Down,
    Left,
    Right,
    L1,
    L2,
    R1,
    R2,
    Select,
    Start,
    PSButton
};

class ControllerMgr {
    private:
        // Cache atomic status koneksi PS3 (thread-safe untuk multi-core).
        std::atomic<bool> _connectedCache{false};
        
        // Mutex for protecting analog stick snapshot
        std::mutex _analogMutex;
        AnalogSnapshot _analogSnapshot;

        // Constructor Kelas ControllerMgr
        ControllerMgr();
        // ---- Konstanta tuning drive ----
        // Kecepatan PWM motor saat idle/tanpa boost.
        static const int16_t BASE_MOTOR_SPD              = 100;
        // Batas atas kecepatan PWM motor (nilai maksimal ESP32 LEDC / Arduino analogWrite).
        static const int16_t MAX_MOTOR_SPD               = 255;
        // Ambang absolut stick kanan (0-127) di mana rotasi masih "dicampur" dengan boost;
        // di atas nilai ini boost ditiadakan dan prioritas diberikan ke rotasi.
        static const int16_t ROTATION_PRIORITY_THRESHOLD = 70;
        // Deadzone minimum untuk stick kiri (nilai ADC absolut di bawah ini dianggap 0).
        static const uint8_t MIN_DZ                      = 15;
        // Deadzone yang lebih besar untuk stick kanan, dipakai sebagai ambang awal boost
        // agar gerakan kecil tidak langsung memicu kecepatan penuh.
        static const uint8_t BOOST_DZ                    = 25;
        // Deadzone maksimum (dicadangkan untuk penggunaan lanjut, saat ini tidak dipakai).
        static const uint8_t MAX_DZ                      = 40;
        
        // ---- Callback library Ps3Controller.h ----
        // Dipasang ke Ps3.attach(): terpanggil setiap ada update data dari PS3.
        // Saat ini dijadikan placeholder untuk membunyikan buzzer sebagai umpan balik.
        static void notifyUser();
        // Dipasang ke Ps3.attachOnConnect(): dipanggil sekali saat controller berhasil pairing.
        // Hanya mencetak log "[INFO] PS3 Controller has been Connected!" ke Serial.
        static void onConnect();
        // Dipasang ke Ps3.attachOnDisconnect(): dipanggil saat koneksi terputus.
        // Hanya mencetak log "[INFO] PS3 Controller has been Disconnected!" ke Serial.
        static void onDisconnect();

        // Titik kontrol (P1.x, P2.x, P1.y, P2.y) untuk kurva Bézier kubik
        // yang memetakan input stick (sumbu x) menjadi output drive (sumbu y).
        float bezierControlX1, bezierControlX2, bezierControlY1, bezierControlY2; 
        
        /**
         * @brief Inverse-mapping kurva Bézier: mencari parameter t (0..1) sehingga
         *        Bézier(t).x ≈ target_axis. Menggunakan Newton-Raphson yang
         *        dibatasi dengan bracketing fallback jika langkah keluar rentang.
         *
         * @param target_axis nilai x yang ingin dicapai (hasil read stick ternormalisasi).
         * @return parameter t hasil iterasi (0.0 - 1.0).
         */
        float solveBezier(float target_axis);
        
        /**
         * @brief Mengevaluasi kurva Bézier kubik pada parameter t dan mengembalikan
         *        nilai (x, y, dx/dt). Rumus mengikuti bentuk standar Bézier kubik
         *        dengan P0=(0,0) dan P3=(1,1) sehingga tinggal titik kontrol
         *        P1=(bezierControlX1, bezierControlY1) dan P2=(bezierControlX2, bezierControlY2).
         *
         * @param t parameter kurva (0.0 - 1.0).
         * @return BezierSample berisi x, y, dan dx pada t tersebut.
         */
        inline BezierSample getBezierSample(float t);
    public:
        ControllerMgr(const ControllerMgr&) = delete;
        ControllerMgr operator=(const ControllerMgr&) = delete;
        /**
         * @brief Mengakses instance tunggal ControllerMgr (Meyers singleton,
         *        thread-safe inisialisasi di C++11+, hemat RAM karena dialokasikan
         *        di stack statis, bukan heap).
         *
         * @return referensi ke satu-satunya instance ControllerMgr.
         */
        static ControllerMgr& getInstance();
        
        /**
         * @brief Inisialisasi library Ps3Controller dengan MAC address host ESP32
         *        dan mendaftarkan ketiga callback (notifyUser, onConnect, onDisconnect).
         *
         * @param mac string MAC address Bluetooth ESP32 yang sudah di-set
         *             via Ps3.setMac() / dipairing dengan PS3. Disimpan oleh
         *             library dan dipakai saat Ps3.begin().
         */
        void initPs3(const char* mac);

        // ---- Helper status PS3 ----
        /**
         * @brief Mengecek apakah controller PS3 sedang terkoneksi.
         *
         * @return true jika Ps3.isConnected() true, selain itu false.
         */
        bool isConnected();
        
        /**
         * @brief Membaca level baterai PS3 dalam persen (0-100) sesuai field
         *        Ps3.data.status.battery. Mengembalikan 0 jika belum terkoneksi.
         *
         * @return level baterai (integer).
         */
        int getBatteryLevel();
        
        /**
         * @brief Mengecek apakah tombol tertentu sedang ditekan pada snapshot
         *        data PS3 terakhir. Pemetaan dilakukan via switch-case ke field
         *        Ps3.data.button.* yang bersesuaian.
         *
         * @param Btn enumerasi ButtonPress yang ingin dicek.
         * @return true bila tombol sedang ditekan, false bila tidak atau
         *         nilai Btn di luar enumerasi yang dikenali.
         */
        bool isPressed(ButtonPress Btn);

        /**
         * @brief Mengambil dan memproses input stick untuk kebutuhan driving.
         *        Alur kerja:
         *          1. Jika tidak terkoneksi → semua output di-nol-kan, keluar.
         *          2. Baca analog stick snapshot (thread-safe via mutex).
         *          3. Terapkan deadzone (MIN_DZ untuk stick kiri, BOOST_DZ untuk kanan).
         *          4. Normalisasi ke rentang -1.0 .. 1.0 (di luar deadzone boost).
         *          5. Aplikasikan kurva Bézier lewat applyBezierCurve() agar
         *             respons bisa dibentuk (lebih agresif di tengah/ujung).
         *          6. Hitung kecepatan motor: BASE_MOTOR_SPD default, naik ke
         *             MAX_MOTOR_SPD bila stick kiri ditarik ke belakang,
         *             atau di-boost proporsional dengan stick kanan-Y.
         *             Boost ditahan saat rotasi (kanan-Y) sudah melewati
         *             ROTATION_PRIORITY_THRESHOLD.
         *
         * @param lx    [out] stick kiri X ternormalisasi + dikurva (-1.0 .. 1.0).
         * @param ly    [out] stick kiri Y ternormalisasi + dikurva (-1.0 .. 1.0).
         * @param rx    [out] stick kanan X ternormalisasi + dikurva (-1.0 .. 1.0).
         * @param speed [out] kecepatan motor PWM hasil kalkulasi boost
         *                    (rentang BASE_MOTOR_SPD .. MAX_MOTOR_SPD).
         */
        void getDriveInput(float& lx, float& ly, float& rx, int16_t& speed);
        
        /**
         * @brief Mengatur titik kontrol kurva Bézier kubik. Setiap parameter
         *        dibatasi (constrain) ke rentang 0.0 .. 1.0 agar kurva tetap valid.
         *
         * @param x1 koordinat X titik kontrol P1 (default 0.0).
         * @param x2 koordinat X titik kontrol P2 (default 1.0).
         * @param y1 koordinat Y titik kontrol P1 (default 0.0).
         * @param y2 koordinat Y titik kontrol P2 (default 1.0).
         */
        void setCurve(float x1, float x2, float y1, float y2);

        // ---- Helper kurva Bézier ----
        /**
         * @brief Menerapkan kurva Bézier ke sebuah nilai input. Mendukung nilai
         *        negatif dengan memproses magnitude lalu mengembalikan tanda yang
         *        sama. Input 0 langsung menghasilkan 0.
         *
         * @param rawVal nilai input ternormalisasi (-1.0 .. 1.0).
         * @return hasil setelah dilewatkan kurva, dengan tanda yang sama dengan rawVal.
         */
        inline float applyBezierCurve(float rawVal);
};


#endif // CONTROLLERMGR_H