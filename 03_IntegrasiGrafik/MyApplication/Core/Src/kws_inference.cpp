#include "kws_inference.hpp"

#include "edge-impulse-sdk/classifier/ei_run_classifier.h"

#include <cstdint>
#include <cstdio>
#include <cstdarg>
#include <cstddef>

/* TODO: sesuaikan include ini kalau nama file/handle UART berbeda di project kamu */
extern "C" {
#include "main.h"
extern UART_HandleTypeDef huart1;
}

#if EI_CLASSIFIER_TFLITE_ENABLE_CMSIS_NN == 1
  #pragma message "CMSIS_NN = AKTIF"
#else
  #pragma message "CMSIS_NN = NONAKTIF"
#endif

#if EIDSP_USE_CMSIS_DSP == 1
  #pragma message "CMSIS_DSP = AKTIF"
#else
  #pragma message "CMSIS_DSP = NONAKTIF"
#endif

/* ---------------------------------------------------------------------
 * MODE LAMA (window-based, run_classifier() biasa) -- dipakai untuk
 * STATIC_TEST di main.c. Buffer perantara: EI SDK minta data float
 * (bukan int16 langsung), dan minta lewat mekanisme get_data callback
 * (bukan pointer langsung), supaya SDK bisa "page-in" data secara
 * bertahap kalau perlu. Karena kita sudah punya semua 16000 sample
 * sekaligus di RAM, callback ini cukup copy dari buffer float statis.
 * ------------------------------------------------------------------- */
static float ei_features_buffer[EI_CLASSIFIER_RAW_SAMPLE_COUNT];

static int get_signal_data(size_t offset, size_t length, float *out_ptr) {
    for (size_t i = 0; i < length; i++) {
        out_ptr[i] = ei_features_buffer[offset + i];
    }
    return 0;
}

void KWS_Init(void) {
    /* Placeholder. Kalau nanti pindah ke run_classifier_continuous(),
     * panggil run_classifier_init() di sini. Untuk run_classifier() biasa
     * (window-based, bukan sliding), ini tidak wajib diisi. */
}

int KWS_RunInference(int16_t *audio_buffer, uint32_t buffer_size, const char *tag) {
    const char *tagStr = (tag != NULL) ? tag : "UNSPECIFIED";

    if (buffer_size != EI_CLASSIFIER_RAW_SAMPLE_COUNT) {
        ei_printf("[%s] KWS: buffer_size mismatch, got %lu, expected %d\r\n",
                  tagStr, (unsigned long)buffer_size, EI_CLASSIFIER_RAW_SAMPLE_COUNT);
        return -1;
    }

    /* Konversi int16 -> float. Model Edge Impulse untuk audio biasanya
     * expect raw PCM value langsung (tidak dinormalisasi ke -1..1) -
     * ini SESUAI cara Edge Impulse Studio training dari data mentah int16.
     * Kalau hasil klasifikasi nanti aneh/selalu salah, ini salah satu titik
     * yang perlu dicurigai & dicek ulang terhadap dokumentasi resmi. */
    for (uint32_t i = 0; i < buffer_size; i++) {
        ei_features_buffer[i] = (float)audio_buffer[i];
    }

    signal_t signal;
    signal.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
    signal.get_data = &get_signal_data;

    ei_impulse_result_t result = { 0 };

    uint32_t t_start = HAL_GetTick();
    EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false /* debug */);
    uint32_t t_end = HAL_GetTick();
    ei_printf("[%s] KWS: awawk took %lu ms\r\n", tagStr, (unsigned long)(t_end - t_start));
    ei_printf("[%s] ===MARKER_CEK_INI_12345===\r\n", tagStr);


    if (res != EI_IMPULSE_OK) {
        ei_printf("[%s] KWS: run_classifier failed, error code %d\r\n", tagStr, (int)res);
        return -1;
    }
    // TAMBAHAN: breakdown durasi DSP vs NN
    ei_printf("[%s] KWS: DSP=%lu us, NN=%lu us\r\n", tagStr,
              (unsigned long)result.timing.dsp_us,
              (unsigned long)result.timing.classification_us);

    int best_idx = -1;
    float best_val = -1.0f;

    ei_printf("[%s] KWS result:\r\n", tagStr);
    for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        ei_printf("  %s: %.5f\r\n",
                  result.classification[i].label,
                  result.classification[i].value);

        if (result.classification[i].value > best_val) {
            best_val = result.classification[i].value;
            best_idx = (int)i;
        }
    }

    return best_idx;
}


/* ---------------------------------------------------------------------
 * MODE BARU (continuous/sliding-window, run_classifier_continuous()) --
 * dipakai untuk REALTIME. Prinsip sama seperti buffer perantara di
 * atas (get_data callback baca dari buffer float statis), tapi
 * ukurannya SLICE_SIZE (4000), bukan RAW_SAMPLE_COUNT (16000) --
 * karena signal yang dikasih ke SDK sekarang cuma 1 slice, bukan 1
 * window penuh. FIFO window penuhnya dikelola SENDIRI oleh SDK di
 * dalam run_classifier_continuous(), kita tidak perlu (dan tidak
 * boleh) menyimpan window penuh di sisi kita lagi.
 * ------------------------------------------------------------------- */
