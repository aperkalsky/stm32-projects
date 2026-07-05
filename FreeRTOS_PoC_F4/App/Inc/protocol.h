#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include "flash.h"

/*
						TLV Protocol Definitions
						========================


           Structure of Rx packets
           -----------------------

			+------+--------+-----+----------+--------+
			| Type | Length | Seq | Payload  | CRC32  |
			+------+--------+-----+----------+--------+
			 1 B      2 B     2 B    N B       4 B

	 Type: command type (0 - 0xFF)
	 Length: payload length. Up to RAW_PACKET_BUF_SIZE - 9
	 Seq: sequence number (0 - 0xFFFF)
	 Payload: data sent (*see Length)
	 CRC32: checksum, according to STM32 hardware implementation

           Structure of Tx packets
           -----------------------

			+------+--------+-----+--------+----------+--------+
			| Type | Length | Seq | Status | Payload  | CRC32  |
			+------+--------+-----+--------+----------+--------+
			 1 B      2 B     2 B    2 B       N B       4 B

	 Type: command type (0 - 0xFF)
	 Length: payload length. Up to RAW_PACKET_BUF_SIZE - 11
	 Seq: sequence number (0 - 0xFFFF)
	 Status: status of command execution
	 Payload: data sent (*see Length)
	 CRC32: checksum, according to STM32 hardware implementation
 *
 */

#define RX_RING_SIZE           1024 // ring buffer for collecting Rx packets

#define TLV_TYPE_SIZE    1
#define TLV_LENGTH_SIZE  2
#define TLV_SEQ_SIZE     2
#define TLV_STATUS_SIZE  2
#define TLV_CRC_SIZE     4

#define TLV_RX_HEADER_SIZE (TLV_TYPE_SIZE + TLV_LENGTH_SIZE + TLV_SEQ_SIZE)
#define TLV_RX_FOOTER_SIZE TLV_CRC_SIZE
#define TLV_TX_HEADER_SIZE (TLV_TYPE_SIZE + TLV_LENGTH_SIZE + TLV_SEQ_SIZE + TLV_STATUS_SIZE)
#define TLV_TX_FOOTER_SIZE TLV_CRC_SIZE

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

// TLV commands
// ============
#define CMD_GET_FW_VERSION     0x01
#define CMD_GET_FLASH_ID		   0x02
#define CMD_READ_FLASH         0x03
#define CMD_WRITE_FLASH        0x04

// TLV response statuses
#define TLV_STAT_OK         0x0000
#define TLV_STAT_TIMEOUT    0x0001
#define TLV_STAT_NOT_IMPLEMENTED	0x0002
#define TLV_STAT_INVALID_ARGUMENT 0X0003
#define TLV_STAT_FAILURE		0x0004

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
