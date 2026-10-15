#ifndef OLED_TEST_STM32_H
#define OLED_TEST_STM32_H
#include <stdint.h>
typedef struct { volatile uint32_t BSRR, BRR; } GPIO_TypeDef;
typedef struct { uint32_t unused; } I2C_TypeDef;
typedef struct { uint32_t unused; } USART_TypeDef;
typedef int GPIOMode_TypeDef;
typedef struct { uint16_t GPIO_Pin; int GPIO_Speed, GPIO_Mode; } GPIO_InitTypeDef;
#define GPIOA ((GPIO_TypeDef *)0x1000)
#define GPIOB ((GPIO_TypeDef *)0x2000)
#define GPIOC ((GPIO_TypeDef *)0x3000)
#define GPIOD ((GPIO_TypeDef *)0x4000)
#define GPIOE ((GPIO_TypeDef *)0x5000)
#define GPIOF ((GPIO_TypeDef *)0x6000)
#define GPIOG ((GPIO_TypeDef *)0x7000)
#define USART1 ((USART_TypeDef *)0x8000)
#define GPIO_Pin_6 (1u << 6)
#define GPIO_Pin_7 (1u << 7)
#define GPIO_Mode_Out_OD 1
#define GPIO_Speed_50MHz 1
#define Bit_RESET 0
#define ENABLE 1
#define RCC_APB2Periph_GPIOA 1
#define RCC_APB2Periph_GPIOB 2
#define RCC_APB2Periph_GPIOC 4
#define RCC_APB2Periph_GPIOD 8
#define RCC_APB2Periph_GPIOE 16
#define RCC_APB2Periph_GPIOF 32
#define RCC_APB2Periph_GPIOG 64
void RCC_APB2PeriphClockCmd(uint32_t clock, int enable);
void GPIO_Init(GPIO_TypeDef *gpio, GPIO_InitTypeDef *init);
#endif
