#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#define CMD_GET_VERSION     0x01
#define CMD_LED_ON          0x02
#define CMD_LED_OFF         0x03

#define RESP_OK             0x80
#define RESP_ERROR          0x81
#define RESP_VERSION        0x82

void Protocol_Process(void);

#endif
