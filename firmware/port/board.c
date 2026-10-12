#include "board.h"
#include "OLED_I2C.h"
#include <string.h>

#define RX_SIZE 512u
static volatile uint8_t rx[RX_SIZE];
static volatile uint16_t rx_head, rx_tail;
static volatile uint8_t rx_fault;
static volatile uint16_t rx_last_timer;
static volatile uint8_t key_event;
static uint32_t now_ms;
static uint16_t last_timer;
static uint16_t half_ms;
static uint8_t key_waiting, key_consumed;
static uint32_t key_at;

static int flash_read(void *user, uint32_t address, uint8_t *data, uint32_t len)
{
    (void)user;
    if (address < 0x08000000u || len > 0x10000u || address > 0x08010000u - len) return 0;
    memcpy(data, (const void *)address, len);
    return 1;
}

static int writable(uint32_t address, uint32_t len)
{
    return address >= 0x08004000u && len <= 0xC000u && address <= 0x08010000u - len;
}

static int flash_erase(void *user, uint32_t address)
{
    FLASH_Status status;
    uint32_t irq = __get_PRIMASK();
    (void)user;
    if (!writable(address, 0x400u) || (address & 0x3FFu)) return 0;
    RCC_HSICmd(ENABLE);
    while (RCC_GetFlagStatus(RCC_FLAG_HSIRDY) == RESET) {}
    __disable_irq();
    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
    status = FLASH_ErasePage(address);
    FLASH_Lock();
    __set_PRIMASK(irq);
    if (status != FLASH_COMPLETE) return 0;
    for (uint32_t i = 0; i < 0x400u; i += 4u)
        if (*(volatile uint32_t *)(address + i) != 0xFFFFFFFFu) return 0;
    return 1;
}

static int flash_write(void *user, uint32_t address, const uint8_t *data, uint32_t len)
{
    uint32_t irq;
    FLASH_Status status = FLASH_COMPLETE;
    (void)user;
    if ((address & 1u) || !len || !writable(address, (len + 1u) & ~1u)) return 0;
    RCC_HSICmd(ENABLE);
    while (RCC_GetFlagStatus(RCC_FLAG_HSIRDY) == RESET) {}
    irq = __get_PRIMASK();
    __disable_irq();
    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
    for (uint32_t i = 0; i < len; i += 2u) {
        uint16_t value = data[i] | ((uint16_t)(i + 1u < len ? data[i + 1u] : 0xFFu) << 8);
        if (*(volatile uint16_t *)(address + i) != 0xFFFFu) { status = FLASH_ERROR_PG; break; }
        status = FLASH_ProgramHalfWord(address + i, value);
        if (status != FLASH_COMPLETE || *(volatile uint16_t *)(address + i) != value) break;
    }
    FLASH_Lock();
    __set_PRIMASK(irq);
    if (status != FLASH_COMPLETE || memcmp((const void *)address, data, len)) return 0;
    return 1;
}

static const ota_flash_t flash_io = {0, flash_read, flash_erase, flash_write};
const ota_flash_t *board_flash(void) { return &flash_io; }

uint32_t board_millis(void)
{
    uint16_t count = (uint16_t)TIM2->CNT;
    uint32_t elapsed = (uint16_t)(count - last_timer) + half_ms;
    last_timer = count;
    now_ms += elapsed / 2u;
    half_ms = (uint16_t)(elapsed & 1u);
    return now_ms;
}

void board_wait(uint32_t ms)
{
    uint32_t start = board_millis();
    while ((uint32_t)(board_millis() - start) < ms) {}
}

void board_init(void)
{
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef uart;
    TIM_TimeBaseInitTypeDef timer;
    NVIC_InitTypeDef nvic;
    EXTI_InitTypeDef exti;
    RCC_ClocksTypeDef clocks;
    rx_head = rx_tail = 0;
    rx_fault = key_event = key_waiting = key_consumed = 0;
    now_ms = half_ms = last_timer = 0;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO | RCC_APB2Periph_USART1, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    RCC_GetClocksFreq(&clocks);
    TIM_TimeBaseStructInit(&timer);
    timer.TIM_Period = 65535;
    timer.TIM_Prescaler = (uint16_t)((clocks.PCLK1_Frequency * ((RCC->CFGR & RCC_CFGR_PPRE1) ? 2u : 1u)) / 2000u - 1u);
    TIM_TimeBaseInit(TIM2, &timer);
    TIM_SetCounter(TIM2, 0);
    TIM_Cmd(TIM2, ENABLE);
    gpio.GPIO_Pin = GPIO_Pin_9;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &gpio);
    gpio.GPIO_Pin = GPIO_Pin_10;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);
    gpio.GPIO_Pin = GPIO_Pin_0;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &gpio);
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource0);
    EXTI_ClearITPendingBit(EXTI_Line0);
    exti.EXTI_Line = EXTI_Line0;
    exti.EXTI_Mode = EXTI_Mode_Interrupt;
    exti.EXTI_Trigger = EXTI_Trigger_Falling;
    exti.EXTI_LineCmd = ENABLE;
    EXTI_Init(&exti);
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    nvic.NVIC_IRQChannel = EXTI0_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 1;
    nvic.NVIC_IRQChannelSubPriority = 0;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);
    USART_StructInit(&uart);
    uart.USART_BaudRate = 9600;
    USART_Init(USART1, &uart);
    nvic.NVIC_IRQChannel = USART1_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_Init(&nvic);
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART1, ENABLE);
    __enable_irq();
    key_event = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0;
}

