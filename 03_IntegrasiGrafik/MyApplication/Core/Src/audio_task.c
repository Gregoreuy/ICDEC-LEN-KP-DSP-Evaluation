#include "audio_task.h"
#include "main.h"
#include "kws_inference.hpp"
#include <string.h>

/* hsai_BlockA1 didefinisikan di main.c, tidak ada extern otomatis
 * di main.h karena project ini pakai mode "generate as single file" */
extern SAI_HandleTypeDef hsai_BlockA1;

/* Shared state untuk dibaca TouchGFX Model */
volatile int32_t  g_kwsLastLabelIndex    = -1;
volatile float    g_kwsLastConfidence    = 0.0f;
volatile uint32_t g_kwsInferenceCount    = 0;
volatile uint32_t g_kwsSliceOverrunCount = 0;

/* Buffer raw 32-bit dari SAI (24-bit data INMP441 MSB-first di [31:8]).
 * Dua kali ukuran slice supaya half-complete/complete callback DMA
 * membagi buffer ini jadi dua paruh yang saling bergantian diisi. */
#define FULL_BUFFER_SAMPLES  (KWS_SLICE_SAMPLES * 2)
static uint32_t audioBufferRaw[FULL_BUFFER_SAMPLES];

/* Slice buffer int16 double-buffer, hasil ekstraksi dari audioBufferRaw,
 * ini yang dikonsumsi KWS_RunInferenceContinuousSlice() */
static int16_t kwsSliceBuffer[2][KWS_SLICE_SAMPLES];

/* Semaphore sinyal dari callback (ISR context) ke AudioTask_Entry (thread) */
static TX_SEMAPHORE sliceReadySemaphore;

/* Index buffer mana yang baru selesai diisi & siap diinferensi */
static volatile uint8_t sliceReadyIndex = 0;
/* Flag apakah AudioTask masih memproses slice sebelumnya saat slice
 * baru datang -- kalau iya, hitung sebagai overrun (data lama dilewati) */
static volatile uint8_t sliceBusy = 0;

/* LED feedback pin -- sesuai definisi GPIO di main.c: LED_GREEN_Pin/LED_RED_Pin di GPIOE */
static void KWS_LedFeedback(int32_t labelIndex)
{
    /* Sesuaikan mapping label->LED ini dengan urutan label model kamu */
    if (labelIndex == 0)
    {
        HAL_GPIO_WritePin(GPIOE, LED_GREEN_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOE, LED_RED_Pin, GPIO_PIN_RESET);
    }
    else if (labelIndex == 1)
    {
        HAL_GPIO_WritePin(GPIOE, LED_GREEN_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOE, LED_RED_Pin, GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(GPIOE, LED_GREEN_Pin | LED_RED_Pin, GPIO_PIN_RESET);
    }
}

/* Ekstraksi 16-bit dari raw 32-bit SAI: data 24-bit MSB-first ada di
 * bit [31:8], padding nol di [7:0] -- ambil 16 bit teratas saja */
static void ExtractSlice(const uint32_t *rawHalf, int16_t *outSlice)
{
    for (uint32_t i = 0; i < KWS_SLICE_SAMPLES; i++)
    {
        outSlice[i] = (int16_t)(rawHalf[i] >> 16);
    }
}

void HAL_SAI_RxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
    if (hsai->Instance != SAI1_Block_A) return;

    if (sliceBusy)
    {
        g_kwsSliceOverrunCount++;
    }

    /* Paruh pertama buffer (index 0 .. KWS_SLICE_SAMPLES-1) baru selesai diisi DMA */
    ExtractSlice(&audioBufferRaw[0], kwsSliceBuffer[0]);
    sliceReadyIndex = 0;
    sliceBusy = 1;
    tx_semaphore_put(&sliceReadySemaphore);
}

void HAL_SAI_RxCpltCallback(SAI_HandleTypeDef *hsai)
{
    if (hsai->Instance != SAI1_Block_A) return;

    if (sliceBusy)
    {
        g_kwsSliceOverrunCount++;
    }

    /* Paruh kedua buffer (index KWS_SLICE_SAMPLES .. 2*KWS_SLICE_SAMPLES-1) baru selesai diisi DMA */
    ExtractSlice(&audioBufferRaw[KWS_SLICE_SAMPLES], kwsSliceBuffer[1]);
    sliceReadyIndex = 1;
    sliceBusy = 1;
    tx_semaphore_put(&sliceReadySemaphore);
}

void HAL_SAI_ErrorCallback(SAI_HandleTypeDef *hsai)
{
    if (hsai->Instance != SAI1_Block_A) return;
    /* TODO: tambahkan penanganan error/recovery kalau diperlukan
     * (misal restart HAL_SAI_Receive_DMA), untuk sekarang cukup dicatat */
    g_kwsSliceOverrunCount++;
}

void AudioTask_Entry(ULONG thread_input)
{
    (void)thread_input;

    tx_semaphore_create(&sliceReadySemaphore, "SliceReadySem", 0);

    KWS_InitContinuous();

    memset(audioBufferRaw, 0, sizeof(audioBufferRaw));

    if (HAL_SAI_Receive_DMA(&hsai_BlockA1, (uint8_t *)audioBufferRaw,
                             FULL_BUFFER_SAMPLES) != HAL_OK)
    {
        Error_Handler();
    }

    for (;;)
    {
        if (tx_semaphore_get(&sliceReadySemaphore, TX_WAIT_FOREVER) == TX_SUCCESS)
        {
            uint8_t idx = sliceReadyIndex;
            float confidence = 0.0f;

            int labelIndex = KWS_RunInferenceContinuousSlice(
                kwsSliceBuffer[idx], KWS_SLICE_SAMPLES, "AudioTask", &confidence);

            g_kwsLastLabelIndex = labelIndex;
            g_kwsLastConfidence = confidence;
            g_kwsInferenceCount++;

            KWS_LedFeedback(labelIndex);

            sliceBusy = 0;
        }
    }
}
