#include "protocol.h"
#include "usbd_cdc_if.h"
#include "cmsis_os.h"
#include "main.h"
#include "SEGGER_RTT.h"
#include <string.h>
#include "flash.h"

extern CRC_HandleTypeDef hcrc;
extern USBD_HandleTypeDef hUsbDeviceFS;

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

static uint8_t rxPayload[TLV_MAX_RX_PAYLOAD_SIZE];
static uint8_t txPayload[TLV_MAX_TX_PAYLOAD_SIZE];	// buffer for laying out the response

static uint32_t rxPayloadIndex;
static uint32_t rxCrcIndex;

static uint8_t crcBytes[TLV_CRC_SIZE];

static uint8_t packetRaw[RX_RAW_PACKET_BUF_SIZE];
static uint32_t packetRawIndex;

static uint8_t tx[TX_RAW_PACKET_BUF_SIZE];	// TX buffer for SendResponse()

static uint32_t words[CRC_CALC_BUF_SIZE]; // buffer for crc32 calculation

static uint32_t CalcCrc32(
		const uint8_t *data,
		uint32_t len)
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

static uint8_t UsbTransmit(
		uint8_t *buf,
		uint16_t len)
{
	USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData;

	uint32_t start = osKernelGetTickCount();

	while (hcdc->TxState)
	{
		if ((osKernelGetTickCount() - start) > USB_TX_BUSY_WAIT_TIMEOUT_MS)
			return USBD_BUSY;

		osDelay(1);
	}

	return CDC_Transmit_FS(buf, len) == USBD_OK;
}

static uint8_t SendResponse(
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

	memcpy(&tx[pos], payload, payloadLen);
	pos += payloadLen;

	uint32_t crc = CalcCrc32(tx, pos);

	memcpy(&tx[pos], &crc, sizeof(crc));
	pos += TLV_CRC_SIZE;

//	SEGGER_RTT_printf(0, "Xmitting %d bytes\r\n", pos);

	return UsbTransmit(tx, pos);
}

// Message handlers start
// ----------------------

void OnCmdGetFwVersion(uint16_t seq)
{
	GET_FW_VERSION_OUT* pOut = (GET_FW_VERSION_OUT*)txPayload;
	pOut->major = 1;
	pOut->minor = 0;

	SendResponse(
			CMD_GET_FW_VERSION,
			seq,
			TLV_STAT_OK,
			txPayload,
			sizeof(GET_FW_VERSION_OUT));
}

void OnCmdGetFlashID(uint16_t seq)
{
	GET_FLASH_ID_OUT* pOut = (GET_FLASH_ID_OUT*)txPayload;

	FlashReset();
	pOut->id = FlashReadID();

	SEGGER_RTT_printf(0, "Flash ID = %08X\r\n", pOut->id);

	SendResponse(
			CMD_GET_FLASH_ID,
			seq,
			TLV_STAT_OK,
			txPayload,
			sizeof(GET_FLASH_ID_OUT));
}

void OnCmdReadFlash(uint16_t seq, uint8_t* payload)
{
	READ_FLASH_IN* pIn = (READ_FLASH_IN*)payload;
	FlashStatus_t status = FLASH_OK;

//	SEGGER_RTT_printf(0, "Flash read: addr = %08X, len = %d\r\n", pIn->address, pIn->size);

	// input validation
	if((pIn->size == 0) || (pIn->size > FLASH_PAGE_SIZE) || ((pIn->address + pIn->size) > FLASH_SIZE))
	{
		SendResponse(
				CMD_READ_FLASH,
				seq,
				TLV_STAT_INVALID_ARGUMENT,
				NULL,
				0);
	}

	// try to read data
	status = FlashRead(pIn->address, (void*)txPayload, pIn->size);
//	FlashTestRead(pIn->address, pIn->size, txPayload);


	if(status == FLASH_OK)
	{
		SendResponse(
				CMD_READ_FLASH,
				seq,
				TLV_STAT_OK,
				txPayload,
				sizeof(READ_FLASH_OUT));
	}
	else
	{
		SEGGER_RTT_printf(0, "Flash read failed. Status = %d\r\n", status);

		SendResponse(
				CMD_READ_FLASH,
				seq,
				TLV_STAT_FAILURE,
				NULL,
				0);
	}
}

void OnCmdTest1(uint16_t seq)
{
	FlashStatus_t ret = FlashChipErase();

	SEGGER_RTT_printf(0, "Erase status = %d\r\n", ret);

	SendResponse(
			CMD_TEST_1,
			seq,
			TLV_STAT_OK,
			NULL,
			0);
}

void OnUnknownCommand(uint8_t type, uint16_t seq)
{
	SendResponse(
			type,
			seq,
			TLV_STAT_NOT_IMPLEMENTED,
			NULL,
			0);
}

// Message handlers end
// --------------------

static void HandlePacket(
		uint8_t type,
		uint16_t seq,
		uint8_t *payload,
		uint16_t len)
{
//	SEGGER_RTT_printf(0, "type = %d\r\n");

	switch(type)
	{
	case CMD_GET_FW_VERSION:
		OnCmdGetFwVersion(seq);
		break;

	case CMD_GET_FLASH_ID:
		OnCmdGetFlashID(seq);
		break;

	case CMD_READ_FLASH:
		OnCmdReadFlash(seq, payload);
		break;

	case CMD_TEST_1:
		OnCmdTest1(seq);
		break;

	default:
		OnUnknownCommand(type, seq);
		break;
	}
}

void Protocol_Process(void)
{
	while(gUsbRxTail != gUsbRxHead)
	{
		uint8_t byte =
				gUsbRxRing[gUsbRxTail];

//		SEGGER_RTT_printf(0, "Byte %02X, h=%d t=%d s=%d\r\n", byte, gUsbRxHead, gUsbRxTail, rxState);

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
			rxPayload[rxPayloadIndex++] = byte;

			if(rxPayloadIndex >= packetLength)
			{
				rxCrcIndex = 0;
				rxState = RX_CRC;
			}

			break;

		case RX_CRC:
			crcBytes[rxCrcIndex++] = byte;

			if(rxCrcIndex == TLV_CRC_SIZE)
			{
				uint32_t rxCrc;
				uint32_t calcCrc;

				memcpy(
						&rxCrc,
						crcBytes,
						TLV_CRC_SIZE);

				calcCrc =
						CalcCrc32(
								packetRaw,
								packetRawIndex - TLV_CRC_SIZE);

//				SEGGER_RTT_printf(0, "rxCrc=%08X, calcCrc=%08X\r\n",rxCrc, calcCrc);

				if(rxCrc == calcCrc)
				{
					HandlePacket(
							packetType,
							packetSeq,
							rxPayload,
							packetLength);
				}

				rxState = RX_TYPE;
				packetRawIndex = 0;
			}

			break;
		}
	}
}


