CC = arm-none-eabi-gcc
CPPFLAGS = \
		 -I./Libraries/CMSIS/Device/ST/STM32F30x/Include/ \
		 -I./Libraries/STM32_USB-FS-Device_Driver/inc/ \
		 -I./Libraries/CMSIS/Include/ \
		 -I./Utilities/STM32F3_Discovery/ \
		 -I./Libraries/STM32F30x_StdPeriph_Driver/inc/ \
		 -I./ \
		 -DUSE_STDPERIPH_DRIVER \
		 -DARM_MATH_CM4
CFLAGS = -Wall -O0 -g -mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -L/usr/arm-none-eabi/lib/fpu/
LDFLAGS = -O0 -g -mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -lc -lm -L/usr/arm-none-eabi/lib/fpu/

all: program.bin

MY_SOURCES = $(wildcard *.c)
STM32F3_Discovery_SOURCES = $(wildcard Utilities/STM32F3_Discovery/*.c)
STM32F30x_StdPeriph_Driver_SOURCES = $(wildcard Libraries/STM32F30x_StdPeriph_Driver/src/*.c)
STM32_USBFS_Driver_SOURCES = $(wildcard Libraries/STM32_USB-FS-Device_Driver/src/*.c)

SOURCES = $(MY_SOURCES) \
		  $(STM32F3_Discovery_SOURCES) \
		  $(STM32F30x_StdPeriph_Driver_SOURCES) \
		  $(STM32_USBFS_Driver_SOURCES)

stm32f3_discovery.a: $(subst .c,.o,$(STM32F3_Discovery_SOURCES))
	ar rcs $@ $^
stm32f30x_stdperiph_driver.a: $(subst .c,.o,$(STM32F30x_StdPeriph_Driver_SOURCES))
	ar rcs $@ $^
stm32_usbfs_driver.a: $(subst .c,.o,$(STM32_USBFS_Driver_SOURCES))
	ar rcs $@ $^

startup.o: ./Libraries/CMSIS/Device/ST/STM32F30x/Source/Templates/gcc_ride7/startup_stm32f30x.s
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

program.bin: $(subst .c,.o,$(MY_SOURCES)) stm32f3_discovery.a stm32f30x_stdperiph_driver.a stm32_usbfs_driver.a startup.o
	$(CC) $(LDFLAGS) -nostartfiles -o $@ $^ -T stm32f303.ld


-include $(subst .c,.d,$(SOURCES))

%.d: %.c
	$(CC) -M $(CPPFLAGS) $< > $@.$$$$;                  \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

clean:
	rm -f *.d *.o *.a program.bin $(subst .c,.d,$(SOURCES)) $(subst .c,.o,$(SOURCES))

install-ocd: program.bin
	echo -e "reset halt\nflash erase_sector 0 0 127\nflash write_image program.bin\nreset init\nreset run\nexit" | netcat localhost 4444

install: program.bin
	st-util 2>&1 >/dev/null & \
	arm-none-eabi-gdb program.bin -ex "set confirm off" -ex "target extended-remote :4242" -ex "load" -ex "quit"

gdb: program.bin
	st-util 2>&1 >/dev/null & \
	arm-none-eabi-gdb program.bin -ex "target extended-remote :4242"

ocd-shell: program.bin
	telnet localhost 4444
