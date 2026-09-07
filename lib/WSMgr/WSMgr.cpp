#include "WSMgr.h"
#include "ServoMgr.h"
#include "PinConfig.h"
#include "DriveMgr.h"

static const char UI_INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>FRT-OS Controller</title>
    <style>
        * {
            box-sizing: border-box;
            margin: 0;
            padding: 0;
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
        }

        body {
            background-color: #121212;
            color: #e0e0e0;
            padding: 16px;
            font-size: 14px;
            line-height: 1.5;
        }

        .container {
            max-width: 600px;
            margin: 0 auto;
        }

        header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding-bottom: 12px;
            margin-bottom: 16px;
            border-bottom: 1px solid #2e2e2e;
        }

        header h1 {
            font-size: 16px;
            font-weight: 600;
            letter-spacing: 0.5px;
        }

        .status {
            font-size: 12px;
            color: #888;
        }

        .nav-tabs {
            display: flex;
            gap: 6px;
            margin-bottom: 16px;
            overflow-x: auto;
        }

        .nav-tabs button {
            background: #1e1e1e;
            color: #aaa;
            border: 1px solid #333;
            padding: 8px 14px;
            border-radius: 4px;
            cursor: pointer;
            font-size: 12px;
            white-space: nowrap;
        }

        .nav-tabs button.active {
            background: #ffffff;
            color: #000000;
            border-color: #ffffff;
            font-weight: 600;
        }

        .card {
            background: #1a1a1a;
            border: 1px solid #2c2c2c;
            border-radius: 6px;
            padding: 16px;
            margin-bottom: 14px;
        }

        .card h2 {
            font-size: 14px;
            margin-bottom: 12px;
            padding-bottom: 8px;
            border-bottom: 1px solid #2a2a2a;
        }

        .section-box {
            background: #222222;
            border: 1px solid #333333;
            border-radius: 4px;
            padding: 12px;
            margin-bottom: 12px;
        }

        .section-box h3 {
            font-size: 12px;
            color: #bbb;
            margin-bottom: 10px;
            text-transform: uppercase;
        }

        .form-row {
            margin-bottom: 10px;
        }

        .form-row label {
            display: block;
            font-size: 11px;
            color: #999;
            margin-bottom: 4px;
        }

        input[type="text"],
        input[type="password"],
        input[type="number"],
        select,
        textarea {
            width: 100%;
            padding: 8px;
            background: #141414;
            border: 1px solid #3a3a3a;
            color: #fff;
            border-radius: 4px;
            font-size: 13px;
        }

        .grid-2 {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 10px;
        }

        .grid-3 {
            display: grid;
            grid-template-columns: 1fr 1fr 1fr;
            gap: 10px;
        }

        .grid-4 {
            display: grid;
            grid-template-columns: repeat(4, 1fr);
            gap: 8px;
        }

        .checkbox-row {
            display: flex;
            align-items: center;
            justify-content: space-between;
            padding: 8px 10px;
            background: #141414;
            border: 1px solid #333;
            border-radius: 4px;
            margin-bottom: 8px;
        }

        .slider-group {
            margin-bottom: 12px;
        }

        .slider-header {
            display: flex;
            justify-content: space-between;
            font-size: 12px;
            margin-bottom: 4px;
        }

        input[type="range"] {
            width: 100%;
            height: 6px;
            background: #333;
            border-radius: 3px;
            outline: none;
        }

        .btn {
            background: #2a2a2a;
            color: #fff;
            border: 1px solid #444;
            padding: 8px 14px;
            border-radius: 4px;
            cursor: pointer;
            font-size: 12px;
        }

        .btn:hover {
            background: #333;
        }

        .btn-primary {
            width: 100%;
            background: #ffffff;
            color: #000000;
            border: 1px solid #ffffff;
            font-weight: 600;
            padding: 10px;
            margin-top: 6px;
        }

        .btn-primary:hover {
            background: #e0e0e0;
        }

        .btn-danger {
            background: #3a1a1a;
            color: #ff6b6b;
            border-color: #5a2a2a;
        }

        .btn-group {
            display: flex;
            gap: 6px;
            flex-wrap: wrap;
            margin-bottom: 8px;
        }

        .oled-preview {
            background: #000;
            border: 1px solid #333;
            border-radius: 4px;
            padding: 12px;
            font-family: monospace;
            font-size: 12px;
            line-height: 1.6;
            margin-bottom: 12px;
        }

        .tab-content {
            display: none;
        }

        .tab-content.active {
            display: block;
        }

        .toast {
            position: fixed;
            bottom: 20px;
            left: 50%;
            transform: translateX(-50%);
            background: #fff;
            color: #000;
            padding: 8px 16px;
            border-radius: 4px;
            font-size: 12px;
            font-weight: 600;
            display: none;
        }

        .footer {
            text-align: center;
            color: #666;
            font-size: 11px;
            margin-top: 24px;
            padding-bottom: 12px;
        }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>FRT-OS Controller</h1>
            <span class="status">Online (ESP32)</span>
        </header>

        <nav class="nav-tabs">
            <button class="active" onclick="switchTab('gripper')">Gripper</button>
            <button onclick="switchTab('servo')">Servo</button>
            <button onclick="switchTab('joystick')">Joystick</button>
            <button onclick="switchTab('motor')">Motor</button>
            <button onclick="switchTab('pins')">Pin Config</button>
            <button onclick="switchTab('oled')">OLED</button>
            <button onclick="switchTab('wifi')">WiFi</button>
            <button onclick="switchTab('about')">About</button>
        </nav>

        <!-- Gripper Tab -->
        <section id="tab-gripper" class="tab-content active">
            <div class="card">
                <h2>Kontrol Gripper</h2>

                <div class="checkbox-row">
                    <span>Tukar Tombol (Swap X / O)</span>
                    <input type="checkbox" id="swapButtons" onchange="sendCmd('gripper_swap ' + (this.checked ? 1 : 0))">
                </div>

                <div class="section-box">
                    <h3>Gripper 1</h3>
                    <div class="btn-group">
                        <button class="btn" onclick="sendCmd('gripper 1 open'); showToast('Gripper 1 Buka')">Buka</button>
                        <button class="btn" onclick="sendCmd('gripper 1 close'); showToast('Gripper 1 Tutup')">Tutup</button>
                    </div>

                    <div class="slider-group">
                        <div class="slider-header"><span>Servo Besar</span><span id="val_s0">90</span></div>
                        <input type="range" min="0" max="180" value="90" id="s0" oninput="updateSlider('s0', 'val_s0', 0)">
                    </div>
                    <div class="slider-group">
                        <div class="slider-header"><span>Servo Kecil 1</span><span id="val_s1">90</span></div>
                        <input type="range" min="0" max="180" value="90" id="s1" oninput="updateSlider('s1', 'val_s1', 1)">
                    </div>
                    <div class="slider-group">
                        <div class="slider-header"><span>Servo Kecil 2</span><span id="val_s2">90</span></div>
                        <input type="range" min="0" max="180" value="90" id="s2" oninput="updateSlider('s2', 'val_s2', 2)">
                    </div>

                    <div class="checkbox-row">
                        <span>Balik Arah Servo Besar</span>
                        <input type="checkbox" id="inv_g1" onchange="sendCmd('servo_invert 0 ' + (this.checked ? 1 : 0))">
                    </div>

                    <div class="grid-2">
                        <div class="form-row">
                            <label>Sudut Buka</label>
                            <input type="number" id="open_g1" value="150" min="0" max="180">
                        </div>
                        <div class="form-row">
                            <label>Sudut Tutup</label>
                            <input type="number" id="close_g1" value="80" min="0" max="180">
                        </div>
                    </div>
                </div>

                <div class="section-box">
                    <h3>Gripper 2</h3>
                    <div class="btn-group">
                        <button class="btn" onclick="sendCmd('gripper 2 open'); showToast('Gripper 2 Buka')">Buka</button>
                        <button class="btn" onclick="sendCmd('gripper 2 close'); showToast('Gripper 2 Tutup')">Tutup</button>
                    </div>

                    <div class="slider-group">
                        <div class="slider-header"><span>Servo Besar</span><span id="val_s3">90</span></div>
                        <input type="range" min="0" max="180" value="90" id="s3" oninput="updateSlider('s3', 'val_s3', 3)">
                    </div>
                    <div class="slider-group">
                        <div class="slider-header"><span>Servo Kecil 1</span><span id="val_s4">90</span></div>
                        <input type="range" min="0" max="180" value="90" id="s4" oninput="updateSlider('s4', 'val_s4', 4)">
                    </div>
                    <div class="slider-group">
                        <div class="slider-header"><span>Servo Kecil 2</span><span id="val_s5">90</span></div>
                        <input type="range" min="0" max="180" value="90" id="s5" oninput="updateSlider('s5', 'val_s5', 5)">
                    </div>

                    <div class="checkbox-row">
                        <span>Balik Arah Servo Besar</span>
                        <input type="checkbox" id="inv_g2" onchange="sendCmd('servo_invert 3 ' + (this.checked ? 1 : 0))">
                    </div>

                    <div class="grid-2">
                        <div class="form-row">
                            <label>Sudut Buka</label>
                            <input type="number" id="open_g2" value="150" min="0" max="180">
                        </div>
                        <div class="form-row">
                            <label>Sudut Tutup</label>
                            <input type="number" id="close_g2" value="80" min="0" max="180">
                        </div>
                    </div>
                </div>

                <button class="btn-primary" onclick="saveGripperConfig()">Simpan Pengaturan Gripper</button>
            </div>
        </section>

        <!-- Servo Tab -->
        <section id="tab-servo" class="tab-content">
            <div class="card">
                <h2>Kontrol Servo Individu</h2>

                <div class="section-box">
                    <h3>Kontrol Global</h3>
                    <div class="btn-group">
                        <button class="btn" onclick="sendCmd('servo all open'); showToast('Semua Servo Buka')">Buka Semua (180°)</button>
                        <button class="btn" onclick="sendCmd('servo all close'); showToast('Semua Servo Tutup')">Tutup Semua (0°)</button>
                        <button class="btn" onclick="sendCmd('servo all 90'); showToast('Semua Posisi Tengah')">Posisi Tengah (90°)</button>
                    </div>
                </div>

                <div class="section-box">
                    <h3>Gripper 1 - Servo (0, 1, 2)</h3>
                    <div class="slider-group">
                        <div class="slider-header"><span>[0] Servo Besar</span><span id="val_raw_s0">90°</span></div>
                        <input type="range" min="0" max="180" value="90" id="raw_s0" oninput="updateRawServo(0, this.value)">
                    </div>
                    <div class="slider-group">
                        <div class="slider-header"><span>[1] Servo Kecil 1</span><span id="val_raw_s1">90°</span></div>
                        <input type="range" min="0" max="180" value="90" id="raw_s1" oninput="updateRawServo(1, this.value)">
                    </div>
                    <div class="slider-group">
                        <div class="slider-header"><span>[2] Servo Kecil 2</span><span id="val_raw_s2">90°</span></div>
                        <input type="range" min="0" max="180" value="90" id="raw_s2" oninput="updateRawServo(2, this.value)">
                    </div>
                </div>

                <div class="section-box">
                    <h3>Gripper 2 - Servo (3, 4, 5)</h3>
                    <div class="slider-group">
                        <div class="slider-header"><span>[3] Servo Besar</span><span id="val_raw_s3">90°</span></div>
                        <input type="range" min="0" max="180" value="90" id="raw_s3" oninput="updateRawServo(3, this.value)">
                    </div>
                    <div class="slider-group">
                        <div class="slider-header"><span>[4] Servo Kecil 1</span><span id="val_raw_s4">90°</span></div>
                        <input type="range" min="0" max="180" value="90" id="raw_s4" oninput="updateRawServo(4, this.value)">
                    </div>
                    <div class="slider-group">
                        <div class="slider-header"><span>[5] Servo Kecil 2</span><span id="val_raw_s5">90°</span></div>
                        <input type="range" min="0" max="180" value="90" id="raw_s5" oninput="updateRawServo(5, this.value)">
                    </div>
                </div>

                <div class="section-box">
                    <h3>Preset Cepat Per Servo</h3>
                    <div class="grid-2">
                        <div class="form-row">
                            <label>Pilih Servo (0 - 5)</label>
                            <select id="presetServoIdx">
                                <option value="0">Servo 0 (Grip 1 Besar)</option>
                                <option value="1">Servo 1 (Grip 1 Kecil 1)</option>
                                <option value="2">Servo 2 (Grip 1 Kecil 2)</option>
                                <option value="3">Servo 3 (Grip 2 Besar)</option>
                                <option value="4">Servo 4 (Grip 2 Kecil 1)</option>
                                <option value="5">Servo 5 (Grip 2 Kecil 2)</option>
                            </select>
                        </div>
                        <div class="form-row">
                            <label>Aksi Cepat</label>
                            <div class="btn-group">
                                <button class="btn" onclick="setQuickAngle(0)">0°</button>
                                <button class="btn" onclick="setQuickAngle(90)">90°</button>
                                <button class="btn" onclick="setQuickAngle(180)">180°</button>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </section>

        <!-- Joystick Tab -->
        <section id="tab-joystick" class="tab-content">
            <div class="card">
                <h2>Joystick & Respon</h2>

                <div class="section-box">
                    <h3>Sumbu Joystick</h3>
                    <div class="checkbox-row">
                        <span>Tukar Joystick Kiri</span>
                        <input type="checkbox" onchange="sendCmd('joy j1_swap ' + (this.checked ? 1 : 0))">
                    </div>
                    <div class="checkbox-row">
                        <span>Tukar Joystick Kanan</span>
                        <input type="checkbox" onchange="sendCmd('joy j2_swap ' + (this.checked ? 1 : 0))">
                    </div>
                </div>

                <div class="section-box">
                    <h3>Kurva Respon (x1, y1, x2, y2)</h3>
                    <div class="form-row">
                        <input type="text" id="bezierInput" value="0.25, 0.1, 0.25, 1.0" onchange="updateBezier(this.value)">
                    </div>
                    <div class="btn-group">
                        <button class="btn" onclick="setBezierPreset('0,0,1,1')">Linear</button>
                        <button class="btn" onclick="setBezierPreset('0.5,0.0,0.5,1.0')">Soft</button>
                        <button class="btn" onclick="setBezierPreset('0.25,0.1,0.25,1.0')">Standar</button>
                        <button class="btn" onclick="setBezierPreset('0.1,0.9,0.2,1.0')">Agresif</button>
                    </div>
                </div>

                <div class="section-box">
                    <h3>Offset D-Pad (Derajat)</h3>
                    <div class="grid-4">
                        <div class="form-row">
                            <label>Kiri</label>
                            <input type="number" step="0.5" value="0.0" onchange="sendCmd('dpad l ' + this.value)">
                        </div>
                        <div class="form-row">
                            <label>Kanan</label>
                            <input type="number" step="0.5" value="0.0" onchange="sendCmd('dpad r ' + this.value)">
                        </div>
                        <div class="form-row">
                            <label>Atas</label>
                            <input type="number" step="0.5" value="0.0" onchange="sendCmd('dpad u ' + this.value)">
                        </div>
                        <div class="form-row">
                            <label>Bawah</label>
                            <input type="number" step="0.5" value="0.0" onchange="sendCmd('dpad d ' + this.value)">
                        </div>
                    </div>
                </div>

                <button class="btn-primary" onclick="showToast('Pengaturan joystick tersimpan')">Simpan Joystick</button>
            </div>
        </section>

        <!-- Motor Tab -->
        <section id="tab-motor" class="tab-content">
            <div class="card">
                <h2>Kalibrasi Motor</h2>

                <div class="section-box">
                    <h3>Trim Distribusi Berat</h3>
                    <div class="grid-2">
                        <div class="form-row">
                            <label>Balance Depan / Belakang (-1.0 s/d 1.0)</label>
                            <input type="number" step="0.05" min="-1.0" max="1.0" value="0.0">
                        </div>
                        <div class="form-row">
                            <label>Balance Kiri / Kanan (-1.0 s/d 1.0)</label>
                            <input type="number" step="0.05" min="-1.0" max="1.0" value="0.0">
                        </div>
                    </div>
                </div>

                <div class="section-box">
                    <h3>Kecepatan Motor (0 - 255)</h3>
                    <div class="grid-3">
                        <div class="form-row">
                            <label>Utama</label>
                            <input type="number" value="100" min="0" max="255">
                        </div>
                        <div class="form-row">
                            <label>Strafe</label>
                            <input type="number" value="100" min="0" max="255">
                        </div>
                        <div class="form-row">
                            <label>Putar</label>
                            <input type="number" value="100" min="0" max="255">
                        </div>
                    </div>
                </div>

                <button class="btn-primary" onclick="showToast('Pengaturan motor tersimpan')">Simpan Motor</button>
            </div>
        </section>

        <!-- Pin Config Tab -->
        <section id="tab-pins" class="tab-content">
            <div class="card">
                <h2>Konfigurasi Pin Motor & Servo</h2>
                <p style="font-size: 12px; color: #888; margin-bottom: 16px;">
                    Ubah pin GPIO untuk motor dan servo. Perubahan diterapkan langsung tanpa reboot.
                    Pin 34-39 hanya input, tidak bisa dipakai untuk output.
                </p>

                <div class="section-box">
                    <h3>Motor Pins (FL, FR, BL, BR — masing-masing 2 pin: A=maju, B=mundur)</h3>
                    <div class="grid-4">
                        <div class="form-row">
                            <label>FL Pin A</label>
                            <input type="number" id="pin_fl_a" min="0" max="39" value="2">
                        </div>
                        <div class="form-row">
                            <label>FL Pin B</label>
                            <input type="number" id="pin_fl_b" min="0" max="39" value="0">
                        </div>
                        <div class="form-row">
                            <label>FR Pin A</label>
                            <input type="number" id="pin_fr_a" min="0" max="39" value="17">
                        </div>
                        <div class="form-row">
                            <label>FR Pin B</label>
                            <input type="number" id="pin_fr_b" min="0" max="39" value="5">
                        </div>
                        <div class="form-row">
                            <label>BL Pin A</label>
                            <input type="number" id="pin_bl_a" min="0" max="39" value="4">
                        </div>
                        <div class="form-row">
                            <label>BL Pin B</label>
                            <input type="number" id="pin_bl_b" min="0" max="39" value="16">
                        </div>
                        <div class="form-row">
                            <label>BR Pin A</label>
                            <input type="number" id="pin_br_a" min="0" max="39" value="18">
                        </div>
                        <div class="form-row">
                            <label>BR Pin B</label>
                            <input type="number" id="pin_br_b" min="0" max="39" value="19">
                        </div>
                    </div>
                </div>

                <div class="section-box">
                    <h3>Servo Pins (6 servo: Grip1 Big, Small1, Small2, Grip2 Big, Small1, Small2)</h3>
                    <div class="grid-3">
                        <div class="form-row">
                            <label>Servo 0 (Grip1 Big)</label>
                            <input type="number" id="pin_servo_0" min="0" max="39" value="14">
                        </div>
                        <div class="form-row">
                            <label>Servo 1 (Grip1 Small1)</label>
                            <input type="number" id="pin_servo_1" min="0" max="39" value="13">
                        </div>
                        <div class="form-row">
                            <label>Servo 2 (Grip1 Small2)</label>
                            <input type="number" id="pin_servo_2" min="0" max="39" value="12">
                        </div>
                        <div class="form-row">
                            <label>Servo 3 (Grip2 Big)</label>
                            <input type="number" id="pin_servo_3" min="0" max="39" value="27">
                        </div>
                        <div class="form-row">
                            <label>Servo 4 (Grip2 Small1)</label>
                            <input type="number" id="pin_servo_4" min="0" max="39" value="25">
                        </div>
                        <div class="form-row">
                            <label>Servo 5 (Grip2 Small2)</label>
                            <input type="number" id="pin_servo_5" min="0" max="39" value="26">
                        </div>
                    </div>
                </div>

                <div class="btn-group">
                    <button class="btn" onclick="loadPins()">Load Current</button>
                    <button class="btn" onclick="loadDefaultPins()">Load Defaults</button>
                    <button class="btn-primary" onclick="savePins()">Save & Apply</button>
                </div>
            </div>
        </section>

        <!-- OLED Tab -->
        <section id="tab-oled" class="tab-content">
            <div class="card">
                <h2>Layar OLED</h2>

                <div class="oled-preview">
                    <div>[ OLED DISPLAY ]</div>
                    <div id="oled_line1">SYS: READY</div>
                    <div id="oled_line2">IP: 192.168.4.1</div>
                </div>

                <div class="section-box">
                    <div class="form-row">
                        <label>Mode Tampilan</label>
                        <select onchange="sendCmd('oled mode ' + this.value)">
                            <option value="0">Mode Teks</option>
                            <option value="1">Logo Agency</option>
                        </select>
                    </div>

                    <div class="form-row">
                        <label>Teks Kustom</label>
                        <textarea id="oledCustomText" rows="2">FRT-OS</textarea>
                    </div>

                    <button class="btn" onclick="sendOledText()">Kirim Teks ke OLED</button>
                </div>

                <div class="section-box">
                    <h3>Ekspresi</h3>
                    <div class="btn-group">
                        <button class="btn" onclick="sendCmd('oled face happy')">Happy</button>
                        <button class="btn" onclick="sendCmd('oled face angry')">Angry</button>
                        <button class="btn" onclick="sendCmd('oled face sleep')">Sleep</button>
                        <button class="btn" onclick="sendCmd('oled clear')">Clear</button>
                    </div>
                </div>
            </div>
        </section>

        <!-- WiFi Tab -->
        <section id="tab-wifi" class="tab-content">
            <div class="card">
                <h2>Pengaturan WiFi</h2>

                <div class="section-box">
                    <div class="form-row">
                        <label>MAC Controller</label>
                        <input type="text" id="macAddr" value="34:f6:4b:30:3c:f6">
                    </div>
                    <div class="form-row">
                        <label>Mode WiFi</label>
                        <select id="wifiMode" onchange="toggleWifiMode(this.value)">
                            <option value="ap" selected>Access Point (Hotspot)</option>
                            <option value="sta">Station (Hubungkan ke Router)</option>
                        </select>
                    </div>
                    <div class="form-row">
                        <label>SSID</label>
                        <input type="text" id="wifiSsid" value="RTOS-setup">
                    </div>
                    <div class="form-row">
                        <label>Password</label>
                        <input type="password" id="wifiPass" value="" placeholder="Kosongkan jika open">
                    </div>
                </div>

                <button class="btn-primary" onclick="saveWifiConfig()">Simpan & Terapkan</button>
                <button class="btn" style="width: 100%; margin-top: 8px;" onclick="sendCmd('wifi reboot'); showToast('Rebooting ESP32...')">Reboot ESP32</button>
            </div>
        </section>

        <!-- About Tab -->
        <section id="tab-about" class="tab-content">
            <div class="card">
                <h2>Informasi Sistem</h2>
                <div class="section-box">
                    <div class="form-row"><label>Sistem Operasi</label><div>FRT-OS Core</div></div>
                    <div class="form-row" style="margin-top: 8px;"><label>Versi Firmware</label><div>Oswin-0.0.1</div></div>
                </div>

                <div class="section-box">
                    <h3>Reset Pabrik</h3>
                    <p style="font-size: 12px; color: #888; margin-bottom: 10px;">Mengembalikan seluruh parameter ke pengaturan default pabrik.</p>
                    <button class="btn btn-danger" style="width: 100%;" onclick="if(confirm('Reset semua pengaturan ke default?')) { sendCmd('system reset'); showToast('Mereset...'); setTimeout(() => location.reload(), 1500); }">Reset Pabrik</button>
                </div>
            </div>
        </section>

        <div id="toast" class="toast">Disimpan</div>
        <div class="footer">FRT-OS Controller &bull; ESP32</div>
    </div>

    <script>
        function switchTab(tabId) {
            document.querySelectorAll('.tab-content').forEach(function(el) {
                el.classList.remove('active');
            });
            document.querySelectorAll('.nav-tabs button').forEach(function(btn) {
                btn.classList.remove('active');
            });

            var target = document.getElementById('tab-' + tabId);
            if (target) {
                target.classList.add('active');
            }

            if (event && event.target && event.target.tagName === 'BUTTON') {
                event.target.classList.add('active');
            }
        }

        function showToast(message) {
            var toast = document.getElementById('toast');
            toast.innerText = message;
            toast.style.display = 'block';
            setTimeout(function() {
                toast.style.display = 'none';
            }, 1800);
        }

        function sendCmd(cmd) {
            fetch('/command?cmd=' + encodeURIComponent(cmd)).catch(function(err) {
                console.error(err);
            });
        }

        function updateSlider(sliderId, labelId, servoIndex) {
            var val = document.getElementById(sliderId).value;
            document.getElementById(labelId).innerText = val;
            var rawSlider = document.getElementById('raw_s' + servoIndex);
            if (rawSlider) {
                rawSlider.value = val;
                var rawLabel = document.getElementById('val_raw_s' + servoIndex);
                if (rawLabel) rawLabel.innerText = val + '°';
            }
            sendCmd('servo ' + servoIndex + ' ' + val);
        }

        function updateRawServo(index, val) {
            var label = document.getElementById('val_raw_s' + index);
            if (label) label.innerText = val + '°';
            var mainSlider = document.getElementById('s' + index);
            if (mainSlider) {
                mainSlider.value = val;
                var mainLabel = document.getElementById('val_s' + index);
                if (mainLabel) mainLabel.innerText = val;
            }
            sendCmd('servo ' + index + ' ' + val);
        }

        function setQuickAngle(angle) {
            var idx = document.getElementById('presetServoIdx').value;
            var slider = document.getElementById('raw_s' + idx);
            if (slider) slider.value = angle;
            updateRawServo(idx, angle);
            showToast('Servo ' + idx + ' set ke ' + angle + '°');
        }

        function saveGripperConfig() {
            var swap = document.getElementById('swapButtons').checked ? 1 : 0;
            var inv1 = document.getElementById('inv_g1').checked ? 1 : 0;
            var open1 = document.getElementById('open_g1').value;
            var close1 = document.getElementById('close_g1').value;
            var inv2 = document.getElementById('inv_g2').checked ? 1 : 0;
            var open2 = document.getElementById('open_g2').value;
            var close2 = document.getElementById('close_g2').value;

            var cmd = 'gripper save ' + swap + ' ' + inv1 + ' ' + open1 + ' ' + close1 + ' ' + inv2 + ' ' + open2 + ' ' + close2;
            sendCmd(cmd);
            showToast('Pengaturan gripper tersimpan');
        }

        function updateBezier(val) {
            sendCmd('joy bezier ' + val);
        }

        function setBezierPreset(val) {
            document.getElementById('bezierInput').value = val;
            updateBezier(val);
        }

        function sendOledText() {
            var text = document.getElementById('oledCustomText').value;
            sendCmd('oled custom_text "' + text + '"');
            document.getElementById('oled_line2').innerText = text;
            showToast('Teks dikirim ke OLED');
        }

        function toggleWifiMode(mode) {
            var ssid = document.getElementById('wifiSsid');
            if (mode === 'ap') {
                ssid.value = 'RTOS-setup';
            } else {
                ssid.value = 'Home_WiFi';
            }
        }

        function saveWifiConfig() {
            var mode = document.getElementById('wifiMode').value;
            var ssid = document.getElementById('wifiSsid').value;
            var pass = document.getElementById('wifiPass').value;
            var mac = document.getElementById('macAddr').value;

            sendCmd('wifi save ' + mode + ' "' + ssid + '" "' + pass + '" "' + mac + '"');
            showToast('Pengaturan WiFi tersimpan');
        }

        // Pin Config functions
        function loadPins() {
            fetch('/api/pins')
                .then(response => response.json())
                .then(data => {
                    // Motor pins
                    document.getElementById('pin_fl_a').value = data.motors.fl_a;
                    document.getElementById('pin_fl_b').value = data.motors.fl_b;
                    document.getElementById('pin_fr_a').value = data.motors.fr_a;
                    document.getElementById('pin_fr_b').value = data.motors.fr_b;
                    document.getElementById('pin_bl_a').value = data.motors.bl_a;
                    document.getElementById('pin_bl_b').value = data.motors.bl_b;
                    document.getElementById('pin_br_a').value = data.motors.br_a;
                    document.getElementById('pin_br_b').value = data.motors.br_b;
                    // Servo pins
                    for (var i = 0; i < 6; i++) {
                        document.getElementById('pin_servo_' + i).value = data.servos[i];
                    }
                    showToast('Pin config loaded');
                })
                .catch(err => {
                    console.error(err);
                    showToast('Failed to load pins');
                });
        }

        function loadDefaultPins() {
            // Defaults from RaggedyPins.h
            document.getElementById('pin_fl_a').value = 2;
            document.getElementById('pin_fl_b').value = 0;
            document.getElementById('pin_fr_a').value = 17;
            document.getElementById('pin_fr_b').value = 5;
            document.getElementById('pin_bl_a').value = 4;
            document.getElementById('pin_bl_b').value = 16;
            document.getElementById('pin_br_a').value = 18;
            document.getElementById('pin_br_b').value = 19;
            document.getElementById('pin_servo_0').value = 14;
            document.getElementById('pin_servo_1').value = 13;
            document.getElementById('pin_servo_2').value = 12;
            document.getElementById('pin_servo_3').value = 27;
            document.getElementById('pin_servo_4').value = 25;
            document.getElementById('pin_servo_5').value = 26;
            showToast('Default pins loaded');
        }

        function savePins() {
            var data = {
                motors: {
                    fl_a: parseInt(document.getElementById('pin_fl_a').value),
                    fl_b: parseInt(document.getElementById('pin_fl_b').value),
                    fr_a: parseInt(document.getElementById('pin_fr_a').value),
                    fr_b: parseInt(document.getElementById('pin_fr_b').value),
                    bl_a: parseInt(document.getElementById('pin_bl_a').value),
                    bl_b: parseInt(document.getElementById('pin_bl_b').value),
                    br_a: parseInt(document.getElementById('pin_br_a').value),
                    br_b: parseInt(document.getElementById('pin_br_b').value)
                },
                servos: [
                    parseInt(document.getElementById('pin_servo_0').value),
                    parseInt(document.getElementById('pin_servo_1').value),
                    parseInt(document.getElementById('pin_servo_2').value),
                    parseInt(document.getElementById('pin_servo_3').value),
                    parseInt(document.getElementById('pin_servo_4').value),
                    parseInt(document.getElementById('pin_servo_5').value)
                ]
            };

            var jsonStr = JSON.stringify(data);
            fetch('/api/pins', {
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: 'data=' + encodeURIComponent(jsonStr)
            })
            .then(response => response.text())
            .then(text => {
                if (text === 'ok') {
                    showToast('Pin config saved & applied');
                } else {
                    showToast('Error: ' + text);
                }
            })
            .catch(err => {
                console.error(err);
                showToast('Failed to save pins');
            });
        }
    </script>
