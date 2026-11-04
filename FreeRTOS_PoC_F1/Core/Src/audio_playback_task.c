#include <stdint.h>
#include <stdbool.h>
#include "stm32f1xx_hal.h"
#include "speech_conv_defs.h"
#include "speech_conv_task.h"
#include "my_rtt.h"

extern DAC_HandleTypeDef hdac;
extern TIM_HandleTypeDef htim6;

TaskHandle_t audioTaskHandle;
static uint16_t audioDma[AUDIO_DMA_SAMPLES];	// buffer for passing the PCM stream to DAC via DMA
static uint8_t pcmChunk[AUDIO_HALF_SAMPLES];	// buffer for receiving PCM chunks from SAM via speech conversion task
static bool audioHalfHasData[2] = { false, false };

static bool FillDmaHalf(uint16_t *dst)
{
	size_t received = xStreamBufferReceive( pcmStream, pcmChunk, AUDIO_HALF_SAMPLES, 0);

	for (size_t i = 0; i < received; i++)
	{
		dst[i] = ((uint16_t)pcmChunk[i]) << 4;
	}

	// Fill the unused portion with DAC midpoint silence
	for (size_t i = received; i < AUDIO_HALF_SAMPLES; i++)
	{
		dst[i] = 2048U;
	}

	return (received > 0U);
}

void AudioPlaybackTask(void *argument)
{
	uint32_t events;
	AudioState_t state = AUDIO_WAITING;

	audioTaskHandle = xTaskGetCurrentTaskHandle();

	while(1)
	{
		switch (state)
		{
		case AUDIO_WAITING:
		{
			// Wait until enough PCM is available, or conversion has finished. Check for errors
			while ((xStreamBufferBytesAvailable(pcmStream) < AUDIO_START_THRESHOLD) && !speechConversionDone)
			{
			    if (xTaskNotifyWait(0, UINT32_MAX, &events, portMAX_DELAY) == pdTRUE)
			    {
			        if (events & AUDIO_EVT_ERROR)
			        {
			            state = AUDIO_ERROR;
			            break;
			        }
			    }
			}

			if (state == AUDIO_ERROR)
			{
			    break;
			}

			size_t available = xStreamBufferBytesAvailable(pcmStream);

			// Empty utterance: nothing to play
			if ((available == 0U) && speechConversionDone)
			{
				state = AUDIO_IDLE;
				break;
			}

			// Prepare both DMA halves
			audioHalfHasData[0] = FillDmaHalf(&audioDma[0]);

			audioHalfHasData[1] = FillDmaHalf(&audioDma[AUDIO_HALF_SAMPLES]);

			RTT_printf(0, "Starting audio DMA\r\n");

			if (HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, (uint32_t *)audioDma, AUDIO_DMA_SAMPLES, DAC_ALIGN_12B_R) != HAL_OK)
			{
				RTT_printf(0, "DAC DMA start failed\r\n");
				state = AUDIO_ERROR;
				break;
			}

			if (HAL_TIM_Base_Start(&htim6) != HAL_OK)
			{
				RTT_printf(0, "TIM6 start failed\r\n");

				HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);

				state = AUDIO_ERROR;
				break;
			}

			state = speechConversionDone ? AUDIO_DRAINING : AUDIO_PLAYING;
			RTT_printf(0, "Audio started\r\n");
			break;
		}

		case AUDIO_PLAYING:
		case AUDIO_DRAINING:
		{
			// Wait for DMA events or conversion-task notifications
			xTaskNotifyWait(0, UINT32_MAX, &events, portMAX_DELAY);

			if (events & AUDIO_EVT_ERROR)
			{
				state = AUDIO_ERROR;
				break;
			}

			if (events & AUDIO_EVT_STOP)
			{
				state = AUDIO_DRAINING;
			}

			// Both bits at once mean that the task has missed at least one DMA refill deadline
			// Do not guess which half is safe to modify
			if ((events & (AUDIO_EVT_HALF | AUDIO_EVT_FULL)) ==  (AUDIO_EVT_HALF | AUDIO_EVT_FULL))
			{
				RTT_printf(0, "Audio DMA deadline missed\r\n");
				state = AUDIO_ERROR;
				break;
			}

			if (events & AUDIO_EVT_HALF)
			{
				// First half has just finished playing. DMA is now consuming the second half
				if (speechConversionDone && (xStreamBufferBytesAvailable(pcmStream) == 0U) && !audioHalfHasData[1])
				{
					// No samples remain in the stream and the currently playing half contains silence.
					// All speech has drained.
					state = AUDIO_STOPPING;
					break;
				}

				audioHalfHasData[0] = FillDmaHalf(&audioDma[0]);
			}

			if (events & AUDIO_EVT_FULL)
			{
				// Second half has just finished playing. DMA is now consuming the first half
				if (speechConversionDone && (xStreamBufferBytesAvailable(pcmStream) == 0U) && !audioHalfHasData[0])
				{
					state = AUDIO_STOPPING;
					break;
				}

				audioHalfHasData[1] = FillDmaHalf(&audioDma[AUDIO_HALF_SAMPLES]);
			}

			// Conversion may finish while DMA is running. The STOP event will move us into AUDIO_DRAINING.
			if (speechConversionDone)
			{
				state = AUDIO_DRAINING;
			}

			break;
		}

		case AUDIO_STOPPING:
		{
			// Stop the sample-rate generator first
			HAL_TIM_Base_Stop(&htim6);

			// Then stop DAC DMA
			HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);

			audioHalfHasData[0] = false;
			audioHalfHasData[1] = false;

			RTT_printf(0, "Audio playback stopped\r\n");

			state = AUDIO_IDLE;
			break;
		}

		case AUDIO_IDLE:
		{
			// Wait for the next conversion request/data notification. The conversion task resets speechConversionDone
			// before beginning the next utterance
			xTaskNotifyWait(0, UINT32_MAX, &events, portMAX_DELAY);

			state = AUDIO_WAITING;
			break;
		}

		case AUDIO_ERROR:
		{
			HAL_TIM_Base_Stop(&htim6);
			HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);

			RTT_printf(0, "Audio playback error\r\n");

			vTaskDelay(pdMS_TO_TICKS(100));

			state = AUDIO_WAITING;
			break;
		}

		default:
			state = AUDIO_ERROR;
			break;
		}
	}
}

void HAL_DAC_ConvHalfCpltCallbackCh1(DAC_HandleTypeDef *hdac)
{
	BaseType_t higherPriorityTaskWoken = pdFALSE;

	xTaskNotifyFromISR(audioTaskHandle,	AUDIO_EVT_HALF,	eSetBits,	&higherPriorityTaskWoken);

	portYIELD_FROM_ISR(higherPriorityTaskWoken);
}

void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef *hdac)
{
	BaseType_t higherPriorityTaskWoken = pdFALSE;

	xTaskNotifyFromISR(audioTaskHandle,	AUDIO_EVT_FULL,	eSetBits,	&higherPriorityTaskWoken);

	portYIELD_FROM_ISR(higherPriorityTaskWoken);
}
