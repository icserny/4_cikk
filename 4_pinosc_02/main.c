
/*********************************************************************
 *  Launchpad demóprogram: 4_pinosc_02
 *
 * Egyszerû 4 gombos kapacitív érzékelõ az I/O portokba beépített
 * Pin Oscillator felhasználásával. A PinOsc jelét Timer_A0 INTOSC
 * bemenetére vezetjük, s a WDT periodikus megszakításait használjuk
 * a számlálási periódusok idõzítésére. A számlálásban mutatatkozó
 * változások jelzik a kapacitív nyomógombok megérintését.
 *
 * A gomb érintések észlelését a LED-ek segítségével jelezzük:
 *  Bemenet 1: LED1 világít (LED2 ki)
 *  Bemenet 2: LED2 világít (LED1 ki)
 *  Bemenet 3: Mindkét LED világít
 *  Bemenet 4: Mindkét LED villog
 *
 *  ACLK = VLO = 12kHz, MCLK = SMCLK = 1MHz DCO
 *
 * Hardver követelmények:
 *  - Launchpad MSP430G2xx2 vagy MSP430G2xx3 mikrovezérlõvel
 *  - Vegyük le az RXD,TXD átkötéseket! 
 *
 *               MSP430G2xx2
 *               MSP430G2xx3 
 *             -----------------
 *         /|\|              XIN|-
 *          | |                 | 
 *          --|RST          XOUT|-
 *            |                 |
 *            |             P1.1|<--Kapacitív érzékelõ Bemenet 1
 *            |                 |
 *  LED 2  <--|P1.6         P1.2|<--Kapacitív érzékelõ Bemenet 2
 *            |                 |
 *  LED 1  <--|P1.0         P1.4|<--Kapacitív érzékelõ Bemenet 3
 *            |                 |
 *            |             P1.5|<--Kapacitív érzékelõ Bemenet 4
 *
 *  Szerzõk: Brandon Elliott/D. Dang
 *  Texas Instruments Inc.
 *  November 2010
 *
 *  Módosítás és magyar fordítás: I. Cserny
 *  MTA ATOMKI, Debrecen
 *  Nov 2012
 *  IAR Embedded Workbench for MSP430 v5.51
 **********************************************************************/

#include <io430.h>
#include <stdint.h>

//-- Konfigurációs paraméterek -----------------
//-- WDT SMCLK intervallum a szenzorok méréséhez
#define WDT_meas_setting (DIV_SMCLK_512)
//-- WDT ACLK intervallum a mérések közötti késleltetéshez
#define WDT_delay_setting (DIV_ACLK_512)

/* Senzor beállítások */
#define NUM_SEN     4                       // Definiáljuk a szenzorok számát
// Küszöbérték az érintés érzékeléséhez. Legyen kb. a várható vátozás fele
#define KEY_LVL     220                     

/* Makró definíciók WDT osztójának beállításához */
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

//-- Globális változók az érzékeléshez
uint16_t base_cnt[NUM_SEN];
uint16_t meas_cnt[NUM_SEN];
int16_t delta_cnt[NUM_SEN];
uint8_t key_press[NUM_SEN];
char key_pressed;
int16_t cycles;
const uint8_t electrode_bit[NUM_SEN]={BIT1, BIT2, BIT4, BIT5};
//-- Függvény prototípusok */
void measure_count(void);                   // Mérés a kapitív szenzorokon
void pulse_LED(void);                       // A LED-ek vezérlése

