#include "Buzzer.h"
#include "RaggedyPins.h"

// === Pola nada buzzer (frekuensi & durasi paralel) ===
// Indeks i yang sama pada <name>Freqs[] dan <name>Durs[] membentuk satu nada.
// Kapasitas dijaga < MAX_NOTES (16) sehingga muat di satu kali enqueue.

// Startup: ascending ceria (C4 -> E4 -> G4 -> C5) ~610 ms total
static const uint16_t startupFreqs[] = {
    MusicalNotes::C4, MusicalNotes::E4, MusicalNotes::G4, MusicalNotes::C5
};
static const uint16_t startupDurs[]  = { 120, 120, 120, 250 };

// Connected: konfirmasi tinggi cepat (G5 -> C6) ~250 ms total
static const uint16_t connectFreqs[] = {
    MusicalNotes::G5, MusicalNotes::C6
};
static const uint16_t connectDurs[]  = { 100, 150 };

// Save Config: beep menurun (MID -> LOW) ~320 ms total
static const uint16_t saveCfgFreqs[] = {
    MusicalNotes::MID_BEEP, MusicalNotes::LOW_BEEP
};
static const uint16_t saveCfgDurs[]  = { 120, 200 };

// Emergency: staccato berulang + nada panjang di akhir ~1.55 s total
static const uint16_t emergencyFreqs[] = {
    MusicalNotes::HIGH_BEEP, MusicalNotes::SILENCE,
    MusicalNotes::HIGH_BEEP, MusicalNotes::SILENCE,
    MusicalNotes::HIGH_BEEP, MusicalNotes::SILENCE,
    MusicalNotes::HIGH_BEEP
};
static const uint16_t emergencyDurs[]  = { 150, 100, 150, 100, 150, 100, 800 };

// Wifi Reset: 2x beep tinggi konfirmasi ~380 ms total
static const uint16_t wifiRstFreqs[] = {
    MusicalNotes::HIGH_BEEP, MusicalNotes::SILENCE, MusicalNotes::HIGH_BEEP
};
static const uint16_t wifiRstDurs[]  = { 100, 80, 200 };

// Frekuensi default & resolusi LEDC untuk nada buzzer.
static constexpr uint32_t DEFAULT_BUZZER_FREQ     = 2000;
static constexpr uint8_t  LEDC_RESOLUTION         = 8;

// Alur: static-local -> dibuat sekali, return ref ke instance yang sama.
Buzzer& Buzzer::getBuzzerInstance(){
    static Buzzer instance;
    return instance;
}

// Alur: inisialisasi semua member ke nol/default; mutex belum dibuat
// (akan dibuat oleh init() di main thread).
Buzzer::Buzzer()
    : _pin(0), _channel(BUZZER_LEDC_CHANNEL),
      _noteQueueHead(0), _noteQueueTail(0),
      _noteQueueCount(0), _noteStartTime(0),
      _playState{0, false}, _mutex(nullptr) {}

// Alur: ambil pin BUZZER_PIN -> buat mutex statis -> setup LEDC channel
// -> attach pin ke channel -> set nada diam.
void Buzzer::init(){
    _pin = BUZZER_PIN;

    // Mutex statis: alokasi buffer pada compile-time (StaticSemaphore_t),
    // bukan heap runtime. Total ~96 byte, deterministik.
    if (_mutex == nullptr) {
        _mutex = xSemaphoreCreateMutexStatic(&_mutexBuffer);
    }

    // Core v2 API: ledcSetup + ledcAttachPin per pin,
    //              ledcWrite/ledcWriteTone per channel.
    ledcSetup(BUZZER_LEDC_CHANNEL, DEFAULT_BUZZER_FREQ, LEDC_RESOLUTION);
    ledcAttachPin(_pin, BUZZER_LEDC_CHANNEL);
    ledcWriteTone(BUZZER_LEDC_CHANNEL, MusicalNotes::SILENCE);
}

// Alur: kalau antrean penuh -> keluar. Kalau belum, tulis nada di head,
// lalu geser head (mod 16 via mask) dan naikkan count.
void Buzzer::addNote(uint16_t noteFreq, uint16_t noteDuration){
    if(_noteQueueCount >= MAX_NOTES) return;

    _noteQueue[_noteQueueHead].freq     = noteFreq;
    _noteQueue[_noteQueueHead].duration = noteDuration;

    _noteQueueHead = (_noteQueueHead + 1) & MAX_NOTES_BITMASK;
    _noteQueueCount++;
}