</body>
</html>
)rawliteral";

static AsyncWebServer server(80);

static void handleCommand(AsyncWebServerRequest* request) {
    if (!request->hasParam("cmd")) {
        request->send(400, "text/plain", "no cmd");
        return;
    }

    String cmd = request->getParam("cmd")->value();

    int idx = 0;
    int angle = 0;
    int unit = 0;
    int inv = 0;
    char action[16]{};

    if (sscanf(cmd.c_str(), "servo %d %d", &idx, &angle) == 2) {
        servoSetAngle(idx, angle);
    } else if (strcmp(cmd.c_str(), "servo all open") == 0) {
        servoOpenAll();
    } else if (strcmp(cmd.c_str(), "servo all close") == 0) {
        servoCloseAll();
    } else if (sscanf(cmd.c_str(), "servo all %d", &angle) == 1) {
        servoSetAllAngle(angle);
    } else if (sscanf(cmd.c_str(), "gripper %d %15s", &unit, action) == 2) {
        if (strcmp(action, "open") == 0) {
            servoOpenGripper(unit);
        } else if (strcmp(action, "close") == 0) {
            servoCloseGripper(unit);
        }
    } else if (sscanf(cmd.c_str(), "servo_invert %d %d", &idx, &inv) == 2) {
        int cur = servoGetAngle(idx);
        bool open = servoIsOpen(idx);
        servoSetConfig(idx, inv != 0, 0, open ? cur : 150, open ? 80 : cur);
    } else if (cmd.startsWith("gripper save ")) {
        int swap = 0, inv1 = 0, open1 = 150, close1 = 80;
        int inv2 = 0, open2 = 150, close2 = 80;
        if (sscanf(cmd.c_str(), "gripper save %d %d %d %d %d %d %d",
                   &swap, &inv1, &open1, &close1, &inv2, &open2, &close2) == 7) {
            servoSetConfig(GRIP1_BIG, inv1 != 0, 0, open1, close1);
            servoSetConfig(GRIP2_BIG, inv2 != 0, 0, open2, close2);
        }
    }

    request->send(200, "text/plain", "ok");
}

