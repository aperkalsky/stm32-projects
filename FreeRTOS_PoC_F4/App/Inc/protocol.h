#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

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

#define TLV_MAX_RX_PAYLOAD_SIZE 128
#define TLV_MAX_TX_PAYLOAD_SIZE 128

// buffers to send/receive data from/to USB stack
// ----------------------------------------------

// here we collect incoming TLV bytes of Rx packet
#define RX_RAW_PACKET_BUF_SIZE (TLV_RX_HEADER_SIZE + TLV_MAX_RX_PAYLOAD_SIZE + TLV_RX_FOOTER_SIZE)

// here we prepare formatted TLV response
#define TX_RAW_PACKET_BUF_SIZE (TLV_TX_HEADER_SIZE + TLV_MAX_TX_PAYLOAD_SIZE + TLV_TX_FOOTER_SIZE)

// TODO: find smart way to adjust it to the *RAW_PACKET_BUF_SIZE
#define CRC_CALC_BUF_SIZE 128       // in 4-byte words
//#define MAX_PAYLOAD     256
//#define TLV_CRC_SIZE   4
#define USB_TX_BUSY_WAIT_TIMEOUT_MS  100

// TLV commands
// ============
#define CMD_GET_FW_VERSION     0x01
#define CMD_GET_FLASH_ID		   0x02

// TLV response statuses
#define TLV_STAT_OK         0x0000
#define TLV_STAT_TIMEOUT    0x0001
#define TLV_STAT_NOT_IMPLEMENTED	0x0002

// Data structures

#pragma pack(push, 1)

typedef struct{
	uint8_t major;
	uint8_t minor;
} GET_FW_VERSION_OUT;

typedef struct{
	uint32_t id;
} GET_FLASH_ID_OUT;

#pragma pack(pop)

void Protocol_Process(void);

#endif
