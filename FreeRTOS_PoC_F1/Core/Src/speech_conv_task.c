#include <stdbool.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "stream_buffer.h"
#include "speech_conv_defs.h"
#include "sam.h"
#include "tlv_common.h"

extern TaskHandle_t audioTaskHandle;

QueueHandle_t speechQueue;	// input queue for text to be pronounced
StreamBufferHandle_t pcmStream;
bool audioStarted;
volatile bool audioUnderrun;
volatile bool speechConversionDone = false;

void Speech_Init(void)
{
	speechQueue = xQueueCreate(1, sizeof(SpeechRequest_t));

	pcmStream = xStreamBufferCreate(PCM_STREAM_SIZE, 1U);

	configASSERT(speechQueue != NULL);
	configASSERT(pcmStream != NULL);
}

static bool Audio_SendPcm(const uint8_t *pcm, size_t length)
{
	size_t offset = 0;

	while (offset < length)
	{
		size_t sent = xStreamBufferSend(pcmStream, &pcm[offset], length - offset, portMAX_DELAY);

		if (sent == 0U)
		{
			return false;
		}

		offset += sent;

		xTaskNotify(audioTaskHandle, AUDIO_EVT_DATA, eSetBits);
	}

	return true;
}

void SpeechConversionTask(void *argument)
{
	SpeechRequest_t req;
	uint8_t pcm[SAM_CHUNK_SIZE];

	while(1)
	{
		if (xQueueReceive(speechQueue, &req, portMAX_DELAY) != pdPASS)
		{
			continue;
		}

		// Previous utterance must have been completely drained.
		configASSERT(xStreamBufferBytesAvailable(pcmStream) == 0U);

		// Start a new utterance.
		speechConversionDone = false;

		bool conversionOk = true;

		if (!SAM_Begin(req.text))
		{
			conversionOk = false;
		}
		else
		{
			while(1)
			{
				size_t produced = 0;

				SAM_Status status =	SAM_GenerateChunk(pcm, sizeof(pcm), &produced);

				if (status == SAM_STATUS_ERROR)
				{
					conversionOk = false;
					break;
				}

				if (produced > 0U)
				{
					if (!Audio_SendPcm(pcm, produced))
					{
						conversionOk = false;
						break;
					}
				}

				if (status == SAM_STATUS_DONE)
				{
					break;
				}
			}
		}

		if (conversionOk)
		{
			// All PCM samples have been sent to the stream buffer.
			speechConversionDone = true;

			// Playback must drain the remaining PCM data.
			xTaskNotify(audioTaskHandle, AUDIO_EVT_STOP, eSetBits);
		}
		else
		{
			// Conversion failed. Playback must handle the error.
			xTaskNotify(audioTaskHandle, AUDIO_EVT_ERROR, eSetBits);
		}
	}
}

TLV_STATUS Speech_Say(const char *text, uint16_t requestId)
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
