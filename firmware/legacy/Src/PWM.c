#include "stm32f10x.h"                  // Device header
#include "PWM.h"
#include "stdio.h"

uint32_t duty_set = 0;

/*例子:PWM_Init_ms(TIM_TypeDef* TIMx,uint8_t CHannel,uint32_t Period,uint8_t original_Duty)
TIMx:选择定时器
Channel:选择通道
Period:填入周期(0.0001-8s)
original_Duty:填入初始占空比
*/
void PWM_Init_ms(TIM_TypeDef* TIMx,uint8_t CHannel,uint32_t Period,uint8_t original_Duty)
{
	TIM_InternalClockConfig(TIMx);//use internalClock
	if(TIMx==TIM2){
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_AFIO, ENABLE);
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	}
	else if(TIMx==TIM3){
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB|RCC_APB2Periph_AFIO, ENABLE);
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	}
	else if(TIMx==TIM4){
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD|RCC_APB2Periph_AFIO, ENABLE);
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
		GPIO_PinRemapConfig(GPIO_Remap_TIM4,ENABLE);
	}
	else if(TIMx==TIM5){
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_AFIO, ENABLE);
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);
	}
	//remap
	//GPIO_PinRemapConfig(GPIO_PartialRemap1_TIM2, ENABLE);
	//GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

	//set TIM
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;//Do not Division
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;//Counter Mode:UP edge
	TIM_TimeBaseInitStructure.TIM_Period = Period*10-1;	//ARR Count 10000 times
	duty_set = TIM_TimeBaseInitStructure.TIM_Period;
	TIM_TimeBaseInitStructure.TIM_Prescaler = 7200 - 1;	//PSC Division
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIMx, &TIM_TimeBaseInitStructure);
	
	//TIM output compare
	TIM_OCInitTypeDef TIM_OCInitStructure;
	TIM_OCStructInit(&TIM_OCInitStructure);//Auto init OC
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;//choose Polarity
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;//enable 
	TIM_OCInitStructure.TIM_Pulse = (TIM_TimeBaseInitStructure.TIM_Period +1)*original_Duty/100;	//CCR set Pulse

	if(TIMx==TIM2){
		switch (CHannel)
		{	//which CHannel do you want for PWM
			case 1:
				TIM_OC1Init(TIMx, &TIM_OCInitStructure);GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;break;
			case 2:
				TIM_OC2Init(TIMx, &TIM_OCInitStructure);GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;break;		
			case 3:
				TIM_OC3Init(TIMx, &TIM_OCInitStructure);GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;break;				
			case 4:
				TIM_OC4Init(TIMx, &TIM_OCInitStructure);GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;break;				
		}
		GPIO_Init(GPIOA, &GPIO_InitStructure);
	}
	else if(TIMx==TIM3){
		switch (CHannel)
		{	//which CHannel do you want for PWM
			case 1:
				TIM_OC1Init(TIMx, &TIM_OCInitStructure);GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
				GPIO_Init(GPIOA, &GPIO_InitStructure);break;
			case 2:
				TIM_OC2Init(TIMx, &TIM_OCInitStructure);GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
				GPIO_Init(GPIOA, &GPIO_InitStructure);break;
			case 3:
				TIM_OC3Init(TIMx, &TIM_OCInitStructure);GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
				GPIO_Init(GPIOB, &GPIO_InitStructure);break;				
			case 4:
				TIM_OC4Init(TIMx, &TIM_OCInitStructure);GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
				GPIO_Init(GPIOB, &GPIO_InitStructure);break;				
		}
	}
	else if(TIMx==TIM4){
		switch (CHannel)
		{	//which CHannel do you want for PWM
			case 1:
				TIM_OC1Init(TIMx, &TIM_OCInitStructure);GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;break;
			case 2:
				TIM_OC2Init(TIMx, &TIM_OCInitStructure);GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;break;	
			case 3:
				TIM_OC3Init(TIMx, &TIM_OCInitStructure);GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;break;				
			case 4:
				TIM_OC4Init(TIMx, &TIM_OCInitStructure);GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;break;				
		}
		GPIO_Init(GPIOD, &GPIO_InitStructure);			
	}
	else if(TIMx==TIM5){
		switch (CHannel)
		{	//which CHannel do you want for PWM
			case 1:
				TIM_OC1Init(TIMx, &TIM_OCInitStructure);GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;break;
			case 2:
				TIM_OC2Init(TIMx, &TIM_OCInitStructure);GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;break;	
			case 3:
				TIM_OC3Init(TIMx, &TIM_OCInitStructure);GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;break;				
			case 4:
				TIM_OC4Init(TIMx, &TIM_OCInitStructure);GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;break;				
		}
		GPIO_Init(GPIOA, &GPIO_InitStructure);			
	}
	//every channel frequnt is same cause their Timer is same

	TIM_Cmd(TIMx, ENABLE);//open TIM2
}
/*例子:PWM_Init_Hz(TIM2,1,10000,50);//一个ITM2_CH2的频率10KHz占空比50%的方波
TIMx:选择定时器
Channel:(TIM2~TIM5)选择通道
Hz:(0~10MHz)填入频率
original_Duty:(0~100)填入初始占空比
注意:频率是奇数容易导致占空比不准，因为计算值会出小数，但是arr和psc中没有小数，会导致丢失小数位
*/
void PWM_Init_Hz(TIM_TypeDef* TIMx,uint8_t CHannel,uint32_t Hz,uint8_t original_Duty){
	TIM_InternalClockConfig(TIMx);//use internalClock
	if(TIMx==TIM2){
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_AFIO, ENABLE);
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	}
	else if(TIMx==TIM3){
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB|RCC_APB2Periph_AFIO, ENABLE);
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	}
	else if(TIMx==TIM4){
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD|RCC_APB2Periph_AFIO, ENABLE);
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
		GPIO_PinRemapConfig(GPIO_Remap_TIM4,ENABLE);
	}
	else if(TIMx==TIM5){
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_AFIO, ENABLE);
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);
	}
	//remap
	//GPIO_PinRemapConfig(GPIO_PartialRemap1_TIM2, ENABLE);
	//GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

	//set TIM
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;//Do not Division
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;//Counter Mode:UP edge
	if(Hz>=1000000){
		TIM_TimeBaseInitStructure.TIM_Period = 24000000/Hz-1;
		TIM_TimeBaseInitStructure.TIM_Prescaler = 3 - 1;
	}
	else if(Hz>=10000 && Hz<1000000){
		TIM_TimeBaseInitStructure.TIM_Period = 4000000/Hz-1;
		TIM_TimeBaseInitStructure.TIM_Prescaler = 18 - 1;
	}
	else{
		TIM_TimeBaseInitStructure.TIM_Period = 100000/Hz-1;
		TIM_TimeBaseInitStructure.TIM_Prescaler = 720 - 1;
	}
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIMx, &TIM_TimeBaseInitStructure);
	duty_set = TIM_TimeBaseInitStructure.TIM_Period;
	
	//TIM output compare
	TIM_OCInitTypeDef TIM_OCInitStructure;
	TIM_OCStructInit(&TIM_OCInitStructure);//Auto init OC
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;//choose Polarity
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;//enable 
	TIM_OCInitStructure.TIM_Pulse = (TIM_TimeBaseInitStructure.TIM_Period+1)*original_Duty/100;	//CCR set Pulse

	if(TIMx==TIM2){
		switch (CHannel)
		{	//which CHannel do you want for PWM
			case 1:
				TIM_OC1Init(TIMx, &TIM_OCInitStructure);GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;break;
			case 2:
				TIM_OC2Init(TIMx, &TIM_OCInitStructure);GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;break;		
			case 3:
				TIM_OC3Init(TIMx, &TIM_OCInitStructure);GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;break;				
			case 4:
				TIM_OC4Init(TIMx, &TIM_OCInitStructure);GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;break;				
		}
		GPIO_Init(GPIOA, &GPIO_InitStructure);
	}
	else if(TIMx==TIM3){
		switch (CHannel)
		{	//which CHannel do you want for PWM
			case 1:
				TIM_OC1Init(TIMx, &TIM_OCInitStructure);GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
				GPIO_Init(GPIOA, &GPIO_InitStructure);break;
			case 2:
				TIM_OC2Init(TIMx, &TIM_OCInitStructure);GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
				GPIO_Init(GPIOA, &GPIO_InitStructure);break;
			case 3:
				TIM_OC3Init(TIMx, &TIM_OCInitStructure);GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
				GPIO_Init(GPIOB, &GPIO_InitStructure);break;				
			case 4:
				TIM_OC4Init(TIMx, &TIM_OCInitStructure);GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
				GPIO_Init(GPIOB, &GPIO_InitStructure);break;				
		}
	}
	else if(TIMx==TIM4){
		switch (CHannel)
		{	//which CHannel do you want for PWM
			case 1:
				TIM_OC1Init(TIMx, &TIM_OCInitStructure);GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;break;
			case 2:
				TIM_OC2Init(TIMx, &TIM_OCInitStructure);GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;break;	
			case 3:
				TIM_OC3Init(TIMx, &TIM_OCInitStructure);GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;break;				
			case 4:
				TIM_OC4Init(TIMx, &TIM_OCInitStructure);GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;break;				
		}
		GPIO_Init(GPIOD, &GPIO_InitStructure);			
	}
	else if(TIMx==TIM5){
		switch (CHannel)
		{	//which CHannel do you want for PWM
			case 1:
				TIM_OC1Init(TIMx, &TIM_OCInitStructure);GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;break;
			case 2:
				TIM_OC2Init(TIMx, &TIM_OCInitStructure);GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;break;	
			case 3:
				TIM_OC3Init(TIMx, &TIM_OCInitStructure);GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;break;				
			case 4:
				TIM_OC4Init(TIMx, &TIM_OCInitStructure);GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;break;				
		}
		GPIO_Init(GPIOA, &GPIO_InitStructure);			
	}
	//every channel frequnt is same cause their Timer is same

	TIM_Cmd(TIMx, ENABLE);//open TIM2
}
/*例子:PWM_Duty(TIM2,1,20);把TIM2的CH1重新设置占空比为20%
TIMx:选择定时器
Channel:选择通道
Duty:填入占空比(不要输入小数)
*/
void PWM_Duty(TIM_TypeDef* TIMx,uint8_t CH,float Duty){//runing change CCR
	switch(CH){
		case 1:	TIM_SetCompare1(TIMx, (float)duty_set*Duty/100);break;
		case 2:	TIM_SetCompare2(TIMx, (float)duty_set*Duty/100);break;
		case 3:	TIM_SetCompare3(TIMx, (float)duty_set*Duty/100);break;
		case 4:	TIM_SetCompare4(TIMx, (float)duty_set*Duty/100);break;
	}
}

