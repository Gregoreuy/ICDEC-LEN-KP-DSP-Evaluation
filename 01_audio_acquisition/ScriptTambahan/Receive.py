"""
Script penerima audio dari STM32U5A9J-DK via Serial (ST-LINK VCP).
Menyimpan raw PCM 16-bit mono yang diterima menjadi file .wav
supaya bisa didengar dan diverifikasi.

Cara pakai:
1. pip install pyserial
2. Sesuaikan COM_PORT di bawah (lihat di Device Manager / Tera Term, mis. "COM5")
3. Jalankan: python receive_audio.py
4. Tekan Ctrl+C untuk berhenti merekam, file .wav otomatis tersimpan

Parameter harus SAMA PERSIS dengan konfigurasi firmware:
- Baud rate: 921600
- Sample rate: 16666 Hz (real audio frequency hasil CubeMX, bukan 16000)
- 16-bit, mono
"""

import serial
import wave
import time
import sys

# ============================================================
# KONFIGURASI -- sesuaikan dengan setup Anda
# ============================================================
COM_PORT = "COM5"          # Ganti sesuai port yang muncul di Tera Term/Device Manager
BAUD_RATE = 921600
SAMPLE_RATE = 16666        # Real Audio Frequency dari CubeMX (bukan 16000)
SAMPLE_WIDTH = 2           # 16-bit = 2 byte per sample
CHANNELS = 1                # Mono
OUTPUT_FILE = "rekaman_inmp441.wav"
RECORD_SECONDS = 10         # Durasi rekam (detik). Set None untuk rekam sampai Ctrl+C
# ============================================================


def main():
    print(f"Membuka {COM_PORT} @ {BAUD_RATE} baud...")
    try:
        ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=1)
    except serial.SerialException as e:
        print(f"GAGAL membuka port: {e}")
        print("Cek: apakah Tera Term/serial monitor lain sedang membuka port yang sama?")
        print("Tutup dulu aplikasi lain yang memakai port ini.")
        sys.exit(1)

    print(f"Port terbuka. Merekam ke '{OUTPUT_FILE}'...")
    print("Tekan Ctrl+C untuk berhenti lebih awal.\n")

    audio_data = bytearray()
    start_time = time.time()
    last_report = start_time
    total_bytes = 0

    try:
        while True:
            if RECORD_SECONDS is not None and (time.time() - start_time) >= RECORD_SECONDS:
                print("\nDurasi rekam tercapai.")
                break

            chunk = ser.read(4096)  # baca apa pun yang tersedia di buffer
            if chunk:
                audio_data.extend(chunk)
                total_bytes += len(chunk)

            # Laporan progres tiap 1 detik
            now = time.time()
            if now - last_report >= 1.0:
                elapsed = now - start_time
                kbps = (total_bytes / 1024) / elapsed if elapsed > 0 else 0
                print(f"  [{elapsed:5.1f}s] Total diterima: {total_bytes/1024:.1f} KB  (~{kbps:.1f} KB/s)")
                last_report = now

    except KeyboardInterrupt:
        print("\nDihentikan oleh user (Ctrl+C).")

    finally:
        ser.close()

    if len(audio_data) == 0:
        print("\nTIDAK ADA DATA DITERIMA. Cek:")
        print("  - Apakah firmware sudah benar-benar jalan di board (LED tanda hidup, dst)?")
        print("  - Apakah COM_PORT sudah benar?")
        print("  - Apakah ada aplikasi lain yang mengunci port ini?")
        return

    # Pastikan jumlah byte genap (kelipatan 2, karena 16-bit per sample)
    if len(audio_data) % 2 != 0:
        audio_data = audio_data[:-1]

    # Simpan sebagai WAV
    with wave.open(OUTPUT_FILE, "wb") as wf:
        wf.setnchannels(CHANNELS)
        wf.setsampwidth(SAMPLE_WIDTH)
        wf.setframerate(SAMPLE_RATE)
        wf.writeframes(bytes(audio_data))

    duration = len(audio_data) / (SAMPLE_RATE * SAMPLE_WIDTH * CHANNELS)
    print(f"\nSelesai. File tersimpan: {OUTPUT_FILE}")
    print(f"Total data: {len(audio_data)/1024:.1f} KB, durasi audio: {duration:.2f} detik")
    print("\nBuka file ini dengan media player apa pun (VLC, Windows Media Player, dll)")
    print("untuk verifikasi apakah suara terdengar jelas atau hanya noise/silence.")


if __name__ == "__main__":
    main()