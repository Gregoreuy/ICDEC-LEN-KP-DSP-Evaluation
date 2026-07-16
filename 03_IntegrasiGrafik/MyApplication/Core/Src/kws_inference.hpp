#ifndef KWS_INFERENCE_HPP
#define KWS_INFERENCE_HPP

#ifdef __cplusplus
#include <cstdint>
extern "C" {
#else
#include <stdint.h>
#endif

/* ============================================================================
 * MODE LAMA (window-based, run_classifier() biasa) -- TETAP DIPERTAHANKAN,
 * dipakai untuk STATIC_TEST di main.c (validasi sekali di awal, tidak
 * depend ke mic). TIDAK dipakai lagi untuk REALTIME setelah pindah ke
 * continuous mode di bawah.
 * ============================================================================
 */

/* Panggil sekali di awal, sebelum inference pertama.
 * (Placeholder untuk sekarang - hanya dibutuhkan kalau nanti pakai
 * run_classifier_continuous(), untuk run_classifier() biasa tidak wajib) */
void KWS_Init(void);

/* Jalankan inference sekali atas satu window penuh.
 * audio_buffer : pointer ke buffer int16 hasil ekstraksi dari SAI
 *                (>>16 dari word 32-bit, sesuai baseline project akuisisi)
 * buffer_size  : jumlah sample, HARUS == EI_CLASSIFIER_RAW_SAMPLE_COUNT (16000)
 * tag          : string pendek untuk membedakan sumber panggilan di log UART
 *                (misal "STATIC_TEST" atau "REALTIME") -- boleh NULL kalau
 *                tidak perlu dibedakan.
 *
 * Return: index label dengan confidence tertinggi, atau -1 kalau error/mismatch.
 * (Sementara juga print hasil semua label via ei_printf untuk debug UART)
 */
int KWS_RunInference(int16_t *audio_buffer, uint32_t buffer_size, const char *tag);


/* ============================================================================
 * MODE BARU (continuous/sliding-window, run_classifier_continuous()) --
 * dipakai untuk REALTIME mulai sekarang. Alasan pindah: kata perintah
 * dispenser nanti ("panaskan air", dst) lebih panjang & rawan sample-nya
 * terpotong di batas window kalau pakai mode lama. Continuous mode pakai
 * FIFO rolling window dari slice-slice kecil, jadi tidak ada batas window
 * yang "keras" seperti mode lama.
 *
 * PENTING: mode ini TIDAK memperpanjang durasi window model itu sendiri
 * (window tetap ditentukan EI_CLASSIFIER_RAW_SAMPLE_COUNT / SLICES_PER_
 * MODEL_WINDOW dari model_metadata.h, hasil training di Studio). Continuous
 * mode cuma mengubah CARA window itu diisi (slice + overlap), bukan
 * KAPASITASnya. Kalau kata makin panjang dari window model, tetap perlu
 * retrain dengan window lebih besar (lihat Isu D di dokumen proyek).
 * ============================================================================
 */

/* Panggil SEKALI di awal, SEBELUM slice pertama masuk ke
 * KWS_RunInferenceContinuousSlice(). WAJIB untuk continuous mode (beda
 * dari KWS_Init() di atas yang cuma placeholder untuk mode lama). */
void KWS_InitContinuous(void);

/* Jalankan continuous inference atas SATU SLICE (bukan window penuh).
 * slice_buffer   : pointer ke buffer int16 hasil ekstraksi, ukuran HARUS
 *                  == EI_CLASSIFIER_SLICE_SIZE (bukan RAW_SAMPLE_COUNT)
 * slice_size     : jumlah sample slice ini, HARUS == EI_CLASSIFIER_SLICE_SIZE
 * tag            : sama seperti KWS_RunInference
 * out_confidence : (opsional, boleh NULL) kalau diisi, akan berisi nilai
 *                  confidence dari label pemenang (index yang direturn)
 *
 * Return: index label dengan confidence tertinggi DARI HASIL FIFO SAAT INI,
 * atau -1 kalau error/mismatch.
 *
 * CATATAN PENTING: hasil baru "penuh makna" setelah FIFO terisi minimal
 * 1 window penuh -- yaitu setelah EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW
 * slice pertama terkumpul (di project ini: 4 slice pertama, ~1 detik
 * pertama sejak boot/reset). Sebelum itu, FIFO masih sebagian kosong
 * (biasanya diisi nol), jadi hasil beberapa panggilan PERTAMA boleh
 * dianggap "warm-up" dan diabaikan oleh pemanggil kalau perlu.
 */
int KWS_RunInferenceContinuousSlice(int16_t *slice_buffer, uint32_t slice_size,
                                     const char *tag, float *out_confidence);

#ifdef __cplusplus
}
#endif

#endif /* KWS_INFERENCE_HPP */
