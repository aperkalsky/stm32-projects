#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "stream_buffer.h"
#include <stdbool.h>
#include <string.h>
#include "speech_conv_defs.h"
#include "sam.h"
#include "tlv_common.h"

extern TaskHandle_t audioTaskHandle;

uint16_t audioDma[AUDIO_DMA_SAMPLES];
QueueHandle_t speechQueue;
StreamBufferHandle_t pcmStream;
bool audioStarted;
volatile bool audioUnderrun;

void Speech_Init(void)
{
    speechQueue = xQueueCreate(1, sizeof(SpeechRequest_t));

    pcmStream = xStreamBufferCreate(
        PCM_STREAM_SIZE,
        1U);   /* Minimum trigger level */

    configASSERT(speechQueue != NULL);
    configASSERT(pcmStream != NULL);
}

void SpeechConversionTask(void *argument)
{
    SpeechRequest_t req;
    uint8_t pcm[SAM_CHUNK_SIZE];

    for (;;)
    {
        if (xQueueReceive(speechQueue, &req, portMAX_DELAY) != pdPASS)
            continue;

        /*
         * This example assumes one utterance at a time.
         * Do not reset pcmStream while the previous utterance
         * is still playing.
         */

        if (!SAM_Begin(req.text))
        {
            /* Report conversion failure to protocol/application. */
            continue;
        }

        for (;;)
        {
            size_t produced = 0;

            SAM_Status status = SAM_GenerateChunk(
                pcm,
                sizeof(pcm),
                &produced);

            if (status == SAM_STATUS_ERROR)
            {
                /* Report conversion failure. */
                break;
            }

            if (produced > 0U)
            {
                size_t offset = 0;

                while (offset < produced)
                {
                    size_t sent = xStreamBufferSend(
                        pcmStream,
                        &pcm[offset],
                        produced - offset,
                        portMAX_DELAY);

                    offset += sent;
                }

                /* Wake the playback task if it is waiting to start. */
                xTaskNotify(
                    audioTaskHandle,
                    AUDIO_EVT_DATA,
                    eSetBits);
            }

            if (status == SAM_STATUS_DONE)
                break;
        }

        /*
         * Signal end-of-utterance here using a task notification,
         * event group, or explicit stream protocol.
         * Do not accept the next request until the previous
         * utterance has drained, unless you implement cancellation.
         */
    }
}

TLV_STATUS SPEECH_Say(const char *text, uint16_t requestId)
{
  SpeechRequest_t req = {0};

  if (text == NULL)
  {
    return TLV_STAT_INVALID_ARGUMENT;
  }

  size_t len = strnlen(text, SPEECH_TEXT_MAX + 1U);

  if (len > SPEECH_TEXT_MAX)
  {
    return TLV_STAT_INVALID_ARGUMENT;
  }

  memcpy(req.text, text, len);
  req.text[len] = '\0';
  req.request_id = requestId;

  if(xQueueSend(speechQueue, &req, 0) == pdPASS)
  {
  	return TLV_STAT_OK;
  }
  else
  {
  	return TLV_STAT_NO_ROOM;
  }
}
