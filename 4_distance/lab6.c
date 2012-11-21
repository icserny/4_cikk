
#include "io430.h"
/*********************************************************************
 * Launchpad demóprogram: 4_distance
 *
 * Távolságmérés egy HC-SR04 ultrahangos szenzor segítségével.
 * A szenzor trigger bemenetére egy 10 us hosszúságú impulzust kell kiadni.
 * A válaszul kapott impulzus szélessége a visszhang késési idejét adja meg.
 * A hang terjedési sebességének ismeretében (~340 m/s) a távolság kiszámítható.  
 * 
 * Hardver követelmények:
 *  - HC-SR04 ultrahangos szenzor 
 *  - Launchpad MSP430G2xxx mikrovezérlõvel
 *  - Az RXD átkötést el kell távolítani
 *  - Az újabb kiadású (v1.5) kártyán a TXD átkötést 
 *    SW állásba kell helyezni (a többi átkötéssel párhuzamosan)
 *
 *
 *                MSP430G2xxx                      HC-SR04  
 *             -----------------                -----------------
 *        /|\ |              XIN|-      +5V -->| VCC             |
 *         |  |                 |              |                 |   
 *          --|RST          XOUT|-      GND--->| GND             |  
 *            |                 |              |                 |         
 *    LED1 <--|P1.0         P1.5|------------->| TRIG            |
 *            |                 |      R1      |                 |
 *    LED2 <--|P1.6         P1.2|<---/\/\/\/---| ECHO            |
 *                                    1k
 *   
 *  I. Cserny
 *  MTA ATOMKI, Debrecen
 *  Nov 2012
 *  Fejlesztõi környezet: IAR Embedded Workbench for MSP430 v5.51 
 *********************************************************************/
#include "io430.h"
#include "stdint.h"

//-- Hardver absztrakció -------------------------------------
#define LED1      BIT0            //Piros LED a P1.0 lábon
#define TXD       BIT1            //TXD a P1.1 lábon
#define ECHO      ________        //Echo jel a HC-SR04-bõl (CCI1A bemenet)
#define SW2       BIT3            //Nyomógomb a P1.3 lábon
#define TRIGGER   ________        //Trigger kimenet HC-SR04-hez
#define LED2      BIT6            //Zöld LED a P1.6 lábon 

#define FCPU      1000000         //CPU frekvencia Hz-ben 
#define FCPU_MHZ  1               //CPU frekvencia MHz-ben
#define TEN_US    FCPU/100000     //késleltetési ciklusok 10 us-hez

uint16_t last_capture;
uint16_t this_capture;
volatile uint16_t delta;
int32_t pulse_width,distance;

/**------------------------------------------------------------
 *   Egy karakter kiküldése szoftveresen a soros portra
 *   SW_UART: 9600 bit/s, 8, N, 2 formátum
 *   DCO = 1 MHz (Bitidõ = 104.167 usec)
 *-------------------------------------------------------------
 * c - a kiküldeni kívánt karakter kódja
 */
void sw_uart_putc(char c) {
  uint8_t i;
  uint16_t TXData;                //Adatregiszter (11 bites)
  TXData = (uint16_t)c | 0x300;   //2 Stop bit (mark)
  TXData = TXData << 1;           //Start bit (space)
  for(i=0; i<11; i++) {           //11 bitet küldünk ki
    if(TXData & 0x0001) {         //Soron következõ bit vizsgálata
      P1OUT |= TXD;               //Ha '1'
    } else {
      P1OUT &= ~TXD;              //Ha '0'
    }
    TXData = TXData >> 1;         //Adatregiszter léptetés jobbra
    __delay_cycles(89);           //<== Itt kell hangolni!
  }
  P1OUT |= TXD;                   //Az alaphelyzet: mark
}

/**------------------------------------------------------------
 *  Karakterfüzér kiírása a soros portra
 *-------------------------------------------------------------
 * p_str - karakterfüzér mutató (nullával lezárt stringre mutat)
 */
void sw_uart_puts(char* p_str) {
  char c;
  while ((c=*p_str)) {
      sw_uart_putc(c);
      p_str++;
  }
}

/**------------------------------------------------------------
 * Decimális kiíratás adott számú tizedesjegyre.
 *-------------------------------------------------------------
 * data - a kiírandó elõjeles szám (int32_t)
 * ndigits - a kiírandó tizedesek száma (uint8_t)
 */
void sw_uart_outdec(int32_t data, uint8_t ndigits) {
  static char sign, s[12];
  int8_t i;
  i=0; sign=' ';
  if(data<0) { sign='-'; data = -data;}
    do {
      s[i]=data%10 + '0';
      data=data/10;
      i++;
      if(i==ndigits) {s[i]='.'; i++;}
    } while(data>0 || i<(ndigits+2));
    sw_uart_putc(sign);
    do {
      sw_uart_putc(s[--i]);
    } while(i);
}

