#ifndef INC_SPEECH_CONV_TASK_H_
#define INC_SPEECH_CONV_TASK_H_

#include "FreeRTOS.h"
#include <queue.h>
#include <stream_buffer.h>
#include "speech_conv_defs.h"

extern QueueHandle_t speechQueue;
extern StreamBufferHandle_t pcmStream;
extern volatile bool audioUnderrun;
//extern TaskHandle_t audioTaskHandle;
extern bool audioStarted;
extern uint16_t audioDma[AUDIO_DMA_SAMPLES];

void Speech_Init(void);
void SpeechConversionTask(void *argument);

#endif
