Sistem kontrol dispenser berbasis perintah suara (voice command), menggantikan antarmuka UI grafis konvensional dengan interaksi suara — misalnya "isi air panas", "stop", dan perintah lainnya.

Proyek ini dikembangkan menggunakan **STM32U5A9J-DK** (STMicroelectronics Discovery Kit, MCU STM32U5A9NJH6Q, Arm Cortex-M33) dan mikrofon digital **INMP441** (I2S MEMS microphone), dengan target akhir berupa model machine learning untuk keyword spotting/voice command yang berjalan secara on-device (edge AI).

## Roadmap Proyek

| Tahap | Status | Deskripsi |
|---|---|---|
| [`01_audio_acquisition`](./01_audio_acquisition) | ✅ Selesai | Akuisisi data audio real-time dari INMP441 via STM32U5A9J-DK, streaming ke PC untuk verifikasi |
| [`02_simpleKWS`](./02_simpleKWS) | ✅ Selesai | Integrasi model Edge Impulse "Yes-No" KWS, dari static test hingga inference real-time + kontrol LED |

## Arsitektur Sistem (Rencana Akhir)

```
[INMP441 Mic] → [STM32U5A9J-DK: Audio Capture + ML Inference]
                            ↓
                  [Voice Command Recognized]
                            ↓
                  [Kontrol Aktuator Dispenser]
```

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
- **(Update)** Masalah konektivitas mikrofon INMP441 yang sempat dikira mic rusak, ternyata root cause-nya adalah **suplai 3V3 dari STM32 yang bermasalah** untuk mic — bukan mic itu sendiri. Solusi: pakai sumber 3V3 eksternal (dari board ESP32, dipakai murni sebagai power source, bukan untuk komputasi) untuk mic. SD pin mic juga dipindah dari PE6 ke PD6. Setelah kedua perubahan ini, akuisisi audio real terkonfirmasi bekerja dengan baik

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
- Hasil klasifikasi real-time per window (~1 detik sekali), dicetak via UART dengan tag `[REALTIME]`
- **Kontrol LED on-board** berdasarkan label dominan: label `"no"` → toggle LED merah, label `"yes"` → toggle LED hijau (label `noise`/`unknown` diabaikan)

**Ringkasan pencapaian:**
- Skema akumulasi buffer real-time dari desain single-buffer (ada blind spot ~450ms saat inference berjalan) di-redesign menjadi double-buffer — perekaman audio tidak pernah terputus lagi
- Konflik UART antara streaming audio mentah (`HAL_UART_Transmit_DMA`) dan logging teks (`HAL_UART_Transmit` blocking) di peripheral yang sama teridentifikasi dan diperbaiki (streaming audio mentah dinonaktifkan sementara)
- Inference real-time **tervalidasi bekerja sesuai ucapan pengguna** ("yes"/"no") — dikonfirmasi lewat pengujian langsung
- Integrasi output ke LED (hijau/merah) sebagai indikator hasil deteksi — dikonfirmasi berfungsi sesuai harapan

**Isu terbuka / belum selesai** (dicatat untuk pengembangan lanjutan, tidak menghalangi status tahap ini dianggap selesai):
- **Belum ada threshold confidence** — sistem saat ini murni argmax (ambil label dengan confidence tertinggi apa pun nilainya), belum ada mekanisme "tidak yakin" untuk menghindari false positive saat silence/noise ambient
- **Mismatch sample rate**: frekuensi sampling real terukur (logic analyzer) ~16.2kHz, sementara model dilatih untuk 16kHz — dampaknya ke akurasi belum dievaluasi secara sistematis, keputusan resampling masih ditunda sampai ada evaluasi empiris lebih lanjut
- Durasi `run_classifier()` konsisten terukur ~450ms — sudah ditangani lewat double-buffer, tapi durasi ini sendiri belum dioptimasi (opsi `run_classifier_continuous()` belum dieksplorasi)

---

## Hardware yang Digunakan

- **MCU**: STM32U5A9J-DK (Discovery Kit)
- **Mikrofon**: INMP441 (I2S digital MEMS microphone)

## Tools & Software

- STM32CubeIDE & STM32CubeMX
- Python (untuk tooling sisi PC)
- Edge Impulse (model "Yes-No" KWS classifier, CMSIS-Pack)