//(   TIM1||TIM8    ,   CH1~CH3   ,    Period 0 ~0.0001   ,  ENABLE your Output_N)
void PWM_Advance_CG_init(uint8_t Timer,uint8_t CHannel,double Period,FunctionalState NewState){
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;//Do not Division
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;//Counter Mode:UP edge
	TIM_TimeBaseInitStructure.TIM_Period = 10000-1;	//ARR Count 10000 times
	TIM_TimeBaseInitStructure.TIM_Prescaler = 7200*Period - 1;	//PSC Division
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;

	TIM_OCInitTypeDef TIM_OC_init_struct;
	TIM_OC_init_struct.TIM_OCMode=TIM_OCMode_PWM1;
	TIM_OC_init_struct.TIM_OutputState=TIM_OutputState_Enable;
	if(NewState==ENABLE)TIM_OC_init_struct.TIM_OutputNState = TIM_OutputNState_Enable; //N
	else if(NewState==DISABLE){TIM_OC_init_struct.TIM_OutputNState = TIM_OutputNState_Disable;} //N
	TIM_OC_init_struct.TIM_Pulse = 0;
	TIM_OC_init_struct.TIM_OCPolarity=TIM_OCPolarity_High;
	TIM_OC_init_struct.TIM_OCNPolarity=TIM_OCNPolarity_High;//N
	TIM_OC_init_struct.TIM_OCNIdleState = TIM_OCNIdleState_Reset;//互补空闲电平极性配置
	
	GPIO_InitTypeDef GPIO_Init_struct;
	GPIO_Init_struct.GPIO_Mode=GPIO_Mode_AF_PP;
	GPIO_Init_struct.GPIO_Speed=GPIO_Speed_50MHz;
	if(Timer==1){
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOE|RCC_APB2Periph_AFIO,ENABLE);
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseInitStructure);
		switch(CHannel){////////////////////TIM1
		case 1:	GPIO_Init_struct.GPIO_Pin=GPIO_Pin_8;//A8  N_B13
				GPIO_Init(GPIOA,&GPIO_Init_struct);
				GPIO_Init_struct.GPIO_Pin=GPIO_Pin_13;
				GPIO_Init(GPIOB,&GPIO_Init_struct);
				TIM_OC1Init(TIM1,&TIM_OC_init_struct);
				TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
				break;
		case 2:	GPIO_Init_struct.GPIO_Pin=GPIO_Pin_9;//A9  N_B14
				GPIO_Init(GPIOA,&GPIO_Init_struct);
				GPIO_Init_struct.GPIO_Pin=GPIO_Pin_14;
				GPIO_Init(GPIOB,&GPIO_Init_struct);
				TIM_OC2Init(TIM1,&TIM_OC_init_struct);
				TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Enable);
				break;
		case 3:	GPIO_Init_struct.GPIO_Pin=GPIO_Pin_10;//A10  N_B15
				GPIO_Init(GPIOA,&GPIO_Init_struct);
				GPIO_Init_struct.GPIO_Pin=GPIO_Pin_15;
				GPIO_Init(GPIOB,&GPIO_Init_struct);
				TIM_OC3Init(TIM1,&TIM_OC_init_struct);
				TIM_OC3PreloadConfig(TIM1, TIM_OCPreload_Enable);
				break;
		}
		TIM_Cmd(TIM1, ENABLE);	
		TIM_CtrlPWMOutputs(TIM1, ENABLE);// 主输出使能，当使用的是通用定时器时，这句不需要
	}
	else if(Timer==8){
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM8,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOC|RCC_APB2Periph_AFIO,ENABLE);
	TIM_TimeBaseInit(TIM8, &TIM_TimeBaseInitStructure);
		switch(CHannel){////////////////////TIM8
			case 1:	GPIO_Init_struct.GPIO_Pin=GPIO_Pin_7;//C6  N_A7
					GPIO_Init(GPIOA,&GPIO_Init_struct);
					GPIO_Init_struct.GPIO_Pin=GPIO_Pin_6;
					GPIO_Init(GPIOC,&GPIO_Init_struct);
					TIM_OC1Init(TIM8,&TIM_OC_init_struct);
					TIM_OC1PreloadConfig(TIM8, TIM_OCPreload_Enable);
					break;
			case 2:	GPIO_Init_struct.GPIO_Pin=GPIO_Pin_7;//C7  N_B0
					GPIO_Init(GPIOC,&GPIO_Init_struct);
					GPIO_Init_struct.GPIO_Pin=GPIO_Pin_0;
					GPIO_Init(GPIOB,&GPIO_Init_struct);
					TIM_OC2Init(TIM8,&TIM_OC_init_struct);
					TIM_OC2PreloadConfig(TIM8, TIM_OCPreload_Enable);
					break;
			case 3:	GPIO_Init_struct.GPIO_Pin=GPIO_Pin_8;//C8  N_B1
					GPIO_Init(GPIOC,&GPIO_Init_struct);
					GPIO_Init_struct.GPIO_Pin=GPIO_Pin_1;
					GPIO_Init(GPIOB,&GPIO_Init_struct);
					TIM_OC3Init(TIM8,&TIM_OC_init_struct);
					TIM_OC3PreloadConfig(TIM8, TIM_OCPreload_Enable);
					break;
		}
		TIM_Cmd(TIM8, ENABLE);	
		TIM_CtrlPWMOutputs(TIM8, ENABLE);// 主输出使能，当使用的是通用定时器时，这句不需要
	}

}

