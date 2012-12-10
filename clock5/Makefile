CC = msp430-gcc
AS = msp430-as
CFLAGS=-Os -D__MSP430F5510__ -I. -IUSB_config -IF5xx_F6xx_Core_Lib -D__TI_COMPILER_VERSION__ -D __REGISTER_MODEL__ -mmcu=msp430f5510 -Wall
ASFLAGS=-mmcu=msp430f5510
LDFLAGS=-mmcu=msp430f5510 -Wl,-Map=clock.map,--cref
#CFLAGS=-Os -D__MSP430G2452__
#LDFLAGS=-mmcu=msp430g2452
SRCS=$(shell find . -name '*.c' -or -name '*.asm')
#OBJS=$(addprefix objs/,$(notdir $(SRCS:%.c= %.o)))
OBJS1=$(SRCS:%.c=objs/%.o)
OBJS=$(OBJS1:%.asm=objs/%.o)

all: clock.elf

.PHONY: clean

clean:
	rm -rf objs/
	rm -f clock.elf

objs/%.o : %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ -c $<

objs/%.o : %.asm
	mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -o $@ -c $<


clock.elf: $(OBJS)
	msp430-gcc -oS $(LDFLAGS) -o $@ $(OBJS)

