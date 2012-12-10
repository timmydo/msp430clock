
#include <msp430.h>

//for ISR support
#include <legacymsp430.h>
#include <isr_compat.h>

#include "lcd.h"


// ---------------------------- Globals -----------------------------

char timeString[20] = { 0 };
//                                  S  M  H  D  D  M    Y
unsigned char defaultrtcdata[14] = {0, 0, 0, 5, 1, 12,  0};
unsigned char rtcdata[14] = { 0 };
static volatile int repaintTime = 0;
// ---------------------------- End Globals -------------------------

#define RTC_SEC 0x00
#define RTC_MIN 0x01
#define RTC_HOUR 0x02
#define RTC_DOW 0x03
#define RTC_DATE 0x04
#define RTC_MONTH 0x05
#define RTC_YEAR 0x06

#define     LED_ON          P4OUT |= BIT7
#define     LED_OFF         P4OUT &= ~BIT7
#define     LED_TOGGLE      P4OUT ^= BIT7

#define     BUT1            (P1IN & BIT0)
#define     BUT2            (P2IN & BIT0)



static void setupTime() {
  RTCCTL01 |= RTCHOLD;

  RTCSEC = defaultrtcdata[RTC_SEC];
  RTCMIN = defaultrtcdata[RTC_MIN];
  RTCHOUR = defaultrtcdata[RTC_HOUR];
  RTCDOW = defaultrtcdata[RTC_DOW];
  RTCDAY = defaultrtcdata[RTC_DATE];
  RTCMON = defaultrtcdata[RTC_MONTH];
  RTCYEARL = 0b11011100; // 2012
  RTCYEARH = 0b111;

  RTCCTL01 ^= RTCHOLD;
}


// call readtime instead of this
static void updateTimeString() {
  static unsigned char lastMin;
  static unsigned char lastHour;
  int hour, min, showhour;
  if (lastMin != rtcdata[RTC_MIN]) {
    repaintTime = 1;
    lastMin = rtcdata[RTC_MIN];
  }
  // if the user changes the time
  if (lastHour != rtcdata[RTC_HOUR]) {
    repaintTime = 1;
    lastHour = rtcdata[RTC_HOUR];
  }

  hour = rtcdata[RTC_HOUR];
  min = rtcdata[RTC_MIN];
  showhour = hour;

  if (hour > 12) {
    showhour -= 12;
  }

  timeString[0] = (showhour/10) + '0';
  if (timeString[0] == '0') {
    timeString[0] = ' ';
  }
  timeString[1] = (showhour%10) + '0';
  if (hour == 0) {
    timeString[0] = '1';
    timeString[1] = '2';
  }


  timeString[2] = ':';
  timeString[3] = (min/10) + '0';
  timeString[4] = (min%10) + '0';
  timeString[5] = ' ';
  if (hour >= 12) {
    timeString[6] = 'P';
  } else {
    timeString[6] = 'A';
  }
  timeString[7] = 'M';

}



static void readTime() {
  // read seconds, mins, etc, then update the time string which goes to LCD
  rtcdata[RTC_SEC] = RTCSEC;
  rtcdata[RTC_MIN] = RTCMIN;
  rtcdata[RTC_HOUR] = RTCHOUR;
  rtcdata[RTC_DOW] = RTCDOW;
  rtcdata[RTC_DATE] = RTCDAY;
  rtcdata[RTC_MONTH] = RTCMON;
  rtcdata[RTC_YEAR] = RTCYEARL;

  updateTimeString();
}

static void incrementHour() {
#ifdef USE_BCD
  int rtchour = RTCHOUR;
  int ihour = (rtchour & 0xf) + ((rtchour & 0x30)>>4)*10;
  ihour = (ihour+1)%24;
  RTCHOUR = ((ihour / 10) << 4) | (ihour % 10);
#else
  int rtchour = RTCHOUR;
  RTCHOUR = (rtchour + 1) % 24;
#endif  
    readTime();
}

static void incrementMinute() {
#ifdef USE_BCD
  int rtcmin = RTCMIN;
  int imin = (rtcmin & 0xf) + ((rtcmin & 0x70)>>4)*10;
  imin = (imin+1)%60;
  RTCMIN = ((imin / 10) << 4) | (imin % 10);
#else
  int rtcmin = RTCMIN;
  RTCMIN = (rtcmin + 1) % 60;
  RTCSEC = 0;
#endif
  readTime();
}

static void init32khz() {
  TA0CTL = TASSEL_1 + MC_1 /*+ ID_3*/ + TACLR;         // ACLK/8
  //TA0CTL = TASSEL_2 + MC_1 + TACLR;         // SMCLK
  TA0CCR0 = 32768;
  TA0CCTL0 = CCIE;                          // CCR0 interrupt enabled
}

static void initrtc() {
  RTCCTL01 = RTCRDYIE | RTCMODE;
  //RT0PSIE = 1; // generate interrupt intervals according to RT0IP

  setupTime();
}


