Sistem kontrol dispenser berbasis perintah suara (voice command), menggantikan antarmuka UI grafis konvensional dengan interaksi suara — misalnya "isi air dingin", "stop", dan perintah lainnya.

Proyek ini dikembangkan menggunakan **STM32U5A9J-DK** (STMicroelectronics Discovery Kit, MCU STM32U5A9NJH6Q, Arm Cortex-M33) dan mikrofon digital **INMP441** (I2S MEMS microphone), sebagai bagian dari Kerja Praktik di PT LEN (Persero) / ICDEC.

## Tujuan Proyek

Proyek ini punya **2 objektif** yang berjalan paralel:

1. **Evaluasi batas kapasitas komputasi Cortex-M33** untuk audio/voice command activation — seberapa besar beban DSP+NN yang bisa ditangani chip ini secara real-time, dinyatakan dalam formula yang transferable (satuan MAC) ke arsitektur model lain.
2. **Implementasi proof-of-concept** pada use case nyata — voice-activated dispenser, dikontrol via suara dan ditampilkan lewat display TouchGFX (bukan aktuator fisik).

## Roadmap Proyek

| Tahap | Status | Deskripsi |
|---|---|---|
| [`01_audio_acquisition`](./01_audio_acquisition) | ✅ Selesai | Akuisisi data audio real-time dari INMP441 via STM32U5A9J-DK, streaming ke PC untuk verifikasi |
| [`02_simpleKWS`](./02_simpleKWS) | ✅ Selesai | Integrasi model Edge Impulse "Yes-No" KWS (4 label), dari static test hingga inference real-time + kontrol LED |
| `03_threadx_touchgfx_display` | ✅ Selesai (baseline) | Migrasi ke RTOS ThreadX + integrasi display TouchGFX, model KWS baseline 4 label berjalan continuous mode dengan tampilan confidence real-time |
| `04_capacity_evaluation` | 🔄 Berjalan (Fase 0 selesai) | Instrumentasi cycle-accurate (DWT) untuk evaluasi batas kapasitas M33 — lihat detail di bawah |
| `05_vui_cascade` | 📋 Direncanakan | Integrasi arsitektur cascade dual-model (wake word + command) sesuai dispenser use case |

## Arsitektur Sistem (Rencana Akhir)

```
[INMP441 Mic] → [SAI1/GPDMA1 (double-buffer)] → [VAD] → [DSP: MFCC] → [NN: CNN]
                                                                            ↓
                                          [State Machine: IDLE → LISTENING → EXECUTING]
                                                                            ↓
                                          [Model A: wake/stop/batal — continuous]
                                          [Model B: command dispenser — one-shot, VAD-gated]
                                                                            ↓
                                                [Tampilan aksi di layar TouchGFX]
```

Model A (5 kelas: `noise`/`unknown`/`wake`/`stop`/`batal`) berjalan terus-menerus sebagai layer deteksi utama. Setelah wake word terdeteksi, Model B (command dispenser) dipicu — durasinya adaptif mengikuti kapan user mulai dan selesai bicara (VAD-gated), bukan window waktu tetap, supaya hemat compute dan tidak terpotong oleh jeda alami antar kata.

---

## Tahap 1: Audio Acquisition

Tahap pertama yang sudah berhasil diselesaikan adalah membangun jalur akuisisi audio yang andal dari mikrofon ke PC, sebagai fondasi sebelum pengumpulan dataset training. Detail teknis lengkap — termasuk konfigurasi hardware, firmware, dan proses debugging — ada di [`01_audio_acquisition`](./01_audio_acquisition).

**Input:**
- Sinyal audio analog dari lingkungan, ditangkap mikrofon digital INMP441 (I2S, 24-bit MSB-first over 32-bit frame)

**Output:**
- Stream audio PCM 16-bit mentah, dikirim dari MCU ke PC via UART1 (921600 baud) melalui ST-LINK Virtual COM Port
- File `.wav` hasil rekaman (disimpan oleh script Python sisi PC dari stream UART tersebut)

**Ringkasan pencapaian:**
- Konfigurasi peripheral SAI1 (I2S) + GPDMA untuk membaca data audio dari INMP441 secara real-time
- Streaming data audio mentah (PCM) dari MCU ke PC melalui UART (921600 baud) via ST-LINK Virtual COM Port
- Script Python untuk menerima stream audio dan menyimpannya sebagai file `.wav`
- Berhasil memecahkan masalah noise pada akuisisi data (root cause: ekstraksi bit yang tidak tepat dari frame SAI 32-bit) — proses debugging lengkap didokumentasikan untuk pembelajaran
- Masalah konektivitas mikrofon INMP441 yang sempat dikira mic rusak, ternyata root cause-nya adalah **suplai 3V3 dari STM32 yang bermasalah** untuk mic — bukan mic itu sendiri. Solusi: pakai sumber 3V3 eksternal (dari board ESP32, dipakai murni sebagai power source, bukan untuk komputasi) untuk mic. SD pin mic juga dipindah dari PE6 ke PD6. Setelah kedua perubahan ini, akuisisi audio real terkonfirmasi bekerja dengan baik

