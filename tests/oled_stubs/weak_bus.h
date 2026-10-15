#include "stm32f10x.h"
/* Weak transport declarations in the host build allow real graphics to use
 * the controller model without modifying any production source for tests. */
void OLED_WriteCommand(uint8_t command) __attribute__((weak));
void OLED_WriteData(uint8_t *data, uint8_t count) __attribute__((weak));
void OLED_GPIO_Init(GPIO_TypeDef *gpio, uint16_t scl, uint16_t sda) __attribute__((weak));