static void setupWDT() {
  //BCSCTL1 |= DIVA_1;                        // ACLK/2
  //BCSCTL3 |= LFXT1S_2;                      // ACLK = VLO
  #if 0
  WDTCTL = WDT_ADLY_1000;                   // Interval timer
  SFRIE1 |= WDTIE;
  //IE1 |= WDTIE;                             // Enable WDT interrupt
#endif
}

static void initports() {
    //Initialization of ports (all unused pins as outputs with low-level
    P1REN |= BIT0;                //Enable BUT1 pullup
    P1OUT = 0x01;
    P1DIR = 0xFE;                 //LCD pins are outputs
    P2REN |= BIT0;                //Enable BUT2 pullup
    P2OUT = 0x01;
    P2DIR = 0x00;
    P4OUT = 0x00;
    P4DIR = 0x80;                 //LED oin is output    
    P5DS  = 0x02;                 //Increase drive strainght of P5.1(LCD-PWR)
    P5OUT = 0x02;                 //LCD_PWR_E, Battery measurement enable
    P5DIR = 0x03;
    P6OUT = 0x00;                 
    P6DIR = 0x02;                 //STNBY_E    
}


void enterLPM(void)
{
#if 0
    P1OUT = 0x00;
    P1DIR = 0xFE;                 //LCD pins are outputs
    
    P2OUT = 0x00;
    P2DIR = 0x00;
    
    P4OUT = 0x00;                 //0b00000000
    P4DIR = 0xFF;                 //0b

    P5OUT = 0x00;                 //LCD_PWR_DIS
    P5DIR = 0xFF;

    P6SEL = 0x01;
    P6OUT = 0x00;                 
    P6DIR = 0xFC;                 //STNBY_E    
    
    PJOUT = 0x00;
    PJDIR = 0xFF;
    
    REFCTL0 = 0;                        //turn off internal VREF
#endif
    //    __bis_SR_register(LPM3_bits + GIE);       // Enter LPM3 
    __bis_SR_register(LPM3_bits + GIE);       // Enter LPM3 
    __no_operation();
        
}



static void initclocks()
{


     // Configure XT1
  P5DIR &= ~(BIT5 + BIT4);
  P5SEL |= BIT4+BIT5;                       // Port select XT1


  UCSCTL6 &= ~(XT1OFF);                     // XT1 On
  UCSCTL6 |= XCAP_3;                        // Internal load cap
  UCSCTL3 = 0;                              // FLL Reference Clock = XT1

  // Loop until XT1,XT2 & DCO stabilizes - In this case loop until XT1 and DCo settle
  do
  {
    UCSCTL7 &= ~(XT2OFFG + XT1LFOFFG + DCOFFG);
                                            // Clear XT2,XT1,DCO fault flags
    SFRIFG1 &= ~OFIFG;                      // Clear fault flags
  }while (SFRIFG1&OFIFG);                   // Test oscillator fault flag  
  UCSCTL6 &= ~(XT1DRIVE_3);                 // Xtal is now stable, reduce drive strength

#ifndef USE_VLO
  UCSCTL4 |= SELA_0;                        // ACLK = LFTX1 (by default)
#else
  UCSCTL4 |= SELA_1; // VLO sources ACLK
#endif

#if 0
  // Disable SVS
  PMMCTL0_H = PMMPW_H;                // PMM Password
  SVSMHCTL &= ~(SVMHE+SVSHE);         // Disable High side SVS 
  SVSMLCTL &= ~(SVMLE+SVSLE);         // Disable Low side SVS
#endif


}


int main(void) {

  WDTCTL = WDTPW + WDTHOLD; // Stop watchdog timer
  P1DIR |= 0x01;                            // P1.0 output
  P4DIR= 0x80;
  LED_OFF;

  initports();
  initclocks();
  init32khz();
  initrtc();


  readTime();
  LED_ON;

  LCD_Init();
  LED_OFF;
  //Delay(10000);
  LCD_Send_STR(1, "  TIMMY "); 

  setupWDT();

  repaintTime = 1;
  for (;;) {
    enterLPM();
    if (BUT1 == 0) {
      incrementHour();
      LED_TOGGLE;
    }
    if (BUT2 == 0) {
      incrementMinute();
      LED_TOGGLE;
    }
    if (repaintTime) {
      LCD_Send_STR(1, &timeString[0]); 
      repaintTime = 0;
    }
  }
  return 0;

} 



ISR(RTC, RtcInt) {
  int rtciv;
  //  if (RTCIV == 2) { //RTCRDIFG

  rtciv = RTCIV;
  if (rtciv == RTC_NONE) return;
  if (rtciv | RTC_RTCRDYIFG) {
    readTime();
  } else {
    timeString[0] = 'N';
    timeString[1] = 'R';
  }

}

// Timer A0 interrupt service routine

ISR(TIMER0_A0, ISRTimerA) {
  _BIC_SR_IRQ(LPM3_bits);
  //  RTCCTL0 = RTCRDYIFG;

}


ISR(WDT, WDTISR) {
  initports();
  _BIC_SR_IRQ(LPM3_bits);                   // Clear LPM3 bits from 0(SR)
  //  LED_TOGGLE;
}

ISR(TIMER1_A0, TIMER1_A0_ISR)
{
  //LED_TOGGLE;
}