---

## Tahap 2: Simple KWS

Tahap ini mencakup keseluruhan integrasi model machine learning ("Yes-No" keyword spotting classifier dari Edge Impulse, 4 label: `no` / `noise` / `unknown` / `yes`, window 16000 sample @ 16kHz) — mulai dari validasi awal dengan data statis, hingga berjalan penuh secara real-time dari mic langsung dengan output ke LED. Detail ada di [`02_simpleKWS`](./02_simpleKWS).

### 2a. Validasi Static Test

**Input:**
- Array data audio statis (16000 sample, int16) — "Raw Features" yang diambil langsung dari Edge Impulse Studio (Live Classification, sample "yes"), ditanam manual ke firmware sebagai data uji

**Output:**
- Hasil klasifikasi 4 label beserta confidence masing-masing, dicetak via UART (`ei_printf`) — contoh hasil tervalidasi: `yes: 0.91016` (dominan dan benar)
- Waktu eksekusi `run_classifier()` dalam milidetik

**Ringkasan pencapaian:**
- Wrapper C++ (`kws_inference.hpp`/`.cpp`) untuk menjembatani Edge Impulse SDK (C++) dengan `main.c` (C) via `extern "C"`
- Konversi project ke mode kompilasi campuran C + C++ (`arm-none-eabi-g++ -std=gnu++14`)
- Perbaikan beberapa bug integrasi: conflicting linkage `ei_printf`, format output UART, dan dukungan `printf` float (`-u _printf_float`)
- Validasi berhasil: model + SDK + wrapper terhubung benar, confidence sesuai ekspektasi

### 2b. Integrasi Realtime + Kontrol LED

**Input:**
- Audio real-time langsung dari mic INMP441, diakumulasi menjadi window 16000 sample (~1 detik) melalui skema **double-buffer**: satu buffer diisi ISR SAI/DMA, buffer lainnya diproses `run_classifier()` di main loop — didesain supaya perekaman tidak pernah berhenti/kehilangan data selama inference berjalan

**Output:**
- Hasil klasifikasi real-time per window, dicetak via UART dengan tag `[REALTIME]`
- **Kontrol LED on-board** berdasarkan label dominan: label `"no"` → toggle LED merah, label `"yes"` → toggle LED hijau (label `noise`/`unknown` diabaikan)

**Ringkasan pencapaian:**
- Skema akumulasi buffer real-time dari desain single-buffer (ada blind spot ~450ms saat inference berjalan) di-redesign menjadi double-buffer — perekaman audio tidak pernah terputus lagi
- Konflik UART antara streaming audio mentah dan logging teks di peripheral yang sama teridentifikasi dan diperbaiki
- Inference real-time **tervalidasi bekerja sesuai ucapan pengguna** ("yes"/"no")
- Integrasi output ke LED (hijau/merah) sebagai indikator hasil deteksi — dikonfirmasi berfungsi sesuai harapan

---

## Tahap 3: Migrasi ThreadX + Display TouchGFX

Tahap ini memindahkan sistem dari superloop bare-metal ke RTOS (ThreadX/Azure RTOS) — satu-satunya opsi RTOS yang didukung TouchGFX Designer untuk board ini — supaya audio task dan rendering display bisa berjalan konkuren, sekaligus mengganti mode inferensi dari window tetap ke **continuous/sliding window** (update tiap 250ms, bukan tiap 1 detik).

**Input:**
- Audio real-time via SAI1/GPDMA1 (sama seperti Tahap 2), dikonsumsi oleh `AudioTask` (thread ThreadX terpisah)

**Output:**
- Confidence 4 label ditampilkan real-time di layar round TouchGFX (480×480), diperbarui tiap slice (250ms)
- Runtime/uptime sistem (format `HH:MM:SS`) ditampilkan sebagai indikator sistem berjalan normal

**Ringkasan pencapaian:**
- Mode inferensi diganti dari window-based (`run_classifier()`, ~450ms sekali proses) menjadi **continuous** (`run_classifier_continuous()`) — window tetap 1 detik (50 frame), tapi diproses per-slice 250ms dengan overlap 75% antar inferensi, dipercepat drastis ke rentang puluhan milidetik per slice lewat kombinasi continuous mode + ThreadX + optimasi compiler
- Sempat terjadi **regresi fatal** (display mati total) di percobaan integrasi pertama — root cause tidak berhasil ditemukan meski sudah diisolasi bertahap, sehingga project dibangun ulang dari nol dengan disiplin verifikasi tiap langkah
- Dua insiden ThreadX ditemukan & diperbaiki pada rebuild: byte pool memory terlalu kecil (stuck di `tx_application_define()`) dan stack overflow di dalam CMSIS-NN convolution (HardFault) — keduanya diatasi dengan menaikkan ukuran memory pool dan stack thread
- Bridging data MVP (Model → Presenter → View) ke TouchGFX berhasil, termasuk memperbaiki bug tampilan wildcard text (label duplikat, karakter titik desimal tidak muncul karena keterbatasan `Unicode::snprintf` dan font glyph)

---