//TIM1  ||  TIM8
void PWM_Add_Death(TIM_TypeDef* TIMx,uint8_t Dead_Time){
	GPIO_InitTypeDef GPIO_Init_struct;
	GPIO_Init_struct.GPIO_Mode=GPIO_Mode_AF_PP;
	GPIO_Init_struct.GPIO_Speed=GPIO_Speed_50MHz;

	if(TIMx==TIM1){
		GPIO_Init_struct.GPIO_Pin=GPIO_Pin_12;
		GPIO_Init(GPIOB,&GPIO_Init_struct);//braek in B12
		GPIO_WriteBit(GPIOB,GPIO_Pin_12,Bit_RESET);
	}
	else if(TIMx==TIM8){
		GPIO_Init_struct.GPIO_Pin=GPIO_Pin_6;
		GPIO_Init(GPIOA,&GPIO_Init_struct);//break in A6
		GPIO_WriteBit(GPIOA,GPIO_Pin_6,Bit_RESET);
	}
	//set Death Zone
	TIM_BDTRInitTypeDef TIM_BDTR_struct_init;
	TIM_BDTR_struct_init.TIM_OSSIState=TIM_OSSIState_Enable;
	TIM_BDTR_struct_init.TIM_OSSRState = TIM_OSSRState_Enable;
	TIM_BDTR_struct_init.TIM_LOCKLevel = TIM_LOCKLevel_1;
	TIM_BDTR_struct_init.TIM_DeadTime = Dead_Time;//0x00 ~ 0xff .you can set ded tiem here 
	//break Mode
    TIM_BDTR_struct_init.TIM_Break = TIM_Break_Enable;
	TIM_BDTR_struct_init.TIM_BreakPolarity = TIM_BreakPolarity_High;
    TIM_BDTR_struct_init.TIM_AutomaticOutput = TIM_AutomaticOutput_Enable;
	TIM_BDTRConfig(TIMx, &TIM_BDTR_struct_init);

	TIM_Cmd(TIMx, ENABLE);
	TIM_CtrlPWMOutputs(TIMx, ENABLE);
}
/*例子:Servo_init(TIM2,1)这个时候你的PA0会被初始化
TIMx:(TIM2~TIM5)选择你的定时器
CHannel:(1~4)选择定时器的第几通道
*/
void Servo_init(TIM_TypeDef* TIMx,uint8_t CHannel){
	//时钟自动选择
	if(TIMx==TIM2){
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_AFIO, ENABLE);
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	}
	else if(TIMx==TIM3){
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB|RCC_APB2Periph_AFIO, ENABLE);
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	}
	else if(TIMx==TIM4){
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD|RCC_APB2Periph_AFIO, ENABLE);
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
		GPIO_PinRemapConfig(GPIO_Remap_TIM4,ENABLE);
	}
	else if(TIMx==TIM5){
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_AFIO, ENABLE);
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);
	}

	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_TimeBaseStructure.TIM_Period = 20000 - 1; 		   //20ms
    TIM_TimeBaseStructure.TIM_Prescaler = 72 - 1; 		   //1us计数频率
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;//不分频
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIMx, &TIM_TimeBaseStructure);

	TIM_OCInitTypeDef TIM_OCInitStructure;
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_Pulse = 0; //初始占空比为0%

	//引脚自动选择
	GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	if(TIMx==TIM2){
		switch (CHannel)
		{
			case 1:
				TIM_OC1Init(TIMx, &TIM_OCInitStructure);
				GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;break;
			case 2:
				TIM_OC2Init(TIMx, &TIM_OCInitStructure);
				GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;break;		
			case 3:
				TIM_OC3Init(TIMx, &TIM_OCInitStructure);
				GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;break;				
			case 4:
				TIM_OC4Init(TIMx, &TIM_OCInitStructure);
				GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;break;				
		}
		GPIO_Init(GPIOA, &GPIO_InitStructure);
	}
	else if(TIMx==TIM3){
		switch (CHannel)
		{	//which CHannel do you want for PWM
			case 1:
				TIM_OC1Init(TIMx, &TIM_OCInitStructure);GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
				GPIO_Init(GPIOA, &GPIO_InitStructure);break;
			case 2:
				TIM_OC2Init(TIMx, &TIM_OCInitStructure);GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
				GPIO_Init(GPIOA, &GPIO_InitStructure);break;
			case 3:
				TIM_OC3Init(TIMx, &TIM_OCInitStructure);GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
				GPIO_Init(GPIOB, &GPIO_InitStructure);break;				
			case 4:
				TIM_OC4Init(TIMx, &TIM_OCInitStructure);GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
				GPIO_Init(GPIOB, &GPIO_InitStructure);break;				
		}
	}
	else if(TIMx==TIM4){
		switch (CHannel)
		{	//which CHannel do you want for PWM
			case 1:
				TIM_OC1Init(TIMx, &TIM_OCInitStructure);
				GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;break;
			case 2:
				TIM_OC2Init(TIMx, &TIM_OCInitStructure);
				GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;break;	
			case 3:
				TIM_OC3Init(TIMx, &TIM_OCInitStructure);
				GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;break;				
			case 4:
				TIM_OC4Init(TIMx, &TIM_OCInitStructure);
				GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;break;				
		}
		GPIO_Init(GPIOD, &GPIO_InitStructure);			
	}
	else if(TIMx==TIM5){
		switch (CHannel)
		{	//which CHannel do you want for PWM
			case 1:
				TIM_OC1Init(TIMx, &TIM_OCInitStructure);
				GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;break;
			case 2:
				TIM_OC2Init(TIMx, &TIM_OCInitStructure);
				GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;break;	
			case 3:
				TIM_OC3Init(TIMx, &TIM_OCInitStructure);
				GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;break;				
			case 4:
				TIM_OC4Init(TIMx, &TIM_OCInitStructure);
				GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;break;				
		}
		GPIO_Init(GPIOA, &GPIO_InitStructure);			
	}

    TIM_Cmd(TIMx, ENABLE);
}

