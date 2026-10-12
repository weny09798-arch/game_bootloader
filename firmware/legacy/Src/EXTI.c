#include "stm32f10x.h"
#include "generic.h"

/*
GPIO_PortSourceGPIOx:x can be(A~G)选择GPIO
GPIO_PinSourcex: x can be (0~15)选择
EXTI_Trigger_Mode:EXTI_Trigger_Rising,EXTI_Trigger_Falling,EXTI_Trigger_Rising_Falling;触发模式
SubPeriority:0~4优先级
##已经把中断分组为第四组，你最多只能配置4个优先级,记得线初始化引脚!!!!!
*/
void EXTI_init(uint8_t GPIO_PortSourceGPIOx,uint8_t GPIO_PinSourcex,EXTITrigger_TypeDef EXTI_Trigger_Mode,uint8_t SubPeriority){
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);//Open AFIO Clock

	GPIO_EXTILineConfig(GPIO_PortSourceGPIOx,GPIO_PinSourcex);//which pin do you want for EXTI
	EXTI_InitTypeDef EXTI_sturcture;
	switch(GPIO_PinSourcex){
		case GPIO_PinSource0 :EXTI_sturcture.EXTI_Line=EXTI_Line0;break;
		case GPIO_PinSource1 :EXTI_sturcture.EXTI_Line=EXTI_Line1;break;
		case GPIO_PinSource2 :EXTI_sturcture.EXTI_Line=EXTI_Line2;break;	
		case GPIO_PinSource3 :EXTI_sturcture.EXTI_Line=EXTI_Line3;break;
		case GPIO_PinSource4 :EXTI_sturcture.EXTI_Line=EXTI_Line4;break;
		case GPIO_PinSource5 :EXTI_sturcture.EXTI_Line=EXTI_Line5;break;
		case GPIO_PinSource6 :EXTI_sturcture.EXTI_Line=EXTI_Line6;break;
		case GPIO_PinSource7 :EXTI_sturcture.EXTI_Line=EXTI_Line7;break;
		case GPIO_PinSource8 :EXTI_sturcture.EXTI_Line=EXTI_Line8;break;
		case GPIO_PinSource9 :EXTI_sturcture.EXTI_Line=EXTI_Line9;break;
		case GPIO_PinSource10 :EXTI_sturcture.EXTI_Line=EXTI_Line10;break;
		case GPIO_PinSource11 :EXTI_sturcture.EXTI_Line=EXTI_Line11;break;	
		case GPIO_PinSource12 :EXTI_sturcture.EXTI_Line=EXTI_Line12;break;
		case GPIO_PinSource13 :EXTI_sturcture.EXTI_Line=EXTI_Line13;break;
		case GPIO_PinSource14 :EXTI_sturcture.EXTI_Line=EXTI_Line14;break;
		case GPIO_PinSource15 :EXTI_sturcture.EXTI_Line=EXTI_Line15;break;
	}
	EXTI_sturcture.EXTI_LineCmd=ENABLE;
	EXTI_sturcture.EXTI_Mode=EXTI_Mode_Interrupt;
	EXTI_sturcture.EXTI_Trigger=EXTI_Trigger_Mode;
	EXTI_Init(&EXTI_sturcture);
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);//设置成第四组
	NVIC_InitTypeDef NVIC_Initstructure;
		switch(GPIO_PinSourcex){
		case GPIO_PinSource0 :NVIC_Initstructure.NVIC_IRQChannel=EXTI0_IRQn;break;
		case GPIO_PinSource1 :NVIC_Initstructure.NVIC_IRQChannel=EXTI1_IRQn;break;
		case GPIO_PinSource2 :NVIC_Initstructure.NVIC_IRQChannel=EXTI2_IRQn;break;	
		case GPIO_PinSource3 :NVIC_Initstructure.NVIC_IRQChannel=EXTI3_IRQn;break;
		case GPIO_PinSource4 :NVIC_Initstructure.NVIC_IRQChannel=EXTI4_IRQn;break;
		case GPIO_PinSource5 :NVIC_Initstructure.NVIC_IRQChannel=EXTI9_5_IRQn;break;
		case GPIO_PinSource6 :NVIC_Initstructure.NVIC_IRQChannel=EXTI9_5_IRQn;break;
		case GPIO_PinSource7 :NVIC_Initstructure.NVIC_IRQChannel=EXTI9_5_IRQn;break;
		case GPIO_PinSource8 :NVIC_Initstructure.NVIC_IRQChannel=EXTI9_5_IRQn;break;
		case GPIO_PinSource9 :NVIC_Initstructure.NVIC_IRQChannel=EXTI9_5_IRQn;break;
		case GPIO_PinSource10 :NVIC_Initstructure.NVIC_IRQChannel=EXTI15_10_IRQn;break;
		case GPIO_PinSource11 :NVIC_Initstructure.NVIC_IRQChannel=EXTI15_10_IRQn;break;	
		case GPIO_PinSource12 :NVIC_Initstructure.NVIC_IRQChannel=EXTI15_10_IRQn;break;
		case GPIO_PinSource13 :NVIC_Initstructure.NVIC_IRQChannel=EXTI15_10_IRQn;break;
		case GPIO_PinSource14 :NVIC_Initstructure.NVIC_IRQChannel=EXTI15_10_IRQn;break;
		case GPIO_PinSource15 :NVIC_Initstructure.NVIC_IRQChannel=EXTI15_10_IRQn;break;
	}
	NVIC_Initstructure.NVIC_IRQChannelCmd=ENABLE;//使能中断线
	NVIC_Initstructure.NVIC_IRQChannelPreemptionPriority=0;
	NVIC_Initstructure.NVIC_IRQChannelSubPriority=SubPeriority;//选择优先级

	NVIC_Init(&NVIC_Initstructure);
}