//-- A fõprogram -----------------------------
void main(void) { 
  unsigned int i,j;
  WDTCTL = WDTPW + WDTHOLD;                 // Letiltjuk a watchdog idõzítõt
  DCOCTL = CALDCO_1MHZ;                     // DCO beállítása a gyárilag kalibrált 
  BCSCTL1 = CALBC1_1MHZ;                    // 1 MHz-es frekvenciára  
  BCSCTL3 |= LFXT1S_2;                      // LFXT1 = VLO bekapcsolása
  IE1 |= WDTIE;                             // WDT interrupt engedélyezése
  P2SEL = 0x00;                             // Nincs kristály P2.6/P2.7-en   
  P1DIR = LED_1 + LED_2;                    // P1.0 & P1.6 legyen kimenet
//--- P1.3 belsõ felhúzás engedélyezése -----------------------  
  P1DIR &= ~BIT3;                           // P1.3 legyen digitális bemenet
  P1OUT |= BIT3;                            // Felfelé húzzuk, nem lefelé  
  P1REN |= BIT3;                            // Belsõ felhúzás engedélyezése                            
   __enable_interrupt();                    // Megszakítások általános engedélyezése

  measure_count();                          // Az alapállapoti kapacitások meghatározása
  for (i = 0; i<NUM_SEN; i++)
    base_cnt[i] = meas_cnt[i];

  for(i=15; i>0; i--)                       // Az alapállapot ismételt meghatározása és átlagolás
  { 
    measure_count();
    for (j = 0; j<NUM_SEN; j++)
      base_cnt[j] = (meas_cnt[j]+base_cnt[j])/2;
  }
  
//-- A fõ programhurok itt kezdõdik ----------
  while (1)   {
    j = KEY_LVL;
    key_pressed = 0;                        // Feltételezzük, hogy nincs gomblenyomás
    measure_count();                        // Mérés minden szenzorra
    for (i = 0; i<NUM_SEN; i++) { 
      delta_cnt[i] = base_cnt[i] - meas_cnt[i];  // delta_cnt: az alapállapottól való eltérés

//-- Szükség esetén kezeljük az alapállapoti kapacitás csökkenésének esetét az alapvonal módosításával  
      if (delta_cnt[i] < 0)                 // Ha negatív az eredmény, az alapvonal
      {                                     // alá mentünk, azaz csökkent a kapacitás
          base_cnt[i] = (base_cnt[i]+meas_cnt[i]) >> 1; // gyors újraátlagolás
          delta_cnt[i] = 0;                 // Kinullázás a nyomógomb állapot megállapításához
      }
      if (delta_cnt[i] > j)                 // Annak eldontése, hogy az i-edik gomb le van-e nyomva 
      {                                     // az elõre beállított küszöbhöz történõ hasonlítással
        key_press[i] = 1;                   // A benyomott állapot jelzése
        j = delta_cnt[i];                   // Emeljük a küszöböt: a következõ gombnak ezt kell meghaladnia
        key_pressed = i+1;                  // A legnagyob változású gomb sorszámának rögzítése 
      }
      else
        key_press[i] = 0;                   // Az adott gomb nem számít benyomottnak... 
    }

    /* Várakozás a következõ mintavételig. Hosszabban vár, ha nem volt gomblenyomás */
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

    /* Alapvonal korrekció, ha megnõtt a kapacitás */
    if (!key_pressed)                         // Csak akkor csökkentjük az alapvonalat, 
    {                                         // ha nem történt gomblenyomás
      for (i = 0; i<NUM_SEN; i++)
        base_cnt[i] = base_cnt[i] - 1;        // Lassan csökkentjük az alapvonalat
    }                                         // a kapacitás növekedése esetén
     pulse_LED(); 
   
    __low_power_mode_3();
  }
}                                             // Fõprogram vége

/* A kapacitás mérése számlálással, minden szenzorra */
/* A függvény a NUM_SEN értékétõl függetlenül négy szenzorra van felkészítve! */

void measure_count(void)
{ 
  char i;
                     
  TA0CTL = TASSEL_3+MC_2;                   // TACLK, szabadonfutó mód
  TA0CCTL1 = CM_3+CCIS_2+CAP;               // Positív és negatív élre,GND a bemenetre,Capture

  for (i = 0; i<NUM_SEN; i++) {
//-- Relaxációs oszcillátor módba kapcsoljuk a soron következõ portlábat
//-- A P2SEL2 regiszter beállítása PinOsc módba kapcsolja a portlábat, melynek
//-- jelét Timer0 Intosc bemenetén észleljük  
    P1DIR &= ~ electrode_bit[i];
    P1SEL &= ~ electrode_bit[i];
    P1SEL2 |= electrode_bit[i];   
//-- Az adatgyûjtési idõ beállítása 
    WDTCTL = WDT_meas_setting;              // WDT, ACLK, intervallum mód
    TA0CTL |= TACLR;                        // Timer_A TAR regiszterének törlése
    __low_power_mode_0();                   // Interrupt és LPM0 engedélyezése 
    meas_cnt[i] = TACCR1;                   // Elmentjük az eredményt
    WDTCTL = WDTPW + WDTHOLD;               // A WDT leállítása
    P1SEL2 &= ~electrode_bit[i];            // Visszakapcsolás digitális I/O módba
  }
}

/* A LED-ek állapotának aktualizálása */
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

/* Watchdog Timer interrupt kiszolgálása */
#pragma vector=WDT_VECTOR
__interrupt void watchdog_timer(void) {
  TA0CCTL1 ^= CCIS0;                        // Szofveres capture indítás
  __low_power_mode_off_on_exit();           // A fõprogram felébresztése
}
