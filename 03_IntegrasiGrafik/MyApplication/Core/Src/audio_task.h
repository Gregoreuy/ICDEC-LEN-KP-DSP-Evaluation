#ifndef AUDIO_TASK_H
#define AUDIO_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "tx_api.h"

/* Harus sama persis dengan EI_CLASSIFIER_SLICE_SIZE dari model_metadata.h */
#define KWS_SLICE_SAMPLES   4000

/* Shared state, dibaca TouchGFX Model nanti (read-only dari sisi Model) */
extern volatile int32_t  g_kwsLastLabelIndex;
extern volatile float    g_kwsLastConfidence;
extern volatile uint32_t g_kwsInferenceCount;
extern volatile uint32_t g_kwsSliceOverrunCount;

/* Entry point thread ThreadX, didaftarkan di App_ThreadX_Init() */
void AudioTask_Entry(ULONG thread_input);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_TASK_H */