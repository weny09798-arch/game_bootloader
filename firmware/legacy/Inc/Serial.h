#ifndef __SERIAL_H
#define __SERIAL_H

#include "stm32f10x.h"
#include <stdint.h>

extern char Serial_RxPacket[];

void Serial_Init(USART_TypeDef* USARTx, uint32_t BaudRate);

uint8_t Serial_GetRxFlag(void);
uint8_t Serial_GetRxData(void);
void Serial_ClearRxFlag(void);

#endif