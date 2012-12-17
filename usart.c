#include "usart.h"
#include "stm32f30x.h"
#include "stm32f3_discovery.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#undef errno
extern int errno;



#define RINGBUF_SIZE (1<<RINGBUF_SIZE_BITS)
volatile uint8_t USART1_ringbuf[RINGBUF_SIZE];
volatile uint32_t USART1_readidx = 0;
volatile uint32_t USART1_writeidx = 0;

void USART1_Init()
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
    USART_InitStructure.USART_BaudRate = 9600;
    USART_Init(USART1, &USART_InitStructure);
  
    USART_Cmd(USART1, ENABLE);
  
    NVIC_EnableIRQ(USART1_IRQn);
}

void USART1_IRQHandler()
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
        // this cast should be safe...
        memcpy((void *) USART1_ringbuf + writeidx, str + i, this_len);
        USART1_writeidx += this_len;
        i += this_len;

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

void USART1_flush()
{
    while (USART1_readidx != USART1_writeidx);
}

void _exit(int return_code) 
{
    printf("Program exited with code %d.\n", return_code);
    while (1);
}

FILEHANDLE _open (const char *name, int openmode) {
  return -1;
}

int _close (FILEHANDLE fh) 
{
    if (fh <= STDERR) 
    return (0);
    //return (__fclose (fh));
  return -1;
}

int _write (FILEHANDLE fh, const uint8_t *buf, uint32_t len, int mode) 
{
    if ((fh == STDOUT) || (fh == STDERR))
    {
        USART1_write((const char *) buf, len);
        return (0);
    };
    if (fh <= STDERR) return (-1);
    //return (__write (fh, buf, len));
  return -1;
}

int _read (FILEHANDLE fh, uint8_t *buf, uint32_t len, int mode) 
{
    if (fh == STDIN) {
        /*
        for (  ; len; len--) *buf++ = getkey ();
        return (0);
        */
    };
    if (fh <= STDERR) return (-1); 
    //return (__read (fh, buf, len));
  return -1;
}

int _isatty (FILEHANDLE fh) 
{
    if (fh <= STDERR) 
    return (1);    // Terminal I/O
    return (0); // Storage  I/O
}

register char *stack_ptr asm ("sp");

caddr_t _sbrk(int incr) {
  extern char _end;		/* Defined by the linker */
  static char *heap_end;
  char *prev_heap_end;

  if (heap_end == 0) {
    heap_end = &_end;
  }
  prev_heap_end = heap_end;
  if (heap_end + incr > stack_ptr) {
    USART1_write ("Heap and stack collision\n", 25);
    abort ();
  }

  heap_end += incr;
  return (caddr_t) prev_heap_end;
}

int _kill(int pid, int sig) {
    if (pid == 1) {
        switch (sig) {
        case SIGABRT:
            USART1_directprint("Aborted\n");
            _exit(128+sig);
            return 0;
        }
    }
    errno = EINVAL;
    return -1;
}

int _getpid() {
    return 1;
}

int _fstat(int file, struct stat *st) {
    st->st_mode = S_IFCHR;
    return 0;
}

int _lseek(int file, int ptr, int dir) {
    return 0;
}
