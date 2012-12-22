STM32 F3 discovery board code
===

The most interesting files are:
* `main.c`
* `gyro.c` and `compass.c` handle the sensor stuff, (but some of the compass heading calculation is still done in `main.c`)
* `interrupts.c` is actually just renamed, it was (confusingly) called `stm32f30x_it.c`, but it contains most of the interrupt handlers.
* `usart.c` contains USART output routines
   The USART interrupt lives here, not interrupts.c.
* `newlib.c` contains all the newlib (libc) interfacing stuff, including system call stubs, etc.
   `newlib.c` allows you to use printf as usual, whilst having the output come out of USART.
   Obviously `newlib.c` assumes that you're using the newlib library, which you need to have configure'd with `--disable-newlib-supplied-syscalls`.

If you get a hard fault exception when `printf`ing a `%d` bigger than 10, you have to re-configure and recompile both gcc and newlib with hard floats:
    --with-cpu=cortex-m4
    --with-float=hard
    --with-fpu=fpv4-sp-d16
    --with-mode=thumb

One issue is that printf ignores everything after the first newline you enter in it. so `printf("a\nb\n")` would only print `"a\n"`.
Setting the buffer mode seems to make it act strangely.
