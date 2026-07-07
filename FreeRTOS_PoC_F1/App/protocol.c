#include "protocol.h"
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

static uint32_t CalcCrc32(const uint8_t *data, uint32_t len)
{
	uint32_t crc = 0xFFFFFFFFu;

	for (uint32_t offset = 0; offset < len; offset += 4)
	{
		uint32_t word = 0;

		for (uint32_t i = 0; i < 4; ++i)
		{
			if ((offset + i) < len)
			{
				word |= ((uint32_t)data[offset + i]) << (8u * i);
			}
		}

		crc ^= word;

		for (uint32_t bit = 0; bit < 32; ++bit)
		{
			if (crc & 0x80000000u)
			{
				crc = ((crc << 1) ^ 0x04C11DB7u) & 0xFFFFFFFFu;
			}
			else
			{
				crc = (crc << 1) & 0xFFFFFFFFu;
			}
		}
	}

	return crc;
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

	SEGGER_RTT_printf(0, "TX TLV len=%lu\r\n", (unsigned long)pos);
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

		if (packetRawIndex < sizeof(packetRaw))
		{
			packetRaw[packetRawIndex++] = byte;
		}

		switch(rxState)
		{
		case RX_TYPE:
			if(byte != 0) // work around the problem with Prolific USB to Serial that sends 0x00 upon USB connect
			{
				packetType = byte;
				packetLength = 0;
				rxState = RX_LEN_LO;
			}
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
			rxState = (packetLength == 0) ? RX_CRC : RX_PAYLOAD;
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

				rxState = RX_TYPE;
				packetRawIndex = 0;
			}
			break;
		}
	}
}