// Alur: reset head/tail/count + status play, lalu paksa LEDC diam.
void Buzzer::clearNoteQueue(){
    _noteQueueHead        = 0;
    _noteQueueTail        = 0;
    _noteQueueCount       = 0;
    _playState.duration   = 0;
    _playState.isPlaying  = false;

    ledcWriteTone(BUZZER_LEDC_CHANNEL, MusicalNotes::SILENCE);
}

// Alur utama:
// 1) Kalau sedang play & durasi sudah habis -> set diam, LEDC silence.
// 2) Kalau sudah diam & antrean ada isinya -> ambil nada dari tail,
//    geser tail, turunkan count, nyalakan nada baru, catat waktu mulai.
//
// Thread-safety: dipanggil hanya dari Core 1 task. Tidak perlu lock di
// sini karena hanya 1 writer/reader untuk _noteQueue. Tapi untuk
// konsistensi dengan callback PS3 yang bisa memanggil clearNoteQueue()
// lewat playXxx(), kita lock mutex saat enqueue/dequeue.
void Buzzer::loopPlayNote(){
    uint32_t current_time = millis();

    if(_playState.isPlaying){
        if(current_time - _noteStartTime >= _playState.duration){
            _playState.isPlaying = false;
            ledcWriteTone(BUZZER_LEDC_CHANNEL, MusicalNotes::SILENCE);
        }
    }

    if (!_playState.isPlaying && _noteQueueCount > 0)
    {
        // Lock hanya saat dequeue karena playXxx() bisa enqueue dari
        // core lain lewat callback PS3.
        if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
            ToneNote currentNote = _noteQueue[_noteQueueTail];

            _noteQueueTail = (_noteQueueTail + 1) & MAX_NOTES_BITMASK;
            _noteQueueCount--;

            xSemaphoreGive(_mutex);

            ledcWriteTone(BUZZER_LEDC_CHANNEL, currentNote.freq);

            // Update kondisi pemutaran nada
            _playState.duration = currentNote.duration;
            _playState.isPlaying = true;
            _noteStartTime = current_time;
        }
    }
}

// Enqueue seluruh isi array (frekuensi, durasi) ke antrean nada (member method).
void Buzzer::enqueuePattern(const uint16_t* freqs, const uint16_t* durs, uint8_t count){
    Buzzer& instance = getBuzzerInstance();
    for(uint8_t i = 0; i < count; ++i){
        instance.addNote(freqs[i], durs[i]);
    }
}

// Nada startup: ascending ceria (C4 -> E4 -> G4 -> C5) via antrean.
void Buzzer::playStartup(){
    if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
        clearNoteQueue();
        enqueuePattern(startupFreqs, startupDurs, sizeof(startupFreqs)/sizeof(startupFreqs[0]));
        xSemaphoreGive(_mutex);
    }
}

// Connected: konfirmasi tinggi singkat (G5 -> C6) via antrean.
void Buzzer::playConnect(){
    if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
        clearNoteQueue();
        enqueuePattern(connectFreqs, connectDurs, sizeof(connectFreqs)/sizeof(connectFreqs[0]));
        xSemaphoreGive(_mutex);
    }
}

// Save Config: beep menurun (MID -> LOW) via antrean.
void Buzzer::playSaveConfig(){
    if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
        clearNoteQueue();
        enqueuePattern(saveCfgFreqs, saveCfgDurs, sizeof(saveCfgFreqs)/sizeof(saveCfgFreqs[0]));
        xSemaphoreGive(_mutex);
    }
}

// Emergency: staccato berulang + nada panjang via antrean.
void Buzzer::playEmergency(){
    if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
        clearNoteQueue();
        enqueuePattern(emergencyFreqs, emergencyDurs, sizeof(emergencyFreqs)/sizeof(emergencyFreqs[0]));
        xSemaphoreGive(_mutex);
    }
}

// Wifi Reset: 2x beep tinggi konfirmasi via antrean.
void Buzzer::playWifiRst(){
    if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
        clearNoteQueue();
        enqueuePattern(wifiRstFreqs, wifiRstDurs, sizeof(wifiRstFreqs)/sizeof(wifiRstFreqs[0]));
        xSemaphoreGive(_mutex);
    }
}

// Menghentikan semua nada dan mengosongkan antrean
// Alur: clear queue -> paksa status diam lagi (idempotent) -> LEDC silence.
void Buzzer::stop(){
    if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
        clearNoteQueue();
        _playState.isPlaying = false;
        _playState.duration = 0;
        xSemaphoreGive(_mutex);
    }
    ledcWriteTone(BUZZER_LEDC_CHANNEL, MusicalNotes::SILENCE);
}