void handlePinsGet(AsyncWebServerRequest* request) {
    PinConfig cfg = PinConfigMgr::getInstance().load();
    
    String json = "{";
    json += "\"motors\":{\"fl_a\":" + String(cfg.motors.fl_a) + ",\"fl_b\":" + String(cfg.motors.fl_b) + 
            ",\"fr_a\":" + String(cfg.motors.fr_a) + ",\"fr_b\":" + String(cfg.motors.fr_b) +
            ",\"bl_a\":" + String(cfg.motors.bl_a) + ",\"bl_b\":" + String(cfg.motors.bl_b) +
            ",\"br_a\":" + String(cfg.motors.br_a) + ",\"br_b\":" + String(cfg.motors.br_b) + "}";
    json += ",\"servos\":[";
    for (int i = 0; i < 6; i++) {
        json += String(cfg.servos.pins[i]);
        if (i < 5) json += ",";
    }
    json += "]}";
    
    request->send(200, "application/json", json);
}

void handlePinsPost(AsyncWebServerRequest* request) {
    if (!request->hasParam("data", true)) {
        request->send(400, "text/plain", "missing data");
        return;
    }
    
    String body = request->getParam("data", true)->value();
    
    // Simple JSON parsing for our known structure
    PinConfig cfg;
    
    // Parse motor pins
    auto extractInt = [&](const String& str, const char* key, int defVal) -> int {
        int idx = str.indexOf(key);
        if (idx < 0) return defVal;
        idx = str.indexOf(':', idx);
        if (idx < 0) return defVal;
        idx++;
        while (idx < str.length() && (str[idx] == ' ' || str[idx] == '\t')) idx++;
        int val = 0;
        bool neg = false;
        if (str[idx] == '-') { neg = true; idx++; }
        while (idx < str.length() && isDigit(str[idx])) {
            val = val * 10 + (str[idx] - '0');
            idx++;
        }
        return neg ? -val : val;
    };
    
    cfg.motors.fl_a = extractInt(body, "\"fl_a\"", cfg.motors.fl_a);
    cfg.motors.fl_b = extractInt(body, "\"fl_b\"", cfg.motors.fl_b);
    cfg.motors.fr_a = extractInt(body, "\"fr_a\"", cfg.motors.fr_a);
    cfg.motors.fr_b = extractInt(body, "\"fr_b\"", cfg.motors.fr_b);
    cfg.motors.bl_a = extractInt(body, "\"bl_a\"", cfg.motors.bl_a);
    cfg.motors.bl_b = extractInt(body, "\"bl_b\"", cfg.motors.bl_b);
    cfg.motors.br_a = extractInt(body, "\"br_a\"", cfg.motors.br_a);
    cfg.motors.br_b = extractInt(body, "\"br_b\"", cfg.motors.br_b);
    
    // Parse servo pins array
    int servoIdx = body.indexOf("\"servos\"");
    if (servoIdx >= 0) {
        servoIdx = body.indexOf('[', servoIdx);
        if (servoIdx >= 0) {
            servoIdx++;
            for (int i = 0; i < 6; i++) {
                while (servoIdx < body.length() && (body[servoIdx] == ' ' || body[servoIdx] == '\t' || body[servoIdx] == ',')) servoIdx++;
                int val = 0;
                while (servoIdx < body.length() && isDigit(body[servoIdx])) {
                    val = val * 10 + (body[servoIdx] - '0');
                    servoIdx++;
                }
                cfg.servos.pins[i] = val;
            }
        }
    }
    
    // Validate
    String errorMsg;
    if (!PinConfigMgr::getInstance().validate(cfg, errorMsg)) {
        request->send(400, "text/plain", errorMsg);
        return;
    }
    
    // Apply to hardware
    MotorPins motorPins[MOTOR_COUNT];
    motorPins[MOTOR_IDX_FL] = {cfg.motors.fl_a, cfg.motors.fl_b};
    motorPins[MOTOR_IDX_FR] = {cfg.motors.fr_a, cfg.motors.fr_b};
    motorPins[MOTOR_IDX_BL] = {cfg.motors.bl_a, cfg.motors.bl_b};
    motorPins[MOTOR_IDX_BR] = {cfg.motors.br_a, cfg.motors.br_b};
    
    bool ok = true;
    ok &= drive.reinitPins(motorPins, 10000, 8);
    ok &= servoReinitPins(cfg.servos.pins);
    
    if (ok) {
        PinConfigMgr::getInstance().save(cfg);
        request->send(200, "text/plain", "ok");
    } else {
        request->send(500, "text/plain", "hardware reinit failed");
    }
}

void wsMgrBegin() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(200, "text/html", UI_INDEX_HTML);
    });

    server.on("/command", HTTP_GET, [](AsyncWebServerRequest* request) {
        handleCommand(request);
    });

    server.on("/api/pins", HTTP_GET, handlePinsGet);
    server.on("/api/pins", HTTP_POST, handlePinsPost);

    server.begin();
}