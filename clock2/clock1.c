/* Demo app to blink the red LED on the TI Launchpad */

#include <msp430.h>

//for ISR support
#include <legacymsp430.h>
#include <isr_compat.h>

#include "TI_USCI_I2C_master.h"


#include "bbspi.h"


#define MY_ID 0x69
#define RTC_ID 0x68
#define BUTTON_1 BIT0
#define BUTTON_2 BIT1
#define AM_PM_BIT 0x20
#define TWELVE_HR_BIT 0x40

//not connected currently
//#define RTC_RST BIT4

unsigned char rtcdata[14] = { 0 };
unsigned char updatedata[10] = { 0 };
char timeString[20] = { 0 };
//                                addr  S  M     H                  D  D     M      Y
unsigned char defaultrtcdata[14] = { 0, 0, 0, 0x12 | TWELVE_HR_BIT, 5, 1, 0x11,  0x12};

#define RTC_SEC 0x00
#define RTC_MIN 0x01
#define RTC_HOUR 0x02
#define RTC_DATE 0x04
#define RTC_MONTH 0x05
#define RTC_YEAR 0x06

#define USE_SPI 1

void updateTimeString() {
  // HH:MM:SS YYYY-MM-DD
  timeString[0] = ((rtcdata[RTC_HOUR] & 0x10)>>4) + '0';
  timeString[1] = (rtcdata[RTC_HOUR] & 0xf) + '0';
  timeString[2] = ':';
  timeString[3] = ((rtcdata[RTC_MIN] & 0x70)>>4) + '0';
  timeString[4] = (rtcdata[RTC_MIN] & 0xf) + '0';
  timeString[5] = ':';
  timeString[6] = ((rtcdata[RTC_SEC] & 0x70)>>4) + '0';
  timeString[7] = (rtcdata[RTC_SEC] & 0xf) + '0';
  timeString[8] = ' ';
  timeString[9] = '2';
  timeString[10] = '0';
  timeString[11] = ((rtcdata[RTC_YEAR] & 0xf0)>>4) + '0';
  timeString[12] = (rtcdata[RTC_YEAR] & 0xf) + '0';
  timeString[13] = '-';
  timeString[14] = ((rtcdata[RTC_MONTH] & 0x10)>>4) + '0';
  timeString[15] = (rtcdata[RTC_MONTH] & 0xf) + '0';
  timeString[16] = '-';
  timeString[17] = ((rtcdata[RTC_DATE] & 0x30)>>4) + '0';
  timeString[18] = (rtcdata[RTC_DATE] & 0xf) + '0';

#if USE_SPI
  //sendByteSPI(0x76); //clear display
  sendByteSPI(0x79); //cursor set pos
  if (timeString[0] == '0') {
    timeString[0] = ' '; // display a blank
  }
  sendByteSPI(0x0); //pos = 0
  sendByteSPI(timeString[0]);
  sendByteSPI(timeString[1]);
  sendByteSPI(timeString[3]);
  sendByteSPI(timeString[4]);
#endif
}


static void delay() {
  volatile int i;
  for (i = 0; i < 100; i++);
}

// this lets us sleep and wake up from LPM3
static void setupWDT() {
  BCSCTL1 |= DIVA_1;                        // ACLK/2
  BCSCTL3 |= LFXT1S_2;                      // ACLK = VLO
  WDTCTL = WDT_ADLY_250;                   // Interval timer
  IE1 |= WDTIE;                             // Enable WDT interrupt
}

static void sleep() {

  _BIS_SR(LPM3_bits + GIE);
}



static void rtcOn() {
  P2OUT |= BIT5;
}

static void rtcOff() {
  P2OUT &= ~BIT5;
}

static void setCurrentTime() {
  int i;
  for (i = 0; i < 7; i++) {
    updatedata[i+1] = rtcdata[i];
  }
  //  memcpy(updatedata + 1, rtcdata, 7);
  updatedata[0] = 0; // memory address that we will write to
  TI_USCI_I2C_transmitinit(RTC_ID, 14);
  while ( TI_USCI_I2C_notready() );
  TI_USCI_I2C_transmit(8, updatedata);
  while ( TI_USCI_I2C_notready() );
}

static void incrementMinute() {
  int minute = ((rtcdata[RTC_MIN] & 0x70)>>4) * 10 + (rtcdata[RTC_MIN] & 0xf);
  minute = (minute + 1) % 60;
  rtcdata[RTC_MIN] = ((minute / 10) << 4) | (minute % 10);
  rtcdata[RTC_SEC] = 0;
}

static void incrementHour() {
  int hour = ((rtcdata[RTC_HOUR] & 0x10)>>4) * 10 + (rtcdata[RTC_HOUR] & 0xf);
  int am_pm_bit = rtcdata[RTC_HOUR] & AM_PM_BIT;
  hour++;
  if (hour > 12) {
    hour = 1;
    am_pm_bit ^= AM_PM_BIT;
  }
  rtcdata[RTC_HOUR] = ((hour / 10) << 4) | (hour % 10) | am_pm_bit;
}


int main(void) {
  volatile int i;
  int dotbits;
  WDTCTL = WDTPW + WDTHOLD; // Stop watchdog timer
  _EINT();
  P1DIR &= ~(BUTTON_1 | BUTTON_2);
  //P1REN |= BUTTON_1 | BUTTON_2;
  P1OUT = 0;
  
  P2SEL = 0;
  P2SEL2 = 0;

  rtcOff();
  delay();
  rtcOn();
 
  // let the rtc warm up
  for (i = 0; i < 20; i++) delay();



  // setup the clock with some default data
  TI_USCI_I2C_transmitinit(RTC_ID, 14);
  while ( TI_USCI_I2C_notready() );
  TI_USCI_I2C_transmit(8,defaultrtcdata);
  while ( TI_USCI_I2C_notready() );


#if USE_SPI
  initSPI();


  sendByteSPI(0x81); //factory reset

  sendByteSPI(0x77); // set other leds
  sendByteSPI(0b00010000); // set colon on

  sendByteSPI(0x76); //clear display
  sendByteSPI('1');
  sendByteSPI('2');
  sendByteSPI('3');
  sendByteSPI('4');
#endif
  

  setupWDT();

  for (;;) {

  // we tell it we want to read starting at addr 0
    TI_USCI_I2C_transmitinit(RTC_ID, 14);
    while ( TI_USCI_I2C_notready() );
    TI_USCI_I2C_transmit(1,defaultrtcdata);
    while ( TI_USCI_I2C_notready() );

    TI_USCI_I2C_receiveinit(RTC_ID, 14);
    TI_USCI_I2C_receive(7,rtcdata);

    updateTimeString();


    dotbits = 0;

    // blink the colon every other second
    if ((rtcdata[RTC_SEC] & 0xf) % 2) {
      dotbits ^= 0b10000; 
    }
    // set the PM light if pm
    if (rtcdata[RTC_HOUR] & AM_PM_BIT) {
      dotbits ^= 0b1000; 
    }
    
    sendByteSPI(0x77); // set other leds
    sendByteSPI(dotbits);


    //sleep before the button state so the user has time to remove their finger
    sleep();

    if ((P1IN & BUTTON_1) == 0) {
      incrementMinute();
      setCurrentTime();
    }
    if ((P1IN & BUTTON_2) == 0) {
      incrementHour();
      setCurrentTime();
    }
    
  }

  //while ( TI_USCI_I2C_notready() );


  return 0;

} 


ISR(WDT, WDTISR) {
  _BIC_SR_IRQ(LPM3_bits);                   // Clear LPM3 bits from 0(SR)
}
