#ifndef __PWM_H
#define __PWM_H

//TIM_CH GPIO_Pin praphic
/*
Universal Timer//////////////////////////////
TIM2_default:  	TIM2_remap:	|	TIM3_default:	TIM3_reamp:	 |	TIM4_reamp:		|	TIM5_default:
TIM2:CH1->A0				|	TIM3:CH1->A6	C6,B4		 |	TIM4:CH1->D12	|	IM5:CH1->A0
	 CH2->A1		B3		|		CH2->A7	    C7,B5		 |		CH2->D13	|		CH2->A1
	 CH3->A2		B10		|		CH3->B0	    C8			 |		CH3->D14	|		CH3->A2
	 CH4->A3		B11		|		CH4->B1	    C9			 |		CH4->D15	|		CH4->A3

Advance Timer////////////////////////////////
TIM1_default:	 TIM1_remap:  |	 TIM8_default:
TIM1:BKIN->B12	  A6,E15	  |	 TIM8:ETR->A0
	CH1N->B13	  A7,E8		  |	 	 BKIN->A6
	CH2N->B14	  B0,E10	  |	 	 CH1N->A7
	CH3N->B15	  B1,E12	  |		 CH2N->B0
	CH1->A8		  E9		  |		 CH3N->B1
	CH2->A9		  E11		  |		 CH1->C6
	CH3->A10	  E13		  |	     CH2->C7
	CH4->A11	  E14		  |		 CH3->C8
	ETR->A12	  E7		  |		 CH4->C9
*/

//通道快捷方式
#define CH1 1
#define CH2 2
#define CH3 3
#define CH4 4

void PWM_Init_ms(TIM_TypeDef* TIMx,uint8_t CHannel,uint32_t Period,uint8_t original_Duty);//按时间来设置PWM
void PWM_Init_Hz(TIM_TypeDef* TIMx,uint8_t CHannel,uint32_t Hz,uint8_t original_Duty);//按照频率来设置PWM
void PWM_Duty(TIM_TypeDef* TIMx,uint8_t CH,float Duty);//重新设置占空比
void PWM_Advance_CG_init(uint8_t Timer,uint8_t CHannel,double Period,FunctionalState NewState);//带死区控制
void PWM_Add_Death(TIM_TypeDef* TIMx,uint8_t Dead_Time);

//舵机
void Servo_init(TIM_TypeDef* TIMx,uint8_t CHannel);
void Servo_Set(TIM_TypeDef* TIMx,uint8_t CH,float Angle);

//void PWM_Setprescaler(uint16_t prescaler);//修改PSC
#endif
