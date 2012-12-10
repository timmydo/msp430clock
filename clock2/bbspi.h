#ifndef __BBSPI__
#define __BBSPI__

#include <msp430.h>


#define SPIOUT P2OUT
#define SPIDIR P2DIR

#define SPI_CLK  BIT0
#define SPI_SS   BIT1
#define SPI_MOSI BIT2
#define SPI_RST  BIT3

void initSPI();
void sendByteSPI(unsigned char byte);



#endif
