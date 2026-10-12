#ifndef GAME_BOARD_H
#define GAME_BOARD_H
#include "stm32f10x.h"
#include "ota.h"

void board_init(void);
uint32_t board_millis(void);
void board_wait(uint32_t ms);
int board_key_pressed(void);
int board_rx_byte(uint8_t *byte);
int board_rx_fault(void);
void board_send(const uint8_t *data, uint16_t length);
const ota_flash_t *board_flash(void);
void board_jump_to_app(void);
void board_reset(void);
void board_screen(const char *line1, const char *line2, const char *line3);
#endif
