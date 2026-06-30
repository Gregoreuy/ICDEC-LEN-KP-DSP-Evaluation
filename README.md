Sistem kontrol dispenser berbasis perintah suara (voice command), menggantikan antarmuka UI grafis konvensional dengan interaksi suara — misalnya "isi air panas", "stop", dan perintah lainnya.

Proyek ini dikembangkan menggunakan **STM32U5A9J-DK** (STMicroelectronics Discovery Kit, MCU STM32U5A9NJH6Q, Arm Cortex-M33) dan mikrofon digital **INMP441** (I2S MEMS microphone), dengan target akhir berupa model machine learning untuk keyword spotting/voice command yang berjalan secara on-device (edge AI).

## Roadmap Proyek

| Tahap | Status | Deskripsi |
|---|---|---|
| [`01_audio_acquisition`](./01_audio_acquisition) | ✅ Selesai | Akuisisi data audio real-time dari INMP441 via STM32U5A9J-DK, streaming ke PC untuk verifikasi |
| `02_SimpleKWS` | 🔜 Berikutnya | Mencoba Key Word Spotting Sederhana |

## Arsitektur Sistem (Rencana Akhir)

```
[INMP441 Mic] → [STM32U5A9J-DK: Audio Capture + ML Inference]
                            ↓
                  [Voice Command Recognized]
                            ↓
                  [Kontrol Aktuator Dispenser]
```

## Tahap Saat Ini: Audio Acquisition

Tahap pertama yang sudah berhasil diselesaikan adalah membangun jalur akuisisi audio yang andal dari mikrofon ke PC, sebagai fondasi sebelum pengumpulan dataset training. Detail teknis lengkap — termasuk konfigurasi hardware, firmware, dan proses debugging — ada di [`01_audio_acquisition/README.md`](./01_audio_acquisition/README.md).

**Ringkasan pencapaian:**
- Konfigurasi peripheral SAI1 (I2S) + GPDMA untuk membaca data audio dari INMP441 secara real-time
- Streaming data audio mentah (PCM) dari MCU ke PC melalui UART (921600 baud) via ST-LINK Virtual COM Port
- Script Python untuk menerima stream audio dan menyimpannya sebagai file `.wav`
- Berhasil memecahkan masalah noise pada akuisisi data (root cause: ekstraksi bit yang tidak tepat dari frame SAI 32-bit) — proses debugging lengkap didokumentasikan untuk pembelajaran

## Hardware yang Digunakan

- **MCU**: STM32U5A9J-DK (Discovery Kit)
- **Mikrofon**: INMP441 (I2S digital MEMS microphone)

## Tools & Software

- STM32CubeIDE & STM32CubeMX
- Python (untuk tooling sisi PC)
- Edge Impulse (rencana, untuk tahap training model)

---