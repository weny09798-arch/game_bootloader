#ifndef _GENERIC_H
#define _GENERIC_H

#include "stm32f10x.h"
#include "stdio.h"

#define PA0(x) GPIO_WriteBit(GPIOA,GPIO_Pin_0,(BitAction)x)
#define PA1(x) GPIO_WriteBit(GPIOA,GPIO_Pin_1,(BitAction)x)
#define PA2(x) GPIO_WriteBit(GPIOA,GPIO_Pin_2,(BitAction)x)
#define PA3(x) GPIO_WriteBit(GPIOA,GPIO_Pin_3,(BitAction)x)
#define PA4(x) GPIO_WriteBit(GPIOA,GPIO_Pin_4,(BitAction)x)
#define PA5(x) GPIO_WriteBit(GPIOA,GPIO_Pin_5,(BitAction)x)
#define PA6(x) GPIO_WriteBit(GPIOA,GPIO_Pin_6,(BitAction)x)
#define PA7(x) GPIO_WriteBit(GPIOA,GPIO_Pin_7,(BitAction)x)
#define PA8(x) GPIO_WriteBit(GPIOA,GPIO_Pin_8,(BitAction)x)
#define PA9(x) GPIO_WriteBit(GPIOA,GPIO_Pin_9,(BitAction)x)
#define PA10(x) GPIO_WriteBit(GPIOA,GPIO_Pin_10,(BitAction)x)
#define PA11(x) GPIO_WriteBit(GPIOA,GPIO_Pin_11,(BitAction)x)
#define PA12(x) GPIO_WriteBit(GPIOA,GPIO_Pin_12,(BitAction)x)
#define PA13(x) GPIO_WriteBit(GPIOA,GPIO_Pin_13,(BitAction)x)
#define PA14(x) GPIO_WriteBit(GPIOA,GPIO_Pin_14,(BitAction)x)
#define PA15(x) GPIO_WriteBit(GPIOA,GPIO_Pin_15,(BitAction)x)
#define PB0(x) GPIO_WriteBit(GPIOB,GPIO_Pin_0,(BitAction)x)
#define PB1(x) GPIO_WriteBit(GPIOB,GPIO_Pin_1,(BitAction)x)
#define PB2(x) GPIO_WriteBit(GPIOB,GPIO_Pin_2,(BitAction)x)
#define PB3(x) GPIO_WriteBit(GPIOB,GPIO_Pin_3,(BitAction)x)
#define PB4(x) GPIO_WriteBit(GPIOB,GPIO_Pin_4,(BitAction)x)
#define PB5(x) GPIO_WriteBit(GPIOB,GPIO_Pin_5,(BitAction)x)
#define PB6(x) GPIO_WriteBit(GPIOB,GPIO_Pin_6,(BitAction)x)
#define PB7(x) GPIO_WriteBit(GPIOB,GPIO_Pin_7,(BitAction)x)
#define PB8(x) GPIO_WriteBit(GPIOB,GPIO_Pin_8,(BitAction)x)
#define PB9(x) GPIO_WriteBit(GPIOB,GPIO_Pin_9,(BitAction)x)
#define PB10(x) GPIO_WriteBit(GPIOB,GPIO_Pin_10,(BitAction)x)
#define PB11(x) GPIO_WriteBit(GPIOB,GPIO_Pin_11,(BitAction)x)
#define PB12(x) GPIO_WriteBit(GPIOB,GPIO_Pin_12,(BitAction)x)
#define PB13(x) GPIO_WriteBit(GPIOB,GPIO_Pin_13,(BitAction)x)
#define PB14(x) GPIO_WriteBit(GPIOB,GPIO_Pin_14,(BitAction)x)
#define PB15(x) GPIO_WriteBit(GPIOB,GPIO_Pin_15,(BitAction)x)
#define PC0(x) GPIO_WriteBit(GPIOC,GPIO_Pin_0,(BitAction)x)
#define PC1(x) GPIO_WriteBit(GPIOC,GPIO_Pin_1,(BitAction)x)
#define PC2(x) GPIO_WriteBit(GPIOC,GPIO_Pin_2,(BitAction)x)
#define PC3(x) GPIO_WriteBit(GPIOC,GPIO_Pin_3,(BitAction)x)
#define PC4(x) GPIO_WriteBit(GPIOC,GPIO_Pin_4,(BitAction)x)
#define PC5(x) GPIO_WriteBit(GPIOC,GPIO_Pin_5,(BitAction)x)
#define PC6(x) GPIO_WriteBit(GPIOC,GPIO_Pin_6,(BitAction)x)
#define PC7(x) GPIO_WriteBit(GPIOC,GPIO_Pin_7,(BitAction)x)
#define PC8(x) GPIO_WriteBit(GPIOC,GPIO_Pin_8,(BitAction)x)
#define PC9(x) GPIO_WriteBit(GPIOC,GPIO_Pin_9,(BitAction)x)
#define PC10(x) GPIO_WriteBit(GPIOC,GPIO_Pin_10,(BitAction)x)
#define PC11(x) GPIO_WriteBit(GPIOC,GPIO_Pin_11,(BitAction)x)
#define PC12(x) GPIO_WriteBit(GPIOC,GPIO_Pin_12,(BitAction)x)
#define PC13(x) GPIO_WriteBit(GPIOC,GPIO_Pin_13,(BitAction)x)
#define PC14(x) GPIO_WriteBit(GPIOC,GPIO_Pin_14,(BitAction)x)
#define PC15(x) GPIO_WriteBit(GPIOC,GPIO_Pin_15,(BitAction)x)

void Delay_us(uint32_t us);
void Delay_ms(uint32_t ms);
void Delay_s(uint32_t s);
void Delay_100ns(uint32_t n);//Basic in TIMER������򿪶�ʱ������������ʹ��

void Pin_init(GPIO_TypeDef* GPIOx,uint32_t PINx,GPIOMode_TypeDef GPIO_MODE);
void PIN_Toggle(GPIO_TypeDef* GPIOx,uint32_t PINx);

#endif 
