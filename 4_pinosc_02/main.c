
/*********************************************************************
 *  Launchpad dem�program: 4_pinosc_02
 *
 * Egyszer� 4 gombos kapacit�v �rz�kel� az I/O portokba be�p�tett
 * Pin Oscillator felhaszn�l�s�val. A PinOsc jel�t Timer_A0 INTOSC
 * bemenet�re vezetj�k, s a WDT periodikus megszak�t�sait haszn�ljuk
 * a sz�ml�l�si peri�dusok id�z�t�s�re. A sz�ml�l�sban mutatatkoz�
 * v�ltoz�sok jelzik a kapacit�v nyom�gombok meg�rint�s�t.
 *
 * A gomb �rint�sek �szlel�s�t a LED-ek seg�ts�g�vel jelezz�k:
 *  Bemenet 1: LED1 vil�g�t (LED2 ki)
 *  Bemenet 2: LED2 vil�g�t (LED1 ki)
 *  Bemenet 3: Mindk�t LED vil�g�t
 *  Bemenet 4: Mindk�t LED villog
 *
 *  ACLK = VLO = 12kHz, MCLK = SMCLK = 1MHz DCO
 *
 * Hardver k�vetelm�nyek:
 *  - Launchpad MSP430G2xx2 vagy MSP430G2xx3 mikrovez�rl�vel
 *  - Vegy�k le az RXD,TXD �tk�t�seket! 
 *
 *               MSP430G2xx2
 *               MSP430G2xx3 
 *             -----------------
 *         /|\|              XIN|-
 *          | |                 | 
 *          --|RST          XOUT|-
 *            |                 |
 *            |             P1.1|<--Kapacit�v �rz�kel� Bemenet 1
 *            |                 |
 *  LED 2  <--|P1.6         P1.2|<--Kapacit�v �rz�kel� Bemenet 2
 *            |                 |
 *  LED 1  <--|P1.0         P1.4|<--Kapacit�v �rz�kel� Bemenet 3
 *            |                 |
 *            |             P1.5|<--Kapacit�v �rz�kel� Bemenet 4
 *
 *  Szerz�k: Brandon Elliott/D. Dang
 *  Texas Instruments Inc.
 *  November 2010
 *
 *  M�dos�t�s �s magyar ford�t�s: I. Cserny
 *  MTA ATOMKI, Debrecen
 *  Nov 2012
 *  IAR Embedded Workbench for MSP430 v5.51
 **********************************************************************/

#include <io430.h>
#include <stdint.h>

//-- Konfigur�ci�s param�terek -----------------
//-- WDT SMCLK intervallum a szenzorok m�r�s�hez
#define WDT_meas_setting (DIV_SMCLK_512)
//-- WDT ACLK intervallum a m�r�sek k�z�tti k�sleltet�shez
#define WDT_delay_setting (DIV_ACLK_512)

/* Senzor be�ll�t�sok */
#define NUM_SEN     4                       // Defini�ljuk a szenzorok sz�m�t
// K�sz�b�rt�k az �rint�s �rz�kel�s�hez. Legyen kb. a v�rhat� v�toz�s fele
#define KEY_LVL     220                     

/* Makr� defin�ci�k WDT oszt�j�nak be�ll�t�s�hoz */
#define DIV_ACLK_32768  (WDT_ADLY_1000)     // ACLK/32768
#define DIV_ACLK_8192   (WDT_ADLY_250)      // ACLK/8192 
#define DIV_ACLK_512    (WDT_ADLY_16)       // ACLK/512 
#define DIV_ACLK_64     (WDT_ADLY_1_9)      // ACLK/64 
#define DIV_SMCLK_32768 (WDT_MDLY_32)       // SMCLK/32768
#define DIV_SMCLK_8192  (WDT_MDLY_8)        // SMCLK/8192 
#define DIV_SMCLK_512   (WDT_MDLY_0_5)      // SMCLK/512 
#define DIV_SMCLK_64    (WDT_MDLY_0_064)    // SMCLK/64 

