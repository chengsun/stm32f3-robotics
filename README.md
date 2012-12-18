STM32 F3 discovery board code
===

The most interesting files are:
* `main.c`
* `gyro.c` and `compass.c` handle the sensor stuff, (but some of the compass heading calculation is still done in `main.c`)
* `interrupts.c` is actually just renamed, it was (confusingly) called `stm32f30x_it.c`, but it contains most of the interrupt handlers.
* `usart.c` (I might split this into two files later) contains all the newlib (libc) interfacing stuff, including but not only USART output routines, system call stubs, etc.
   The USART interrupt lives here, not interrupts.c.
   usart.c allows you to use printf as usual, whilst having the output come out of USART.
   usart.c assumes that you're using the newlib library, if so, you need to have configure'd it with `--disable-newlib-supplied-syscalls` in order for printf to work out of USART.

If you get a hard fault exception when printf'ing a %d number bigger than 10, you have to re-configure and recompile both gcc and newlib with hard floats:
--with-cpu=cortex-m4
--with-float=hard
--with-fpu=fpv4-sp-d16
--with-mode=thumb

One issue is that printf ignores everything after the first newline you enter in it. so printf("a\nb\n") would only print "a\n".
Setting the buffer mode seems to make it act strangely.
