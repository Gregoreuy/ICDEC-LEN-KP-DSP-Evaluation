#include "kws_inference.hpp"

#include "edge-impulse-sdk/classifier/ei_run_classifier.h"

#include <cstdint>
#include <cstdio>
#include <cstdarg>

/* TODO: sesuaikan include ini kalau nama file/handle UART berbeda di project kamu */
extern "C" {
#include "usart.h"   /* untuk deklarasi huart1 (UART_HandleTypeDef) */
}

/* ---------------------------------------------------------------------
 * Buffer perantara: EI SDK minta data float (bukan int16 langsung),
 * dan minta lewat mekanisme get_data callback (bukan pointer langsung),
 * supaya SDK bisa "page-in" data secara bertahap kalau perlu.
 * Karena kita sudah punya semua 16000 sample sekaligus di RAM,
 * callback ini cukup copy dari buffer float statis.
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

    ei_printf("[%s] KWS: run_classifier took %lu ms\r\n", tagStr, (unsigned long)(t_end - t_start));

    if (res != EI_IMPULSE_OK) {
        ei_printf("[%s] KWS: run_classifier failed, error code %d\r\n", tagStr, (int)res);
        return -1;
    }

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