static float ei_slice_features_buffer[EI_CLASSIFIER_SLICE_SIZE];

static int get_signal_data_slice(size_t offset, size_t length, float *out_ptr) {
    for (size_t i = 0; i < length; i++) {
        out_ptr[i] = ei_slice_features_buffer[offset + i];
    }
    return 0;
}

void KWS_InitContinuous(void) {
    /* WAJIB dipanggil sekali sebelum slice pertama -- reset state internal
     * SDK (slice_offset, FIFO, moving average filter, dst). Beda dari
     * KWS_Init() di atas yang cuma placeholder kosong untuk mode lama. */
    run_classifier_init();
}

int KWS_RunInferenceContinuousSlice(int16_t *slice_buffer, uint32_t slice_size,
                                     const char *tag, float *out_confidence) {
    const char *tagStr = (tag != NULL) ? tag : "UNSPECIFIED";

    if (slice_size != EI_CLASSIFIER_SLICE_SIZE) {
        ei_printf("[%s] KWS: slice_size mismatch, got %lu, expected %d\r\n",
                  tagStr, (unsigned long)slice_size, EI_CLASSIFIER_SLICE_SIZE);
        return -1;
    }

    /* Konversi int16 -> float, sama seperti mode lama (raw PCM value
     * langsung, tidak dinormalisasi -1..1) -- lihat catatan di
     * KWS_RunInference() soal ini. */
    for (uint32_t i = 0; i < slice_size; i++) {
        ei_slice_features_buffer[i] = (float)slice_buffer[i];
    }

    signal_t signal;
    signal.total_length = EI_CLASSIFIER_SLICE_SIZE;
    signal.get_data = &get_signal_data_slice;

    ei_impulse_result_t result = { 0 };

    uint32_t t_start = HAL_GetTick();
    /* enable_maf (moving average filter) dibiarkan default (true) --
     * ini yang meratakan output supaya tidak gampang false-positive
     * dari 1 slice yang kebetulan mirip suatu label. Lihat Isu A di
     * dokumen proyek -- threshold confidence eksplisit MASIH belum ada,
     * MAF ini baru filter tambahan, bukan pengganti threshold. */
    EI_IMPULSE_ERROR res = run_classifier_continuous(&signal, &result, false /* debug */);
    uint32_t t_end = HAL_GetTick();

    if (res != EI_IMPULSE_OK) {
        ei_printf("[%s] KWS: run_classifier_continuous failed, error code %d\r\n",
                  tagStr, (int)res);
        return -1;
    }

    int best_idx = -1;
    float best_val = -1.0f;
    for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        if (result.classification[i].value > best_val) {
            best_val = result.classification[i].value;
            best_idx = (int)i;
        }
    }

    if (out_confidence != NULL) {
        *out_confidence = best_val;
    }

    /* --- FASE DEBUGGING: print TIAP slice, TANPA throttle ---
     * Sebelumnya di-throttle (cuma print 1 dari tiap N panggilan), tapi
     * ini punya masalah: LED toggle & klasifikasi tetap jalan di SETIAP
     * slice, sedangkan print cuma nongol di slice tertentu -- jadi kalau
     * confidence puncak dari ucapan user kebetulan ada di slice yang
     * TIDAK di-print, hasilnya toggle LED benar tapi user tidak pernah
     * lihat angka logits-nya di UART. Untuk fase debugging sekarang,
     * mending print SEMUA slice dulu (4x/detik, ~5 baris tiap kali,
     * masih cukup terbaca) supaya tidak ada hasil yang kelewat.
     *
     * Setelah pipeline ini dipastikan jalan sesuai ekspektasi, baru
     * pertimbangkan throttle lagi -- TAPI dengan strategi yang lebih
     * baik dari sekadar "tiap N slice" (misal: cuma print kalau label
     * pemenang BERUBAH dari panggilan sebelumnya, supaya tidak ada
     * event yang kelewat tapi UART tetap tidak banjir).
     */
    ei_printf("[%s] KWS(slice): took %lu ms\r\n", tagStr, (unsigned long)(t_end - t_start));
    for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        ei_printf("  %s: %.5f\r\n",
                  result.classification[i].label,
                  result.classification[i].value);
    }

    return best_idx;
}


/* ---------------------------------------------------------------------
 * Override ei_printf (default-nya weak & kosong di SDK).
 * Redirect ke UART1, reuse huart1 yang sudah ada dari project akuisisi lama
 * (USART1, 921600 baud).
 * ------------------------------------------------------------------- */
void ei_printf(const char *format, ...) {
    static char buf[256];
    va_list args;
    va_start(args, format);
    int len = vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    if (len > 0) {
        if (len > (int)sizeof(buf)) {
            len = sizeof(buf);
        }
        HAL_UART_Transmit(&huart1, (uint8_t *)buf, (uint16_t)len, HAL_MAX_DELAY);
    }
}
