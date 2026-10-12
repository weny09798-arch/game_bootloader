#ifndef TEST_BOARD_H
#define TEST_BOARD_H
#include "ota.h"
typedef struct { unsigned unused; } USART_TypeDef;
void board_init(void);
uint32_t board_millis(void);
int board_key_pressed(void);
int board_rx_byte(uint8_t *);
int board_rx_fault(void);
void board_send(const uint8_t *,uint16_t);
const ota_flash_t *board_flash(void);
void board_screen(const char *,const char *,const char *);
#endif