## Tahap 4: Evaluasi Batas Kapasitas Cortex-M33 (Fase 0)

Tahap ini menjawab objektif 1 proyek — mengukur secara presisi beban komputasi tiap tahap pipeline DSP+NN menggunakan **DWT (Data Watchpoint and Trace)**, unit cycle-accurate counter bawaan core ARM Cortex-M33.

**Input:**
- Firmware yang sudah diinstrumentasi (`DWT->CYCCNT` dipasang di sekeliling tiap tahap: pre-emphasis, framing, FFT, mel filterbank, log+DCT, CMVN, tiap layer NN)

**Output:**
- Breakdown persentase waktu tiap tahap terhadap total waktu inferensi, dikumpulkan lewat UART capture script Python, dianalisis untuk mean/median/p99/jitter

**Ringkasan pencapaian:**
- Infrastruktur instrumentasi DWT selesai dibangun dan tervalidasi (`cycle_timer`, `stage_profile`, SDK Edge Impulse di-patch untuk hook per-stage)
- **Temuan kunci**: CMVN (normalisasi fitur) menghabiskan **62,7%** dari total waktu inferensi, sementara seluruh Neural Network (conv + pool + fc + softmax) hanya **5,1%** — berkebalikan dari asumsi awal bahwa NN adalah bagian terberat. Root cause: window normalisasi CMVN (101 frame) lebih besar dari buffer window inferensi (50 frame), membuatnya secara efektif jadi normalisasi global yang costly
- Eksperimen `-O2` menunjukkan speedup ~20% total, dengan kode DSP naive (CMVN) diuntungkan jauh lebih banyak (-27%) dibanding kode yang sudah dipercepat CMSIS (-12% s.d. -22%)
- Ditemukan sumber kontaminasi pengukuran: TouchGFX (prioritas thread lebih tinggi) bisa memotong AudioTask kapan saja karena tidak ada proteksi preemption-threshold — menyebabkan ~20-25% sample pengukuran tercampur waktu preemption, bukan waktu compute murni. Fix scoped protection (`tx_thread_preemption_change()`) sudah diimplementasikan, validasi ulang masih berjalan

**Isu terbuka:**
- Re-capture data dengan proteksi preemption aktif untuk memvalidasi angka absolut (kesimpulan arah/relatif sudah valid, angka presisi belum)
- Optimasi CMVN (`cmvnw()` → `cmvn()`) belum dieksekusi
- Rekonsiliasi angka timing dari beberapa metode pengukuran berbeda sepanjang proyek — belum dilakukan, dicatat sebagai keterbatasan

---

## Rencana Selanjutnya: Arsitektur VUI Cascade Dual-Model

Untuk menjawab objektif 2 (use case dispenser), sistem akan diperluas dari model tunggal 4-label menjadi **cascade dual-model**:

- **Model A** (continuous, window 1 detik): `noise`, `unknown`, `wake word`, `stop`, `batal` — 5 kelas, tetap berjalan sebagai layer deteksi utama, termasuk quick-check paralel selama state `LISTENING` supaya `stop`/`batal` tetap responsif
- **Model B** (one-shot, dipicu setelah wake word): command dispenser (scope awal 1 kelas — `tuang_air_dingin`), **dipicu oleh VAD** (voice activity detection berbasis energi sinyal) alih-alih window waktu tetap — supaya durasi capture adaptif mengikuti kapan user benar-benar mulai dan selesai bicara, dengan hangover time untuk mentolerir jeda alami antar kata dalam satu frasa, tanpa membebani compute secara terus-menerus

State machine: `IDLE → LISTENING → EXECUTING`, dengan parameter hysteresis (confidence threshold, consecutive frame count) yang masih perlu dikalibrasi dari data empiris.

**Item kerja yang direncanakan:**
1. Rekam dataset sesuai vocabulary cascade (5 kelas Model A + 1 kelas Model B)
2. Implementasi & kalibrasi modul VAD (threshold energi speech-onset, durasi hangover endpoint)
3. Integrasi arsitektur cascade ke firmware
4. Kalibrasi parameter state machine + lanjutkan rencana evaluasi kapasitas (Fase 1-4)
5. Optimasi CMVN

**Sengaja tidak dikerjakan dalam scope saat ini** (dicatat sebagai future work): penjadwalan heater, command set diperluas lebih dari 1 kelas untuk Model B, perbandingan CNN vs DS-CNN secara empiris, evaluasi konsumsi daya.

---

## Hardware yang Digunakan

- **MCU**: STM32U5A9J-DK (Discovery Kit, Cortex-M33 @ 160MHz)
- **Mikrofon**: INMP441 (I2S digital MEMS microphone)
- **Display**: Panel round 480×480 bawaan Discovery Kit (MIPI DSI, TouchGFX)

## Tools & Software

- STM32CubeIDE & STM32CubeMX
- ThreadX (Azure RTOS) & TouchGFX Designer
- Python (untuk tooling sisi PC — audio capture & analisis data DWT)
- Edge Impulse (model KWS, CMSIS-Pack, CMSIS-DSP/CMSIS-NN untuk akselerasi hardware)
