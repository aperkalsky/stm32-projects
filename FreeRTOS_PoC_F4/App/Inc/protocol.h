#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include "flash.h"
#include "tlv_common.h"

#define RX_RING_SIZE           1024 // ring buffer for collecting Rx packets


// buffers to send/receive data from/to USB stack
// ----------------------------------------------

// here we collect incoming TLV bytes of Rx packet
#define RX_RAW_PACKET_BUF_SIZE (TLV_RX_HEADER_SIZE + TLV_MAX_RX_PAYLOAD_SIZE + TLV_RX_FOOTER_SIZE)

// here we prepare formatted TLV response
#define TX_RAW_PACKET_BUF_SIZE (TLV_TX_HEADER_SIZE + TLV_MAX_TX_PAYLOAD_SIZE + TLV_TX_FOOTER_SIZE)

// The buffer size for CRC calculation is in 4-byte words. We need to relate its size to the size
// of TX buffer payload (it's longer than Rx) plus header size. Add extra word to compensate the
// division remainder
#define CRC_CALC_BUF_SIZE (((TLV_MAX_TX_PAYLOAD_SIZE + TLV_TX_HEADER_SIZE) / 4) + 1)
#define USB_TX_BUSY_WAIT_TIMEOUT_MS  100

// Data structures

#pragma pack(push, 1)

typedef struct{
	uint8_t major;
	uint8_t minor;
} GET_FW_VERSION_OUT;

typedef struct{
	uint32_t id;
} GET_FLASH_ID_OUT;

typedef struct{
	uint32_t address;
	uint16_t size;	// because we can read up to 256 bytes (0x100)
} READ_FLASH_IN;

typedef struct{
	uint8_t data[FLASH_PAGE_SIZE];
} READ_FLASH_OUT;

typedef struct{
	uint32_t address;
	uint8_t size;
	uint8_t data[FLASH_PAGE_SIZE];
} WRITE_FLASH_IN;

#pragma pack(pop)

// buffer sizes depend on the commands
#define TLV_MAX_RX_PAYLOAD_SIZE (sizeof(WRITE_FLASH_IN))	// fit Flash page buffer size
#define TLV_MAX_TX_PAYLOAD_SIZE (sizeof(READ_FLASH_OUT))

// exported functions
void Protocol_Process(void);

#endif
