#ifndef __EXTI_H
#define __EXTI_H
/*外部中断，中断服务函数
NVIC_IRQChannel:
  EXTI0_IRQHandler
  EXTI1_IRQHandler
  EXTI2_IRQHandler
  EXTI3_IRQHandler
  EXTI4_IRQHandler
  EXTI9_5_IRQHandler
  EXTI15_10_IRQHandler
  中断服务函数模板
void EXTI0_IRQHandler(void){
	if (EXTI_GetITStatus (EXTI_Line0) != RESET)
	{
		EXTI_ClearITPendingBit (EXTI_Line0);
		set_mA++;
	}
}
*/
//外部中断
void EXTI_init(uint8_t GPIO_PortSourceGPIOx,uint8_t GPIO_PinSourcex,uint8_t EXTITrigger_TypeDef,uint8_t SubPeriority);

#endif
