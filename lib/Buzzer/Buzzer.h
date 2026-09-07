/**
 * @file Buzzer.h
 * @brief Deklarasi kelas Buzzer sebagai driver buzzer non-blocking berbasis antrean nada (tone queue).
 *
 * File ini mendefinisikan:
 * - Namespace @ref MusicalNotes yang berisi konstanta frekuensi nada musik (Hz)
 *   yang digunakan untuk membentuk melodi pendek pada buzzer.
 * - Kelas @ref Buzzer yang mengimplementasikan pola Singleton untuk menghasilkan
 *   suara melalui peripheral LEDC ESP32 secara non-blocking, dengan dukungan
 *   antrean nada sehingga beberapa melodi dapat dijadwalkan berurutan.
 *
 * Library ini memanfaatkan fungsi LEDC Arduino-ESP32 (`ledcAttach`,
 * `ledcWriteTone`) sehingga tidak memerlukan timer terpisah untuk membangkitkan
 * frekuensi. Pemutaran nada dilakukan secara asynchronous dengan bantuan
 * fungsi @ref Buzzer::loopPlayNote yang harus dipanggil secara berkala dari
 * loop utama (misalnya `loop()` pada sketch Arduino).
 *
 * @author Tim FRT-OS
 * @date 2026
 */

#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>
#include <freertos/semphr.h>

/**
 * @namespace MusicalNotes
 * @brief Kumpulan konstanta frekuensi nada musik dalam satuan Hertz (Hz).
 *
 * Namespace ini menyediakan tabel frekuensi nada musik dari oktaf 3 hingga
 * oktaf 6, ditambah beberapa frekuensi khusus yang umum digunakan untuk
 * indikator singkat pada buzzer (seperti beep tinggi, menengah, rendah,
 * diam, dan nada sangat rendah).
 *
 * Nilai A4 (440 Hz) dijadikan sebagai nada referensi standar.
 *
 * @note Semua nilai bertipe `uint16_t` karena frekuensi audio yang relevan
 *       untuk buzzer pasif berada pada rentang 0 - 65535 Hz.
 */
namespace MusicalNotes {
    /** @name Oktaf 3 */
    /// @{
    static const uint16_t C3  = 131;   ///< Nada C (Do) oktaf 3, 131 Hz.
    static const uint16_t D3  = 147;   ///< Nada D (Re) oktaf 3, 147 Hz.
    static const uint16_t E3  = 165;   ///< Nada E (Mi) oktaf 3, 165 Hz.
    static const uint16_t F3  = 175;   ///< Nada F (Fa) oktaf 3, 175 Hz.
    static const uint16_t G3  = 196;   ///< Nada G (Sol) oktaf 3, 196 Hz.
    static const uint16_t A3  = 220;   ///< Nada A (La) oktaf 3, 220 Hz.
    static const uint16_t B3  = 247;   ///< Nada B (Si) oktaf 3, 247 Hz.
    /// @}

    /** @name Oktaf 4 */
    /// @{
    static const uint16_t C4  = 262;   ///< Nada C (Do) oktaf 4, 262 Hz.
    static const uint16_t D4  = 294;   ///< Nada D (Re) oktaf 4, 294 Hz.
    static const uint16_t E4  = 330;   ///< Nada E (Mi) oktaf 4, 330 Hz.
    static const uint16_t F4  = 349;   ///< Nada F (Fa) oktaf 4, 349 Hz.
    static const uint16_t G4  = 392;   ///< Nada G (Sol) oktaf 4, 392 Hz.
    static const uint16_t A4  = 440;   ///< Nada A (La) oktaf 4, 440 Hz (nada referensi standar).
    static const uint16_t B4  = 494;   ///< Nada B (Si) oktaf 4, 494 Hz.
    /// @}

    /** @name Oktaf 5 */
    /// @{
    static const uint16_t C5  = 523;   ///< Nada C (Do) oktaf 5, 523 Hz.
    static const uint16_t D5  = 587;   ///< Nada D (Re) oktaf 5, 587 Hz.
    static const uint16_t E5  = 659;   ///< Nada E (Mi) oktaf 5, 659 Hz.
    static const uint16_t F5  = 698;   ///< Nada F (Fa) oktaf 5, 698 Hz.
    static const uint16_t G5  = 784;   ///< Nada G (Sol) oktaf 5, 784 Hz.
    static const uint16_t A5  = 880;   ///< Nada A (La) oktaf 5, 880 Hz.
    static const uint16_t B5  = 988;   ///< Nada B (Si) oktaf 5, 988 Hz.
    /// @}