#define LED_1   (0x01)                      // P1.0 LED kimenet
#define LED_2   (0x40)                      // P1.6 LED kimenet

//-- Glob�lis v�ltoz�k az �rz�kel�shez
uint16_t base_cnt[NUM_SEN];
uint16_t meas_cnt[NUM_SEN];
int16_t delta_cnt[NUM_SEN];
uint8_t key_press[NUM_SEN];
char key_pressed;
int16_t cycles;
const uint8_t electrode_bit[NUM_SEN]={BIT1, BIT2, BIT4, BIT5};
//-- F�ggv�ny protot�pusok */
void measure_count(void);                   // M�r�s a kapit�v szenzorokon
void pulse_LED(void);                       // A LED-ek vez�rl�se

//-- A f�program -----------------------------
void main(void) { 
  unsigned int i,j;
  WDTCTL = WDTPW + WDTHOLD;                 // Letiltjuk a watchdog id�z�t�t
  DCOCTL = CALDCO_1MHZ;                     // DCO be�ll�t�sa a gy�rilag kalibr�lt 
  BCSCTL1 = CALBC1_1MHZ;                    // 1 MHz-es frekvenci�ra  
  BCSCTL3 |= LFXT1S_2;                      // LFXT1 = VLO bekapcsol�sa
  IE1 |= WDTIE;                             // WDT interrupt enged�lyez�se
  P2SEL = 0x00;                             // Nincs krist�ly P2.6/P2.7-en   
  P1DIR = LED_1 + LED_2;                    // P1.0 & P1.6 legyen kimenet
//--- P1.3 bels� felh�z�s enged�lyez�se -----------------------  
  P1DIR &= ~BIT3;                           // P1.3 legyen digit�lis bemenet
  P1OUT |= BIT3;                            // Felfel� h�zzuk, nem lefel�  
  P1REN |= BIT3;                            // Bels� felh�z�s enged�lyez�se                            
   __enable_interrupt();                    // Megszak�t�sok �ltal�nos enged�lyez�se

  measure_count();                          // Az alap�llapoti kapacit�sok meghat�roz�sa
  for (i = 0; i<NUM_SEN; i++)
    base_cnt[i] = meas_cnt[i];

  for(i=15; i>0; i--)                       // Az alap�llapot ism�telt meghat�roz�sa �s �tlagol�s
  { 
    measure_count();
    for (j = 0; j<NUM_SEN; j++)
      base_cnt[j] = (meas_cnt[j]+base_cnt[j])/2;
  }
  
//-- A f� programhurok itt kezd�dik ----------
  while (1)   {
    j = KEY_LVL;
    key_pressed = 0;                        // Felt�telezz�k, hogy nincs gomblenyom�s
    measure_count();                        // M�r�s minden szenzorra
    for (i = 0; i<NUM_SEN; i++) { 
      delta_cnt[i] = base_cnt[i] - meas_cnt[i];  // delta_cnt: az alap�llapott�l val� elt�r�s

//-- Sz�ks�g eset�n kezelj�k az alap�llapoti kapacit�s cs�kken�s�nek eset�t az alapvonal m�dos�t�s�val  
      if (delta_cnt[i] < 0)                 // Ha negat�v az eredm�ny, az alapvonal
      {                                     // al� ment�nk, azaz cs�kkent a kapacit�s
          base_cnt[i] = (base_cnt[i]+meas_cnt[i]) >> 1; // gyors �jra�tlagol�s
          delta_cnt[i] = 0;                 // Kinull�z�s a nyom�gomb �llapot meg�llap�t�s�hoz
      }
      if (delta_cnt[i] > j)                 // Annak eldont�se, hogy az i-edik gomb le van-e nyomva 
      {                                     // az el�re be�ll�tott k�sz�bh�z t�rt�n� hasonl�t�ssal
        key_press[i] = 1;                   // A benyomott �llapot jelz�se
        j = delta_cnt[i];                   // Emelj�k a k�sz�b�t: a k�vetkez� gombnak ezt kell meghaladnia
        key_pressed = i+1;                  // A legnagyob v�ltoz�s� gomb sorsz�m�nak r�gz�t�se 
      }
      else
        key_press[i] = 0;                   // Az adott gomb nem sz�m�t benyomottnak... 
    }

    /* V�rakoz�s a k�vetkez� mintav�telig. Hosszabban v�r, ha nem volt gomblenyom�s */
    if (key_pressed)
    {
      BCSCTL1 = (BCSCTL1 & 0x0CF) + DIVA_0; // ACLK/(0:1,1:2,2:4,3:8)
      cycles = 20;
    }
    else
    {
      cycles--;
      if (cycles > 0)
        BCSCTL1 = (BCSCTL1 & 0x0CF) + DIVA_0; // ACLK/1)
      else
      {
        BCSCTL1 = (BCSCTL1 & 0x0CF) + DIVA_3; // ACLK/8)
        cycles = 0;
      }
    }
    WDTCTL = WDT_delay_setting;               // WDT, ACLK, interval timer

    /* Alapvonal korrekci�, ha megn�tt a kapacit�s */
    if (!key_pressed)                         // Csak akkor cs�kkentj�k az alapvonalat, 
    {                                         // ha nem t�rt�nt gomblenyom�s
      for (i = 0; i<NUM_SEN; i++)
        base_cnt[i] = base_cnt[i] - 1;        // Lassan cs�kkentj�k az alapvonalat
    }                                         // a kapacit�s n�veked�se eset�n
     pulse_LED(); 
   
    __low_power_mode_3();
  }
}                                             // F�program v�ge

