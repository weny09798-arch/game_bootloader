#include "board.h"
void Serial_Init(USART_TypeDef*,uint32_t);
uint8_t Serial_GetRxFlag(void);
uint8_t Serial_GetRxData(void);
void Serial_ClearRxFlag(void);