    /** @name Oktaf 6 */
    /// @{
    static const uint16_t C6  = 1047;  ///< Nada C (Do) oktaf 6, 1047 Hz.
    static const uint16_t D6  = 1175;  ///< Nada D (Re) oktaf 6, 1175 Hz.
    static const uint16_t E6  = 1319;  ///< Nada E (Mi) oktaf 6, 1319 Hz.
    static const uint16_t F6  = 1397;  ///< Nada F (Fa) oktaf 6, 1397 Hz.
    static const uint16_t G6  = 1568;  ///< Nada G (Sol) oktaf 6, 1568 Hz.
    static const uint16_t A6  = 1760;  ///< Nada A (La) oktaf 6, 1760 Hz.
    static const uint16_t B6  = 1976;  ///< Nada B (Si) oktaf 6, 1976 Hz.
    /// @}

    /** @name Frekuensi khusus untuk indikator buzzer */
    /// @{
    static const uint16_t SILENCE    = 0;     ///< Tidak ada suara (diam), 0 Hz.
    static const uint16_t HIGH_BEEP  = 2000;  ///< Beep nada tinggi, 2000 Hz.
    static const uint16_t MID_BEEP   = 1500;  ///< Beep nada menengah, 1500 Hz.
    static const uint16_t LOW_BEEP   = 1000;  ///< Beep nada rendah, 1000 Hz.
    static const uint16_t VERY_LOW   = 500;   ///< Nada sangat rendah, 500 Hz.
    /// @}
}

/**
 * @class Buzzer
 * @brief Driver buzzer non-blocking berbasis antrean nada dengan pola Singleton.
 *
 * Kelas @ref Buzzer menyediakan antarmuka untuk memutar nada/melodi pendek
 * pada buzzer pasif melalui peripheral LEDC ESP32. Pemutaran nada dilakukan
 * secara asynchronous menggunakan mekanisme antrean melingkar (ring buffer)
 * sehingga program utama tidak perlu menunggu (`delay`) selama nada
 * dibunyikan.
 *
 * Fitur utama:
 * - Pola Singleton: cukup panggil @ref getBuzzerInstance untuk mendapatkan
 *   instance tunggal yang digunakan bersama di seluruh program.
 * - Antrean nada hingga @ref MAX_NOTES entri, diputar berurutan oleh
 *   @ref loopPlayNote.
 * - Mendukung perintah preset seperti @ref playStartup, @ref playConnect,
 *   @ref playSaveConfig, @ref playEmergency, dan @ref playWifiRst.
 * - Dapat dihentikan sewaktu-waktu melalui @ref stop.
 *
 * @note Konstruktor bersifat privat dan copy/assignment dihapus untuk
 *       menjamin hanya ada satu instance (Singleton).
 *
 * Contoh penggunaan umum:
 * @code
 * Buzzer::getBuzzerInstance().init();
 * Buzzer::getBuzzerInstance().playConnect();
 *
 * void loop() {
 *     Buzzer::getBuzzerInstance().loopPlayNote();
 *     // ... pekerjaan lain
 * }
 * @endcode
 */
class Buzzer {
    public:
        /**
         * @brief Mengambil instance tunggal (Singleton) dari kelas Buzzer.
         *
         * Instance dibuat secara lokal-statis pada pemanggilan pertama
         * (lazy initialization) sehingga alokasi memori hanya terjadi
         * saat benar-benar dibutuhkan.
         *
         * @return Referensi ke instance @ref Buzzer yang aktif.
         */
        static Buzzer& getBuzzerInstance();

        // Konstruktor copy dan operator assignment dihapus untuk mencegah
        // duplikasi instance dan menjaga invariant Singleton.
        Buzzer(const Buzzer&) = delete;
        Buzzer operator=(const Buzzer&) = delete;

