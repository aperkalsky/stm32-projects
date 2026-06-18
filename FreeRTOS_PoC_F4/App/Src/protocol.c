#include "protocol.h"
#include "usbd_cdc_if.h"
//#include "crc.h"
#include "cmsis_os.h"
#include "main.h"
#include "SEGGER_RTT.h"
#include <string.h>

#define RX_RING_SIZE    4096
#define MAX_PAYLOAD     256

extern CRC_HandleTypeDef hcrc;

extern volatile uint32_t gUsbRxHead;
extern volatile uint32_t gUsbRxTail;
extern uint8_t gUsbRxRing[];

typedef enum
{
	RX_TYPE,
	RX_LEN_LO,
	RX_LEN_HI,
	RX_SEQ_LO,
	RX_SEQ_HI,
	RX_PAYLOAD,
	RX_CRC
} RxState_t;

static RxState_t rxState = RX_TYPE;

static uint8_t packetType;
static uint16_t packetLength;
static uint16_t packetSeq;

static uint8_t payload[MAX_PAYLOAD];

static uint32_t rxPayloadIndex;
static uint32_t rxCrcIndex;

static uint8_t crcBytes[4];

static uint8_t packetRaw[512];
static uint32_t packetRawIndex;

static uint32_t CalcCrc32(
		const uint8_t *data,
		uint32_t len)
{
	uint32_t words[128];
	uint32_t wordCount;
//	uint32_t i;

	memset(words, 0, sizeof(words));

	memcpy(words, data, len);

	wordCount = (len + 3) / 4;

	__HAL_CRC_DR_RESET(&hcrc);

	return HAL_CRC_Calculate(
			&hcrc,
			words,
			wordCount);
}

static void SendResponse(
		uint8_t type,
		uint16_t seq,
		const uint8_t *payload,
		uint16_t payloadLen)
{
	uint8_t tx[512];

	uint32_t pos = 0;

	tx[pos++] = type;

	tx[pos++] = payloadLen & 0xff;
	tx[pos++] = payloadLen >> 8;

	tx[pos++] = seq & 0xff;
	tx[pos++] = seq >> 8;

	memcpy(&tx[pos], payload, payloadLen);
	pos += payloadLen;

	uint32_t crc = CalcCrc32(tx, pos);

	memcpy(&tx[pos], &crc, sizeof(crc));
	pos += 4;

	while(CDC_Transmit_FS(tx, pos) == USBD_BUSY)
	{
		osDelay(1);
	}

	SEGGER_RTT_printf(0, "Xmitted %d bytes\r\n", pos);
}

static void HandlePacket(
		uint8_t type,
		uint16_t seq,
		uint8_t *payload,
		uint16_t len)
{
	SEGGER_RTT_printf(0, "type = %d\r\n");

	switch(type)
	{
	case CMD_GET_VERSION:
	{
		const char ver[] = "1.0";

		SendResponse(
				RESP_VERSION,
				seq,
				(uint8_t*)ver,
				sizeof(ver));

		break;
	}

	/*	case CMD_LED_ON:
	{
		HAL_GPIO_WritePin(
				LD2_GPIO_Port,
				LD2_Pin,
				GPIO_PIN_SET);

		SendResponse(
				RESP_OK,
				seq,
				NULL,
				0);

		break;
	}

	case CMD_LED_OFF:
	{
		HAL_GPIO_WritePin(
				LD2_GPIO_Port,
				LD2_Pin,
				GPIO_PIN_RESET);

		SendResponse(
				RESP_OK,
				seq,
				NULL,
				0);

		break;
	} */

	default:
	{
		SendResponse(
				RESP_ERROR,
				seq,
				NULL,
				0);

		break;
	}
	}
}

void Protocol_Process(void)
{
	while(gUsbRxTail != gUsbRxHead)
	{
		uint8_t byte =
				gUsbRxRing[gUsbRxTail];

		SEGGER_RTT_printf(0, "Byte %02X, h=%d t=%d s=%d\r\n", byte, gUsbRxHead, gUsbRxTail, rxState);

		gUsbRxTail =
				(gUsbRxTail + 1) %
				RX_RING_SIZE;

		packetRaw[packetRawIndex++] = byte;

		switch(rxState)
		{
		case RX_TYPE:
			packetType = byte;
			packetLength = 0;
			rxState = RX_LEN_LO;
			break;

		case RX_LEN_LO:
			packetLength = byte;
			rxState = RX_LEN_HI;
			break;

		case RX_LEN_HI:
			packetLength |=
					((uint16_t)byte << 8);

			rxState = RX_SEQ_LO;
			break;

		case RX_SEQ_LO:
			packetSeq = byte;
			rxState = RX_SEQ_HI;
			break;

		case RX_SEQ_HI:
			packetSeq |=
					((uint16_t)byte << 8);

			rxPayloadIndex = 0;

			if(packetLength == 0)
				rxState = RX_CRC;
			else
				rxState = RX_PAYLOAD;

			break;

		case RX_PAYLOAD:
			payload[rxPayloadIndex++] = byte;

			if(rxPayloadIndex >= packetLength)
			{
				rxCrcIndex = 0;
				rxState = RX_CRC;
			}

			break;

		case RX_CRC:
			crcBytes[rxCrcIndex++] = byte;

			if(rxCrcIndex == 4)
			{
				uint32_t rxCrc;
				uint32_t calcCrc;

				memcpy(
						&rxCrc,
						crcBytes,
						4);

				calcCrc =
						CalcCrc32(
								packetRaw,
								packetRawIndex - 4);

				SEGGER_RTT_printf(0, "rxCrc=%08X, calcCrc=%08X\r\n",rxCrc, calcCrc);

				if(rxCrc == calcCrc)
				{
					HandlePacket(
							packetType,
							packetSeq,
							payload,
							packetLength);
				}

				rxState = RX_TYPE;
				packetRawIndex = 0;
			}

			break;
		}
	}
}


