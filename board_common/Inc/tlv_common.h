#ifndef _TLV_H_
#define _TLV_H_

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

#define TLV_TYPE_SIZE    1
#define TLV_LENGTH_SIZE  2
#define TLV_SEQ_SIZE     2
#define TLV_STATUS_SIZE  2
#define TLV_CRC_SIZE     4

#define TLV_RX_HEADER_SIZE (TLV_TYPE_SIZE + TLV_LENGTH_SIZE + TLV_SEQ_SIZE)
#define TLV_RX_FOOTER_SIZE TLV_CRC_SIZE
#define TLV_TX_HEADER_SIZE (TLV_TYPE_SIZE + TLV_LENGTH_SIZE + TLV_SEQ_SIZE + TLV_STATUS_SIZE)
#define TLV_TX_FOOTER_SIZE TLV_CRC_SIZE

// TLV commands
// ============
#define CMD_GET_FW_VERSION     0x01
#define CMD_GET_FLASH_ID       0x02
#define CMD_READ_FLASH         0x03
#define CMD_WRITE_FLASH        0x04
#define CMD_TEST_1             0x05
#define CMD_TEST_2             0x06
#define CMD_TEST_3             0x07
#define CMD_TEST_4             0x08
#define CMD_TEST_5             0x09
#define CMD_PWM_LED_CTL        0x0A
#define CMD_GET_TEMPERATURE    0x0B


// TLV response statuses
#define TLV_STAT_OK         			0x0000
#define TLV_STAT_TIMEOUT    			0x0001
#define TLV_STAT_NOT_IMPLEMENTED	0x0002
#define TLV_STAT_INVALID_ARGUMENT 0X0003
#define TLV_STAT_FAILURE					0x0004

typedef uint16_t TLV_STATUS;



#endif
