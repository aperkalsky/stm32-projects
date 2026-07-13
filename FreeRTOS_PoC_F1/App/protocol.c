#include "protocol.h"
#include "cmsis_os.h"
#include "main.h"
#include "uart_driver.h"
#include "SEGGER_RTT.h"
#include <string.h>

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

extern CRC_HandleTypeDef hcrc;

static RxState_t rxState = RX_TYPE;

static uint8_t packetType;
static uint16_t packetLength;
static uint16_t packetSeq;

static uint8_t rxPayload[TLV_MAX_RX_PAYLOAD_SIZE];
static uint8_t txPayload[TLV_MAX_TX_PAYLOAD_SIZE];

static uint32_t rxPayloadIndex;
static uint32_t rxCrcIndex;

static uint8_t crcBytes[TLV_CRC_SIZE];
static uint8_t packetRaw[RX_RAW_PACKET_BUF_SIZE];
static uint32_t packetRawIndex;
static uint8_t tx[TX_RAW_PACKET_BUF_SIZE];

static uint32_t words[CRC_CALC_BUF_SIZE]; // buffer for crc32 calculation


static void ResetRxState(void)
{
	rxState = RX_TYPE;
	packetLength = 0;
	packetSeq = 0;
	rxPayloadIndex = 0;
	rxCrcIndex = 0;
	packetRawIndex = 0;
}

static uint32_t CalcCrc32(const uint8_t *data, uint32_t len)
{
	uint32_t wordCount;

	memset(words, 0, sizeof(words));

	memcpy(words, data, len);

	wordCount = (len + 3) / 4;

	__HAL_CRC_DR_RESET(&hcrc);

	return HAL_CRC_Calculate(
			&hcrc,
			words,
			wordCount);
}

static void UartTransmit(uint8_t *buf, uint16_t len)
{
	UartDriver_SendBuffer(buf, len);
}

static void SendResponse(
		uint8_t type,
		uint16_t seq,
		uint16_t status,
		uint8_t *payload,
		uint16_t payloadLen)
{
	uint32_t pos = 0;

	tx[pos++] = type;
	tx[pos++] = payloadLen & 0xff;
	tx[pos++] = payloadLen >> 8;
	tx[pos++] = seq & 0xff;
	tx[pos++] = seq >> 8;
	tx[pos++] = status & 0xff;
	tx[pos++] = status >> 8;

	if ((payload != NULL) && (payloadLen != 0))
	{
		memcpy(&tx[pos], payload, payloadLen);
		pos += payloadLen;
	}

	uint32_t crc = CalcCrc32(tx, pos);
	memcpy(&tx[pos], &crc, sizeof(crc));
	pos += TLV_CRC_SIZE;

	UartTransmit(tx, pos);
}

static void OnCmdGetFwVersion(uint16_t seq)
{
	GET_FW_VERSION_OUT *pOut = (GET_FW_VERSION_OUT *)txPayload;
	pOut->major = 1;
	pOut->minor = 0;

	SendResponse(
			CMD_GET_FW_VERSION,
			seq,
			TLV_STAT_OK,
			txPayload,
			sizeof(GET_FW_VERSION_OUT));
}

static void OnUnknownCommand(uint8_t type, uint16_t seq)
{
	SendResponse(type, seq, TLV_STAT_NOT_IMPLEMENTED, NULL, 0);
}

static void HandlePacket(uint8_t type, uint16_t seq, uint8_t *payload, uint16_t len)
{
	(void)len;

	switch(type)
	{
	case CMD_GET_FW_VERSION:
		OnCmdGetFwVersion(seq);
		break;

	default:
		OnUnknownCommand(type, seq);
		break;
	}
}

void Protocol_Process(void)
{
	uint8_t byte;

	while (UartDriver_GetByte(&byte))
	{
//		SEGGER_RTT_printf(0, "byte=%02X state=%d idx=%d\r\n", byte, rxState, packetRawIndex);

		// work around the problem with Prolific USB to Serial that sends 0x00 upon USB connect
		// rewind the raw packet index, dropping this byte
		if((rxState == RX_TYPE) && (byte == 0))
		{
			continue;
		}

		if (packetRawIndex < sizeof(packetRaw))
		{
			packetRaw[packetRawIndex++] = byte;
		}
		else
		{
			ResetRxState();
			continue;
		}

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
			packetLength |= ((uint16_t)byte << 8);
			rxState = RX_SEQ_LO;
			break;

		case RX_SEQ_LO:
			packetSeq = byte;
			rxState = RX_SEQ_HI;
			break;

		case RX_SEQ_HI:
			packetSeq |= ((uint16_t)byte << 8);
			rxPayloadIndex = 0;

			if(packetLength == 0)
			{
				rxCrcIndex = 0;
				rxState = RX_CRC;
			}
			else
			{
				rxState = RX_PAYLOAD;
			}

			break;

		case RX_PAYLOAD:
			if (rxPayloadIndex < sizeof(rxPayload))
			{
				rxPayload[rxPayloadIndex++] = byte;
			}

			if (rxPayloadIndex >= packetLength)
			{
				rxCrcIndex = 0;
				rxState = RX_CRC;
			}
			break;

		case RX_CRC:
			crcBytes[rxCrcIndex++] = byte;

			if (rxCrcIndex == TLV_CRC_SIZE)
			{
				uint32_t rxCrc;
				uint32_t calcCrc;

				memcpy(&rxCrc, crcBytes, TLV_CRC_SIZE);
				calcCrc = CalcCrc32(packetRaw, packetRawIndex - TLV_CRC_SIZE);

				if (rxCrc == calcCrc)
				{
					HandlePacket(packetType, packetSeq, rxPayload, packetLength);
				}
				else
				{
					ResetRxState();
				}

				rxState = RX_TYPE;
				packetRawIndex = 0;
			}
			break;
		}
	}
}
