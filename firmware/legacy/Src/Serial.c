#include "stm32f10x.h"
#include "Serial.h"
#include <stdint.h>

char Serial_RxPacket[100];

static volatile uint8_t Serial_RxData = 0;
static volatile uint8_t Serial_RxFlag = 0;

void Serial_Init(USART_TypeDef* USARTx, uint32_t BaudRate)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    if (USARTx == USART1)
    {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

        /* PA9 = USART1_TX */
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
        GPIO_Init(GPIOA, &GPIO_InitStructure);

        /* PA10 = USART1_RX */
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
        GPIO_Init(GPIOA, &GPIO_InitStructure);

        NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    }
    else if (USARTx == USART2)
    {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

        /* PA2 = USART2_TX */
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
        GPIO_Init(GPIOA, &GPIO_InitStructure);

        /* PA3 = USART2_RX */
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
        GPIO_Init(GPIOA, &GPIO_InitStructure);

        NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    }
    else if (USARTx == USART3)
    {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

        /* PB10 = USART3_TX */
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
        GPIO_Init(GPIOB, &GPIO_InitStructure);

        /* PB11 = USART3_RX */
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
        GPIO_Init(GPIOB, &GPIO_InitStructure);

        NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    }
    else
    {
        return;
    }

    USART_InitStructure.USART_BaudRate = BaudRate;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;

    USART_Init(USARTx, &USART_InitStructure);

    USART_ITConfig(USARTx, USART_IT_RXNE, ENABLE);

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(USARTx, ENABLE);
}

uint8_t Serial_GetRxFlag(void)
{
    return Serial_RxFlag;
}

uint8_t Serial_GetRxData(void)
{
    Serial_RxFlag = 0;
    return Serial_RxData;
}

void Serial_ClearRxFlag(void)
{
    Serial_RxFlag = 0;
}

void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
    {
        Serial_RxData = (uint8_t)USART_ReceiveData(USART1);
        Serial_RxFlag = 1;

        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}

void USART2_IRQHandler(void)
{
    if (USART_GetITStatus(USART2, USART_IT_RXNE) == SET)
    {
        Serial_RxData = (uint8_t)USART_ReceiveData(USART2);
        Serial_RxFlag = 1;

        USART_ClearITPendingBit(USART2, USART_IT_RXNE);
    }
}

void USART3_IRQHandler(void)
{
    if (USART_GetITStatus(USART3, USART_IT_RXNE) == SET)
    {
        Serial_RxData = (uint8_t)USART_ReceiveData(USART3);
        Serial_RxFlag = 1;

        USART_ClearITPendingBit(USART3, USART_IT_RXNE);
    }
}