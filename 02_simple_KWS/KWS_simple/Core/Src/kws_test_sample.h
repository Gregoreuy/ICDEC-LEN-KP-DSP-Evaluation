#ifndef KWS_TEST_SAMPLE_H
#define KWS_TEST_SAMPLE_H

#include <stdint.h>

/* Harus sama dengan EI_CLASSIFIER_RAW_SAMPLE_COUNT di model_metadata.h (16000).
 * Ditulis manual di sini (bukan include model_metadata.h langsung) supaya
 * file ini tetap murni C tanpa ketergantungan ke header C++ SDK. Kalau
 * suatu saat model diganti/di-retrain dan RAW_SAMPLE_COUNT berubah,
 * angka ini WAJIB disesuaikan manual juga. */
#define KWS_TEST_SAMPLE_LENGTH 16000

extern const int16_t kws_test_sample[KWS_TEST_SAMPLE_LENGTH];

#endif /* KWS_TEST_SAMPLE_H */
