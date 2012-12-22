#include "newlib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#undef errno
extern int errno;


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