        /**
         * @brief Menginisialisasi pin buzzer dan peripheral LEDC.
         *
         * Mengambil nomor pin dari konstanta `BUZZER_PIN` (didefinisikan
         * pada file pins header), lalu mengkonfigurasi channel LEDC dengan
         * frekuensi default dan resolusi 8-bit, kemudian memastikan buzzer
         * dalam keadaan diam.
         *
         * @note Wajib dipanggil sekali pada @c setup() sebelum pemutaran
         *       nada lainnya dilakukan.
         */
        void init();

        /**
         * @brief Memproses antrean nada secara non-blocking.
         *
         * Fungsi ini harus dipanggil secara berkala (idealnya di setiap
         * iterasi loop utama). Tugasnya:
         * - Mengecek apakah nada yang sedang diputar sudah berakhir
         *   berdasarkan waktu mulai (`millis()`); jika sudah, buzzer
         *   dibuat diam dan status pemutaran di-reset.
         * - Jika buzzer diam dan antrean tidak kosong, mengambil nada
         *   berikutnya dari antrean, memulai pemutaran nada baru,
         *   dan mencatat waktu mulainya.
         *
         * @note Aman untuk sering dipanggil; tidak melakukan apa-apa
         *       ketika tidak ada nada yang harus diputar.
         */
        void loopPlayNote();

        /**
         * @brief Memutar melodi startup secara blocking.
         *
         * Mengosongkan antrean nada terlebih dahulu, kemudian memainkan
         * urutan nada C5 - E5 - G5 dengan jeda pendek di antaranya.
         * Karena menggunakan @c delay(), pemutaran akan memblokir
         * eksekusi hingga selesai (±550 ms).
         *
         * @note Disarankan hanya untuk indikator awal booting.
         */
        void playStartup();

        /**
         * @brief Memutar indikator "koneksi berhasil" secara non-blocking.
         *
         * Mengosongkan antrean, lalu menambahkan dua nada tinggi
         * (HIGH_BEEP) diselingi jeda singkat ke dalam antrean. Nada
         * akan diputar berurutan oleh @ref loopPlayNote.
         */
        void playConnect();

        /**
         * @brief Memutar indikator "konfigurasi tersimpan" secara non-blocking.
         *
         * Mengosongkan antrean, lalu menambahkan dua nada berurutan
         * dari menengah (MID_BEEP) ke rendah (LOW_BEEP) dengan jeda
         * singkat sebagai penanda konfigurasi berhasil disimpan.
         */
        void playSaveConfig();

        /**
         * @brief Memutar nada darurat berdurasi panjang secara non-blocking.
         *
         * Mengosongkan antrean, lalu menambahkan satu nada tinggi
         * (HIGH_BEEP) dengan durasi 5000 ms. Nada akan dibunyikan terus
         * hingga durasi habis atau hingga @ref stop dipanggil.
         */
        void playEmergency();

        /**
         * @brief Memutar indikator "reset WiFi" secara non-blocking.
         *
         * Mengosongkan antrean, lalu menambahkan dua nada tinggi
         * (HIGH_BEEP) yang dipisahkan jeda sebagai konfirmasi bahwa
         * proses reset WiFi telah dipicu.
         */
        void playWifiRst();

        /**
         * @brief Menghentikan pemutaran nada dan mengosongkan antrean.
         *
         * Mengatur status pemutaran menjadi tidak aktif, menghapus
         * seluruh isi antrean nada, dan memaksa buzzer ke kondisi diam.
         *
         * @note Setelah @c stop(), pemutaran berikutnya harus dimulai
         *       ulang dengan memanggil salah satu fungsi play*().
         */
        void stop();

    private:
        uint8_t _pin;  //< Nomor pin GPIO yang digunakan untuk buzzer.
        uint8_t _channel;  ///< Nomor channel LEDC (core v2: dipakai untuk ledcWriteTone/ledcWrite).

        static constexpr uint8_t MAX_NOTES = 16;          //< Kapasitas maksimum antrean nada (16 entri).
        static constexpr uint8_t MAX_NOTES_BITMASK = 0xF;  ///< Bitmask 4-bit (0b1111) untuk operasi modulo kelipatan 16.

