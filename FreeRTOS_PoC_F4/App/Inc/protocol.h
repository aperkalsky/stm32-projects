#ifndef __PROTOCOL_H
#define __PROTOCOL_H

typedef uint8_t ProtocolStatus_t;

#define PROT_OK 					((ProtocolStatus_t)0x00)
#define PROT_ERROR	 			((ProtocolStatus_t)0x01)

const ProtocolStatus_t ProtocolProcess(uint8_t* rxBuf, uint16_t rxLen, uint8_t* txBuf, uint16_t txLen);

#endif