//(TIM(x),CH(x),Angle(0~180))
/*例子:Servo_Set(TIM2,1,60)接PA0的舵机旋转60度
TIMx:(TIM2~TIM5)选择你的定时器
Angle:(0~180°)选择舵机旋转角度，可以是小数
*/
void Servo_Set(TIM_TypeDef* TIMx,uint8_t CH,float Angle){//enther your angle
	if(Angle>180){
		//printf("Servo_Set()中,你输入了一个错误的角度\n");
		return;
	}
	switch(CH){
		case 1:	TIM_SetCompare1(TIMx,Angle / 180 * 2500 );break;
		case 2:	TIM_SetCompare2(TIMx,Angle / 180 * 2500 );break;
		case 3:	TIM_SetCompare3(TIMx,Angle / 180 * 2500 );break;
		case 4:	TIM_SetCompare4(TIMx,Angle / 180 * 2500 );break;
	}
}
/*
void PWM_Setprescaler(uint16_t prescaler){//we can change PSC by this function
	TIM_PrescalerConfig(TIM2,prescaler,TIM_PSCReloadMode_Immediate);
}
*/
/*
void PWM_init(uint16_t arr,uint16_t psc){
    RCC_APB1PeriphClockCmd(RCC_APB1ENR_TIM3EN,ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB|RCC_APB2Periph_AFIO,ENABLE);

    //GPIO_PinRemapConfig(GPIO_PartialRemap_TIM3, ENABLE); //重映射 TIM3_CH2->PB5

    //we use GPIO_B_Pin_5 for PWM output
    GPIO_InitTypeDef GPIO_INIT_struct;
    GPIO_INIT_struct.GPIO_Mode=GPIO_Mode_AF_PP;
    GPIO_INIT_struct.GPIO_Pin=GPIO_Pin_0;
    GPIO_INIT_struct.GPIO_Speed=GPIO_Speed_50MHz;
    GPIO_Init(GPIOB,&GPIO_INIT_struct);

    //we set TIM3 mode here
    TIM_TimeBaseInitTypeDef TIM3_INIT_Struct;
    TIM3_INIT_Struct.TIM_ClockDivision=TIM_CKD_DIV1;//No Division
    TIM3_INIT_Struct.TIM_CounterMode=TIM_CounterMode_Up;//Count UP
    TIM3_INIT_Struct.TIM_RepetitionCounter=0;
    TIM3_INIT_Struct.TIM_Period=arr;
    TIM3_INIT_Struct.TIM_Prescaler=psc;
    TIM_TimeBaseInit(TIM3,&TIM3_INIT_Struct);

    //output compare
    TIM_OCInitTypeDef TIM_OCInitStructure;
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1; //选择 PWM 模式 2
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; //比较输出使能
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; //输出极性高

    TIM_OC3Init(TIM3, &TIM_OCInitStructure); //初始化外设 TIM3 OC2
    //TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable);

    TIM_Cmd(TIM3, ENABLE); //Enable TIM3
}
*/
