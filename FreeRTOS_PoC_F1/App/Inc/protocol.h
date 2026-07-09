#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#include <stdint.h>

#define TLV_TYPE_SIZE    1
#define TLV_LENGTH_SIZE  2
#define TLV_SEQ_SIZE     2
#define TLV_STATUS_SIZE  2
#define TLV_CRC_SIZE     4

#define TLV_RX_HEADER_SIZE (TLV_TYPE_SIZE + TLV_LENGTH_SIZE + TLV_SEQ_SIZE)
#define TLV_RX_FOOTER_SIZE TLV_CRC_SIZE
#define TLV_TX_HEADER_SIZE (TLV_TYPE_SIZE + TLV_LENGTH_SIZE + TLV_SEQ_SIZE + TLV_STATUS_SIZE)
#define TLV_TX_FOOTER_SIZE TLV_CRC_SIZE

#define CMD_GET_FW_VERSION    0x01
#define CMD_GET_FLASH_ID      0x02
#define CMD_READ_FLASH        0x03
#define CMD_WRITE_FLASH       0x04
#define CMD_TEST_1            0x05

#define TLV_STAT_OK                0x0000
#define TLV_STAT_TIMEOUT           0x0001
#define TLV_STAT_NOT_IMPLEMENTED   0x0002
#define TLV_STAT_INVALID_ARGUMENT  0x0003
#define TLV_STAT_FAILURE           0x0004

#pragma pack(push, 1)
typedef struct
{
    uint8_t major;
    uint8_t minor;
} GET_FW_VERSION_OUT;
#pragma pack(pop)

#define TLV_MAX_RX_PAYLOAD_SIZE 64
#define TLV_MAX_TX_PAYLOAD_SIZE sizeof(GET_FW_VERSION_OUT)
#define RX_RAW_PACKET_BUF_SIZE (TLV_RX_HEADER_SIZE + TLV_MAX_RX_PAYLOAD_SIZE + TLV_RX_FOOTER_SIZE)
#define TX_RAW_PACKET_BUF_SIZE (TLV_TX_HEADER_SIZE + TLV_MAX_TX_PAYLOAD_SIZE + TLV_TX_FOOTER_SIZE)

void Protocol_Process(void);

#endif