void USART1_IRQHandler(void)
{
    uint32_t status = USART1->SR;
    if (status & (USART_SR_RXNE | USART_SR_ORE | USART_SR_FE | USART_SR_NE)) {
        uint8_t byte = (uint8_t)USART1->DR;
        rx_last_timer = (uint16_t)TIM2->CNT;
        uint16_t next = (uint16_t)((rx_head + 1u) & (RX_SIZE - 1u));
        if ((status & (USART_SR_ORE | USART_SR_FE | USART_SR_NE)) || next == rx_tail) rx_fault = 1;
        else if (!rx_fault) { rx[rx_head] = byte; rx_head = next; }
    }
}

int board_rx_byte(uint8_t *byte)
{
    if (rx_tail == rx_head || rx_fault) return 0;
    *byte = rx[rx_tail];
    rx_tail = (uint16_t)((rx_tail + 1u) & (RX_SIZE - 1u));
    return 1;
}

int board_rx_fault(void)
{
    uint32_t irq;
    if (!rx_fault) return 0;
    /* Discard the whole damaged burst before re-enabling frame/text parsing. */
    if ((uint16_t)((uint16_t)TIM2->CNT - rx_last_timer) < 200u) return 0;
    irq = __get_PRIMASK();
    __disable_irq();
    rx_tail = rx_head;
    rx_fault = 0;
    __set_PRIMASK(irq);
    return 1;
}

void board_send(const uint8_t *data, uint16_t len)
{
    board_wait(50); /* allow module RX-to-TX turnaround */
    for (uint16_t i = 0; i < len; ++i) {
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET) {}
        USART_SendData(USART1, data[i]);
    }
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) {}
}

void EXTI0_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line0) != RESET) {
        EXTI_ClearITPendingBit(EXTI_Line0);
        key_event = 1;
    }
}

int board_key_pressed(void)
{
    uint32_t now = board_millis();
    int low = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0;
    if (!low) { key_waiting = key_consumed = key_event = 0; return 0; }
    if (key_consumed) return 0;
    if (!key_waiting && key_event) { key_at = now; key_waiting = 1; key_event = 0; }
    if (key_waiting && (uint32_t)(now - key_at) >= 30u) {
        key_waiting = 0;
        key_consumed = 1;
        return 1;
    }
    return 0;
}

void board_screen(const char *a, const char *b, const char *c)
{
    OLED_Clear();
    OLED_ShowString(0, 0, (char *)a, 6);
    OLED_ShowString(0, 20, (char *)b, 6);
    OLED_ShowString(0, 40, (char *)c, 6);
    OLED_Update();
}

extern void board_jump_asm(uint32_t sp, uint32_t reset);
void board_jump_to_app(void)
{
    uint32_t sp = *(volatile uint32_t *)0x08004000u;
    uint32_t reset = *(volatile uint32_t *)0x08004004u;
    __disable_irq();
    USART_Cmd(USART1, DISABLE);
    TIM_Cmd(TIM2, DISABLE);
    SysTick->CTRL = SysTick->LOAD = SysTick->VAL = 0;
    for (uint32_t i = 0; i < 8; ++i) {
        NVIC->ICER[i] = 0xFFFFFFFFu;
        NVIC->ICPR[i] = 0xFFFFFFFFu;
    }
    SCB->ICSR = (1u << 25) | (1u << 27);
    RCC_DeInit();
    SCB->VTOR = 0x08004000u;
    __DSB();
    __ISB();
    board_jump_asm(sp, reset);
    while (1) {}
}

void board_reset(void) { NVIC_SystemReset(); }
