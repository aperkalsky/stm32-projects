#ifndef INC_SPEECH_CONV_DEFS_H_
#define INC_SPEECH_CONV_DEFS_H_

#define SPEECH_TEXT_MAX       256U
#define PCM_STREAM_SIZE       2048U
#define SAM_CHUNK_SIZE        256U
#define AUDIO_DMA_SAMPLES     512U
#define AUDIO_HALF_SAMPLES    (AUDIO_DMA_SAMPLES / 2U)
#define AUDIO_START_THRESHOLD AUDIO_DMA_SAMPLES

#define AUDIO_EVT_DATA  (1UL << 0)
#define AUDIO_EVT_HALF  (1UL << 1)
#define AUDIO_EVT_FULL  (1UL << 2)
#define AUDIO_EVT_STOP  (1UL << 3)

typedef struct
{
    char text[SPEECH_TEXT_MAX + 1U];
    uint32_t request_id;
} SpeechRequest_t;

#endif
