CC = msp430-gcc
CFLAGS=-oS -D__MSP430G2553__ -g
LDFLAGS=-mmcu=msp430g2553
#CFLAGS=-oS -D__MSP430G2452__
#LDFLAGS=-mmcu=msp430g2452
OBJS_CLOCK1=clock1.o TI_USCI_I2C_master.o bbspi.o

all: clock1.elf

.PHONY: clean

clean:
	rm -f *.o *.elf

%.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $<


clock1.elf: $(OBJS_CLOCK1)
	msp430-gcc -oS $(LDFLAGS) -o $@ $(OBJS_CLOCK1)

