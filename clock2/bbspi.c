

#include "bbspi.h"

static void setSS() {
  SPIOUT |= SPI_SS;
}

static void clrSS() {
  SPIOUT &= ~SPI_SS;
}


static void setMosi() {
  SPIOUT |= SPI_MOSI;
}

static void clrMosi() {
  SPIOUT &= ~SPI_MOSI;
}

static void clrClk() {
  SPIOUT &= ~SPI_CLK;
}

static void setClk() {
  SPIOUT |= SPI_CLK;
}

static void clrRst() {
  SPIOUT &= ~SPI_RST;
}

static void setRst() {
  SPIOUT |= SPI_RST;
}

static void spiDelay() {
  volatile int i;
  for (i = 0; i < 100; i++);
}


void initSPI() {
  SPIDIR |= SPI_CLK | SPI_MOSI | SPI_RST | SPI_SS;
  setSS();
  clrRst(); //turn off device
  
  clrClk();
  clrMosi();
  spiDelay();

  setRst(); // turns on device
  clrSS();
}

static int savedSEL1, savedSEL2;



void sendByteSPI(unsigned char byte) {
  unsigned char bit;

  clrSS();

  for (bit = 0; bit < 8; bit++) {
    spiDelay();

    if (byte & 0x80) {
      setMosi();
    } else {
      clrMosi();
    }

    setClk();

    spiDelay();

    clrClk();


    byte <<= 1;
  }

  setSS();

}
