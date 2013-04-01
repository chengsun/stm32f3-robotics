#include "usart.h"
#include "stm32f30x.h"
#include "stm32f3_discovery.h"

#include <string.h>

#define RINGBUF_SIZE (1<<RINGBUF_SIZE_BITS)
volatile uint8_t USART1_ringbuf[RINGBUF_SIZE];
volatile uint32_t USART1_readidx = 0;
volatile uint32_t USART1_writeidx = 0;

void USART1_Init(void)
{
    /* enable usart clock */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

  
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
  
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
  
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource8, GPIO_AF_7);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_7);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_7);
  
    USART_InitTypeDef USART_InitStructure;
    USART_StructInit(&USART_InitStructure);
    USART_InitStructure.USART_BaudRate = 57600;
    USART_Init(USART1, &USART_InitStructure);
  
    USART_Cmd(USART1, ENABLE);
  
    NVIC_EnableIRQ(USART1_IRQn);
}

void USART1_IRQHandler(void)
{
    if (USART1_writeidx - USART1_readidx == 0) {
        USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
        return;
    }
    USART_SendData(USART1, USART1_ringbuf[(USART1_readidx++) & (RINGBUF_SIZE-1)]);
}

#define MIN(a, b) ((a)<(b)?(a):(b))

void USART1_putc(char ch)
{
    while (1) {
        uint32_t capacity = RINGBUF_SIZE - (USART1_writeidx - USART1_readidx);
        if (capacity > 0) break;
    }
    USART1_ringbuf[(USART1_writeidx++) & (RINGBUF_SIZE-1)] = ch;
}

void USART1_write(const char *str, int len)
{
    uint32_t i = 0;
    while (i < len) {
        uint32_t writeidx = USART1_writeidx & (RINGBUF_SIZE-1);
        uint32_t len_to_end = RINGBUF_SIZE - writeidx;
        uint32_t capacity = RINGBUF_SIZE - (USART1_writeidx - USART1_readidx);
        uint32_t max_len = MIN(len_to_end, capacity);
        if (max_len == 0) continue;

        uint32_t this_len = MIN(max_len, len - i);

        int j;
        for (j = 0; j < this_len; ++j) {
            USART1_ringbuf[writeidx++] = str[i++];
        }
        USART1_writeidx += this_len;

        USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
    }
}

void USART1_print(const char *str)
{
    uint32_t len = strlen(str);
    USART1_write(str, len);
}

void USART1_directputc(const char ch)
{
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART_SendData(USART1, ch);
}

void USART1_directprint(const char *str)
{
    uint32_t i = 0;
    uint32_t len = strlen(str);
    while (i < len) {
        USART1_directputc(str[i++]);
    }
}

void USART1_flush(void)
{
    while (USART1_readidx != USART1_writeidx);
}
