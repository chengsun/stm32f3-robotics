#ifndef USART_H
#define USART_H

#define RINGBUF_SIZE_BITS 5
void USART1_Init(void);
void USART1_IRQHandler(void);
void USART1_putc(char ch);
void USART1_write(const char *str, int len);
void USART1_print(const char *str);
void USART1_directputc(const char ch);
void USART1_directprint(const char *str);
void USART1_flush(void);

#endif
