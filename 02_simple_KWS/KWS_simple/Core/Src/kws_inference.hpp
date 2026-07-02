#ifndef KWS_INFERENCE_HPP
#define KWS_INFERENCE_HPP

#ifdef __cplusplus
#include <cstdint>
extern "C" {
#else
#include <stdint.h>
#endif

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

#ifdef __cplusplus
}
#endif

#endif /* KWS_INFERENCE_HPP */