        /**
         * @brief Channel LEDC yang dipakai untuk buzzer (core v2 API).
         *
         * Dipakai bersama oleh @ref init, @ref loopPlayNote, dan
         * pemutaran nada langsung (ledcWriteTone). Dipilih 8 supaya
         * tidak konflik dengan channel motor DriveMgr (0..7 untuk 4 motor).
         */
        static constexpr uint8_t BUZZER_LEDC_CHANNEL = 8;

        /**
         * @brief Mutex FreeRTOS untuk thread-safety akses Buzzer.
         *
         * Buzzer diakses dari callback PS3 (BT task - core manapun) dan
         * dari task hardware Core 1. Tanpa mutex, akses ring buffer
         * @ref _noteQueue bisa korup saat preemption.
         *
         * Implementasi: `xSemaphoreCreateMutexStatic` agar alokasi
         * buffer mutex dilakukan pada compile-time / init-time, bukan
         * heap runtime (deterministik, total ~96 byte).
         */
        SemaphoreHandle_t _mutex;
        StaticSemaphore_t _mutexBuffer;

        /**
         * @brief Konstruktor privat (Singleton).
         *
         * Menginisialisasi seluruh variabel internal ke nilai nol/default
         * sehingga objek selalu berada pada kondisi awal yang konsisten
         * sebelum @ref init() dipanggil.
         */
        Buzzer();

        /**
         * @struct ToneNote
         * @brief Representasi satu nada dalam antrean.
         *
         * Struktur ini menyimpan pasangan frekuensi dan durasi nada
         * yang akan dibunyikan oleh buzzer.
         */
        struct ToneNote {
            uint16_t freq;      ///< Frekuensi nada dalam Hertz (Hz); gunakan @ref MusicalNotes::SILENCE untuk diam.
            uint16_t duration;  ///< Durasi pemutaran nada dalam milidetik (ms).
        };

        ToneNote _noteQueue[MAX_NOTES];  ///< Buffer melingkar penyimpan antrean nada.
        uint8_t  _noteQueueHead;         ///< Indeks kepala (tulis) antrean nada.
        uint8_t  _noteQueueTail;         ///< Indeks ekor (baca) antrean nada.
        uint8_t  _noteQueueCount;        ///< Jumlah nada yang sedang tertampung di antrean.

        uint32_t _noteStartTime;         ///< Waktu (`millis()`) saat nada saat ini mulai dibunyikan.

        /**
         * @struct _notePlayState
         * @brief Status pemutaran nada yang sedang aktif.
         *
         * Struktur internal untuk mencatat apakah buzzer sedang
         * membunyikan nada dan berapa durasi total nada tersebut.
         */
        struct _notePlayState{
            uint16_t duration;  ///< Durasi total nada yang sedang diputar (ms).
            bool isPlaying;     ///< @c true jika nada sedang diputar, @c false jika tidak.
        };

        _notePlayState _playState;  ///< Status pemutaran nada yang sedang berjalan.

        /**
         * @brief Menambahkan nada baru ke dalam antrean (internal).
         *
         * Fungsi ini ditulis dengan asumsi antrean belum penuh.
         * Nada akan diletakkan pada posisi @ref _noteQueueHead, kemudian
         * head dimajukan secara melingkar (menggunakan bitmask).
         *
         * @param noteFreq     Frekuensi nada yang akan dibunyikan (Hz),
         *                     gunakan @ref MusicalNotes::SILENCE untuk jeda diam.
         * @param noteDuration Durasi nada dalam milidetik (ms).
         *
         * @note Nada yang dimasukkan tidak langsung dibunyikan; nada akan
         *       diputar saat @ref loopPlayNote memproses antrean.
         */
        void addNote(uint16_t noteFreq, uint16_t noteDuration);

        /**
         * @brief Enqueue pola nada dari array frekuensi & durasi paralel (internal).
         */
        static void enqueuePattern(const uint16_t* freqs, const uint16_t* durs, uint8_t count);

        /**
         * @brief Mengosongkan antrean nada dan mereset status pemutaran (internal).
         *
         * Mengatur ulang head, tail, count, dan status pemutaran ke
         * kondisi awal, serta memaksa output LEDC ke kondisi diam.
         */
        void clearNoteQueue();
};

#endif // BUZZER_H