/* A kapacit�s m�r�se sz�ml�l�ssal, minden szenzorra */
/* A f�ggv�ny a NUM_SEN �rt�k�t�l f�ggetlen�l n�gy szenzorra van felk�sz�tve! */

void measure_count(void)
{ 
  char i;
                     
  TA0CTL = TASSEL_3+MC_2;                   // TACLK, szabadonfut� m�d
  TA0CCTL1 = CM_3+CCIS_2+CAP;               // Posit�v �s negat�v �lre,GND a bemenetre,Capture

  for (i = 0; i<NUM_SEN; i++) {
//-- Relax�ci�s oszcill�tor m�dba kapcsoljuk a soron k�vetkez� portl�bat
//-- A P2SEL2 regiszter be�ll�t�sa PinOsc m�dba kapcsolja a portl�bat, melynek
//-- jel�t Timer0 Intosc bemenet�n �szlelj�k  
    P1DIR &= ~ electrode_bit[i];
    P1SEL &= ~ electrode_bit[i];
    P1SEL2 |= electrode_bit[i];   
//-- Az adatgy�jt�si id� be�ll�t�sa 
    WDTCTL = WDT_meas_setting;              // WDT, ACLK, intervallum m�d
    TA0CTL |= TACLR;                        // Timer_A TAR regiszter�nek t�rl�se
    __low_power_mode_0();                   // Interrupt �s LPM0 enged�lyez�se 
    meas_cnt[i] = TACCR1;                   // Elmentj�k az eredm�nyt
    WDTCTL = WDTPW + WDTHOLD;               // A WDT le�ll�t�sa
    P1SEL2 &= ~electrode_bit[i];            // Visszakapcsol�s digit�lis I/O m�dba
  }
}

/* A LED-ek �llapot�nak aktualiz�l�sa */
void pulse_LED(void) { 
  switch (key_pressed){
  case 0: P1OUT &= ~(LED_1 + LED_2);
          break;
  case 1: P1OUT = LED_1;
          break;
  case 2: P1OUT = LED_2;
          break;
  case 3: P1OUT = LED_1 + LED_2;
          break;
  case 4: P1OUT ^= LED_1 + LED_2;
          break;
          
    }
}

/* Watchdog Timer interrupt kiszolg�l�sa */
#pragma vector=WDT_VECTOR
__interrupt void watchdog_timer(void) {
  TA0CCTL1 ^= CCIS0;                        // Szofveres capture ind�t�s
  __low_power_mode_off_on_exit();           // A f�program fel�breszt�se
}