void main(void) {
  WDTCTL = WDTPW + WDTHOLD;       //Letiltjuk a watchdog idõzítõt
  DCOCTL = CALDCO_1MHZ;           //DCO beállítása a gyárilag kalibrált 
  BCSCTL1 = CALBC1_1MHZ;          //1 MHz-es frekvenciára  
//-- LED-ek konfigurálása -------------------------------------  
  P1OUT &= ~(LED1+LED2);          //Kezdetben nem világítanak  
  P1DIR |= LED1+LED2;             //LED1 és LED2 digitális kimenet
//-- Szoftveres UART kimenet konfigurálása --------------------    
  P1OUT |= TXD;                   //TXD alaphelyzete: mark
  P1DIR |= TXD;                   //TXD legyen digitális kimenet
//--- SW2 konfigurálás, belsõ felhúzás engedélyezése ---------- 
  P1DIR &= ~SW2;                  //P1.3 legyen digitális bemenet
  P1OUT |= SW2;                   //Felfelé húzzuk, nem lefelé  
  P1REN |= SW2;                   //Belsõ felhúzás engedélyezése   
//-- HC-SR04 vezérlés konfigurálása ---------------------------
  P1OUT &= ~TRIGGER;              //Kezdetben legyen alacsony  
  P1DIR |= TRIGGER;               //Trigger kimenet HC-SR04 számára
  P1DIR &= ~ECHO;                 //ECHO bemenet (biztonság kedvéért felhúzzuk)
  P1OUT |= ECHO;                  //Felfelé húzzuk, nem lefelé  
  P1REN |= ECHO;                  //Belsõ felhúzás engedélyezése   
  P1SEL ______________            //TA0CCI1A engedélyezése
  P1SEL2 &= ECHO;                 //a P1.2 bemeneten (ECHO)
  while(1) {
    delta = 0;                    //Nullázzuk a munkaváltozót
//-- Timer0 és TA0CCR1 konfigurálása ---------------------------
    TA0CTL = ________ |           //Timer_A forrása SMCLK legyen
             ________ |           //Bemeneti osztó = 1:1
             ________ |           //Folyamatos számlálás mód
                 TACLR;           //A számlálót töröljük    
   TA0CCR1 = 0;                   //CCR1 adatregiszterét töröljük
   TA0CCTL1 =  ______ |           //Felfutó élre érzékeny 
               ______ |           //TA0CCI1A bemenet kiválasztása
                  SCS |           //Szinkronizált capture
                  CAP |           //Capture mód
                  CCIE;           //CCR1 megszakítás engedélyezése     
//-- Trigger impulzussal elindítjuk a mérést           ____________
    P1OUT |= TRIGGER;             //Trigger impulzus _| min. 10us  |___      
    __delay_cycles(TEN_US);
    P1OUT &= ~TRIGGER;
    WDTCTL = WDT_MRST_32;              //"Nem várok holnapig...."  
    ____________________;              //Várunk a Capture ciklus lezajlására
    if((delta>1000) && (delta<5000)) {
      P1OUT |= LED2;                   //Zöld LED: tartományon belül  
    }  
    else {
      P1OUT |= LED1;                   //Piros LED: tartományon kívül
    }     
      pulse_width = delta/FCPU_MHZ;    //idõtartam mikroszekundumokban
      distance = (delta*17L)/(FCPU_MHZ*100L);  //távolság milliméterekben
      sw_uart_puts("Pulse width = ");
      sw_uart_outdec(pulse_width,3);
      sw_uart_puts(" ms   Distance = ");
      sw_uart_outdec(distance,1);
      sw_uart_puts(" cm\r\n");
      __delay_cycles(FCPU);            //2 s várakozás
      P1OUT &= ~(LED1+LED2);           //Közben lekapcsoljuk a LED-eket
      __delay_cycles(FCPU);    
  }
}

//--- TACCR1 megszakítás kiszolgálása ---------------------------------
#pragma vector=TIMER0_A1_VECTOR
__interrupt void ta1_isr(void) {
  ______________________;              //TA0CCR1 megszakítási jelzõbit törlése
  if(TA0CCTL1&CCI) {
    last_capture = TA0CCR1;            //A felfutó élhez tartozó idõ rögzítése
     TA0CCTL1 ____________;            //Lefutó élre érzékenyítünk
  } 
  else {
    TA0CCTL1 &= __________;            //Capture ciklus vége
    this_capture = TA0CCR1;            //A lefutó élhez tartozó idõ rögzítése
    delta = this_capture - last_capture; 
      WDTCTL = ___________;            //Letiltjuk a watchdog idõzítõt
    __low_power_mode_off_on_exit();    //Felébresztjük a fõprogramot
  }
}  
       