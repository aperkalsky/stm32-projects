#include <stdint.h>
#include <stdbool.h>
#include "stm32f1xx_hal.h"
#include "speech_conv_defs.h"
#include "speech_conv_task.h"

extern DAC_HandleTypeDef hdac;

TaskHandle_t audioTaskHandle;

static void FillDmaHalf(uint16_t *dst)
{
	uint8_t pcm[AUDIO_HALF_SAMPLES];

	size_t received = xStreamBufferReceive(
			pcmStream,
			pcm,
			sizeof(pcm),
			0);

	for (size_t i = 0; i < AUDIO_HALF_SAMPLES; ++i)
	{
		if (i < received)
		{
			/* Example conversion for unsigned 8-bit PCM. */
			dst[i] = (uint16_t)pcm[i] << 4;
		}
		else
		{
			/* Midpoint voltage: digital silence. */
			dst[i] = 2048U;
			audioUnderrun = true;
		}
	}
}

void AudioPlaybackTask(void *argument)
{
	uint32_t events;

	audioTaskHandle = xTaskGetCurrentTaskHandle();

	for (;;)
	{
		if (!audioStarted)
		{
			/* Wait for enough PCM to fill both DMA halves. */
			while (xStreamBufferBytesAvailable(pcmStream)
					< AUDIO_START_THRESHOLD)
			{
				xTaskNotifyWait(
						0,
						UINT32_MAX,
						&events,
						portMAX_DELAY);
			}

			FillDmaHalf(&audioDma[0]);
			FillDmaHalf(&audioDma[AUDIO_HALF_SAMPLES]);

			if (HAL_DAC_Start_DMA(
					&hdac,
					DAC_CHANNEL_1,
					(uint32_t *)audioDma,
					AUDIO_DMA_SAMPLES,
					DAC_ALIGN_12B_R) != HAL_OK)
			{
				/* Handle hardware-start failure. */
				continue;
			}

			audioStarted = true;
		}

		xTaskNotifyWait(
				0,
				UINT32_MAX,
				&events,
				portMAX_DELAY);

		if (events & AUDIO_EVT_HALF)
		{
			/* DMA is now consuming the second half. */
			FillDmaHalf(&audioDma[0]);
		}

		if (events & AUDIO_EVT_FULL)
		{
			/* DMA has looped; it is now consuming the first half. */
			FillDmaHalf(&audioDma[AUDIO_HALF_SAMPLES]);
		}

		if (events & AUDIO_EVT_STOP)
		{
			/* Stop only when the final audio samples have drained. */
		}
	}
}

void HAL_DAC_ConvHalfCpltCallbackCh1(DAC_HandleTypeDef *hdac)
{
	BaseType_t higherPriorityTaskWoken = pdFALSE;

	xTaskNotifyFromISR(
			audioTaskHandle,
			AUDIO_EVT_HALF,
			eSetBits,
			&higherPriorityTaskWoken);

	portYIELD_FROM_ISR(higherPriorityTaskWoken);
}

void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef *hdac)
{
	BaseType_t higherPriorityTaskWoken = pdFALSE;

	xTaskNotifyFromISR(
			audioTaskHandle,
			AUDIO_EVT_FULL,
			eSetBits,
			&higherPriorityTaskWoken);

	portYIELD_FROM_ISR(higherPriorityTaskWoken);
}
