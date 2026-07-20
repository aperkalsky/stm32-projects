#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#include <stdint.h>
#include "tlv_common.h"


#pragma pack(push, 1)

typedef struct
{
  uint8_t major;
  uint8_t minor;
} GET_FW_VERSION_OUT;

typedef struct
{
	uint8_t mode;
	uint16_t param;
}PWM_LED_CTL_IN;

typedef struct{
	int32_t temperature;
} GET_TEMPERATURE_OUT;


#pragma pack(pop)

#define TLV_MAX_RX_PAYLOAD_SIZE 64
#define TLV_MAX_TX_PAYLOAD_SIZE sizeof(GET_FW_VERSION_OUT)
#define RX_RAW_PACKET_BUF_SIZE (TLV_RX_HEADER_SIZE + TLV_MAX_RX_PAYLOAD_SIZE + TLV_RX_FOOTER_SIZE)
#define TX_RAW_PACKET_BUF_SIZE (TLV_TX_HEADER_SIZE + TLV_MAX_TX_PAYLOAD_SIZE + TLV_TX_FOOTER_SIZE)

// The buffer size for CRC calculation is in 4-byte words. We need to relate its size to the size
// of TX buffer payload (it's longer than Rx) plus header size. Add extra word to compensate the
// division remainder
#define CRC_CALC_BUF_SIZE (((TLV_MAX_TX_PAYLOAD_SIZE + TLV_TX_HEADER_SIZE) / 4) + 1)


void Protocol_Process(void);

#endif
