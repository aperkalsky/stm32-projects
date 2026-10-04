#ifndef INC_CAN_TEST_H_
#define INC_CAN_TEST_H_

//#define DO_CAN_RX	// enable Rx
#define DO_CAN_TX	// enable Tx

void CAN_StartDriver();
void CAN_RxInit(void);
void CAN_TxInit(void);
void CAN_TestModifyTxData();
HAL_StatusTypeDef CAN_Send(void);

#endif
