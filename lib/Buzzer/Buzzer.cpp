#include "Buzzer.h"
#include "RaggedyPins.h"

// Frekuensi default & resolusi LEDC untuk nada buzzer.
static constexpr uint32_t DEFAULT_BUZZER_FREQ     = 2000;
static constexpr uint8_t  LEDC_RESOLUTION         = 8;

// Alur: static-local -> dibuat sekali, return ref ke instance yang sama.
Buzzer& Buzzer::getBuzzerInstance(){
    static Buzzer instance;
    return instance;
}

// Alur: inisialisasi semua member ke nilai nol/default (status diam).
Buzzer::Buzzer()
    : _pin(0), _noteQueueHead(0), _noteQueueTail(0),
      _noteQueueCount(0), _noteStartTime(0), _playState{0, false} {}

// Alur: ambil pin BUZZER_PIN -> attach LEDC -> set nada diam.
void Buzzer::init(){
    _pin = BUZZER_PIN;

    ledcAttach(_pin, DEFAULT_BUZZER_FREQ, LEDC_RESOLUTION);
    ledcWriteTone(_pin, MusicalNotes::SILENCE);
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

    ledcWriteTone(_pin, MusicalNotes::SILENCE);
}

// Alur utama:
// 1) Kalau sedang play & durasi sudah habis -> set diam, LEDC silence.
// 2) Kalau sudah diam & antrean ada isinya -> ambil nada dari tail,
//    geser tail, turunkan count, nyalakan nada baru, catat waktu mulai.
void Buzzer::loopPlayNote(){
    uint32_t current_time = millis();

    if(_playState.isPlaying){
        if(current_time - _noteStartTime >= _playState.duration){
            _playState.isPlaying = false;
            ledcWriteTone(_pin, MusicalNotes::SILENCE);
        }
    }

    if (!_playState.isPlaying && _noteQueueCount > 0)
    {
        ToneNote currentNote = _noteQueue[_noteQueueTail];

        _noteQueueTail = (_noteQueueTail + 1) & MAX_NOTES_BITMASK;
        _noteQueueCount--;

        ledcWriteTone(_pin, currentNote.freq);

        // Update kondisi pemutaran nada
        _playState.duration = currentNote.duration;
        _playState.isPlaying = true;
        _noteStartTime = current_time;
    }
}

// Nada startup (blocking/langsung) - Menggunakan namespace MusicalNotes
// Alur: clear queue -> putar C5, jeda, E5, jeda, G5, lalu diam.
void Buzzer::playStartup(){
    clearNoteQueue();
    ledcWriteTone(_pin, MusicalNotes::C5);
    delay(100);
    ledcWriteTone(_pin, MusicalNotes::SILENCE);
    delay(50);
    ledcWriteTone(_pin, MusicalNotes::E5);
    delay(100);
    ledcWriteTone(_pin, MusicalNotes::SILENCE);
    delay(50);
    ledcWriteTone(_pin, MusicalNotes::G5);
    delay(200);
    ledcWriteTone(_pin, MusicalNotes::SILENCE);
}

// 2x beep tinggi (non-blocking via queue)
// Alur: clear queue -> masukkan HIGH, SILENCE, HIGH ke antrean.
void Buzzer::playConnect(){
    clearNoteQueue();
    addNote(MusicalNotes::HIGH_BEEP, 100);
    addNote(MusicalNotes::SILENCE, 50);
    addNote(MusicalNotes::HIGH_BEEP, 100);
}

// Beep turun (Simpan Konfigurasi)
// Alur: clear queue -> masukkan MID, SILENCE, LOW ke antrean.
void Buzzer::playSaveConfig(){
    clearNoteQueue();
    addNote(MusicalNotes::MID_BEEP, 100);
    addNote(MusicalNotes::SILENCE, 50);
    addNote(MusicalNotes::LOW_BEEP, 100);
}

// Tone panjang 5 detik
// Alur: clear queue -> masukkan satu nada HIGH berdurasi 5000 ms.
void Buzzer::playEmergency(){
    clearNoteQueue();
    addNote(MusicalNotes::HIGH_BEEP, 5000);
}

// 2x beep konfirmasi reset WiFi
// Alur: clear queue -> masukkan HIGH, SILENCE, HIGH ke antrean.
void Buzzer::playWifiRst(){
    clearNoteQueue();
    addNote(MusicalNotes::HIGH_BEEP, 100);
    addNote(MusicalNotes::SILENCE, 100);
    addNote(MusicalNotes::HIGH_BEEP, 100);
}

// Menghentikan semua nada dan mengosongkan antrean
// Alur: clear queue -> paksa status diam lagi (idempotent) -> LEDC silence.
void Buzzer::stop(){
    clearNoteQueue();
    _playState.isPlaying = false;
    _playState.duration = 0;
    ledcWriteTone(_pin, MusicalNotes::SILENCE);
}
