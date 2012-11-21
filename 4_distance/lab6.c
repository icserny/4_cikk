
#include "io430.h"
/*********************************************************************
 * Launchpad dem�program: 4_distance
 *
 * T�vols�gm�r�s egy HC-SR04 ultrahangos szenzor seg�ts�g�vel.
 * A szenzor trigger bemenet�re egy 10 us hossz�s�g� impulzust kell kiadni.
 * A v�laszul kapott impulzus sz�less�ge a visszhang k�s�si idej�t adja meg.
 * A hang terjed�si sebess�g�nek ismeret�ben (~340 m/s) a t�vols�g kisz�m�that�.  
 * 
 * Hardver k�vetelm�nyek:
 *  - HC-SR04 ultrahangos szenzor 
 *  - Launchpad MSP430G2xxx mikrovez�rl�vel
 *  - Az RXD �tk�t�st el kell t�vol�tani
 *  - Az �jabb kiad�s� (v1.5) k�rty�n a TXD �tk�t�st 
 *    SW �ll�sba kell helyezni (a t�bbi �tk�t�ssel p�rhuzamosan)
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
 *  Fejleszt�i k�rnyezet: IAR Embedded Workbench for MSP430 v5.51 
 *********************************************************************/
#include "io430.h"
#include "stdint.h"

//-- Hardver absztrakci� -------------------------------------
#define LED1      BIT0            //Piros LED a P1.0 l�bon
#define TXD       BIT1            //TXD a P1.1 l�bon
#define ECHO      ________        //Echo jel a HC-SR04-b�l (CCI1A bemenet)
#define SW2       BIT3            //Nyom�gomb a P1.3 l�bon
#define TRIGGER   ________        //Trigger kimenet HC-SR04-hez
#define LED2      BIT6            //Z�ld LED a P1.6 l�bon 

#define FCPU      1000000         //CPU frekvencia Hz-ben 
#define FCPU_MHZ  1               //CPU frekvencia MHz-ben
#define TEN_US    FCPU/100000     //k�sleltet�si ciklusok 10 us-hez

uint16_t last_capture;
uint16_t this_capture;
volatile uint16_t delta;
int32_t pulse_width,distance;

/**------------------------------------------------------------
 *   Egy karakter kik�ld�se szoftveresen a soros portra
 *   SW_UART: 9600 bit/s, 8, N, 2 form�tum
 *   DCO = 1 MHz (Bitid� = 104.167 usec)
 *-------------------------------------------------------------
 * c - a kik�ldeni k�v�nt karakter k�dja
 */
void sw_uart_putc(char c) {
  uint8_t i;
  uint16_t TXData;                //Adatregiszter (11 bites)
  TXData = (uint16_t)c | 0x300;   //2 Stop bit (mark)
  TXData = TXData << 1;           //Start bit (space)
  for(i=0; i<11; i++) {           //11 bitet k�ld�nk ki
    if(TXData & 0x0001) {         //Soron k�vetkez� bit vizsg�lata
      P1OUT |= TXD;               //Ha '1'
    } else {
      P1OUT &= ~TXD;              //Ha '0'
    }
    TXData = TXData >> 1;         //Adatregiszter l�ptet�s jobbra
    __delay_cycles(89);           //<== Itt kell hangolni!
  }
  P1OUT |= TXD;                   //Az alaphelyzet: mark
}

/**------------------------------------------------------------
 *  Karakterf�z�r ki�r�sa a soros portra
 *-------------------------------------------------------------
 * p_str - karakterf�z�r mutat� (null�val lez�rt stringre mutat)
 */
void sw_uart_puts(char* p_str) {
  char c;
  while ((c=*p_str)) {
      sw_uart_putc(c);
      p_str++;
  }
}

/**------------------------------------------------------------
 * Decim�lis ki�rat�s adott sz�m� tizedesjegyre.
 *-------------------------------------------------------------
 * data - a ki�rand� el�jeles sz�m (int32_t)
 * ndigits - a ki�rand� tizedesek sz�ma (uint8_t)
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
  WDTCTL = WDTPW + WDTHOLD;       //Letiltjuk a watchdog id�z�t�t
  DCOCTL = CALDCO_1MHZ;           //DCO be�ll�t�sa a gy�rilag kalibr�lt 
  BCSCTL1 = CALBC1_1MHZ;          //1 MHz-es frekvenci�ra  
//-- LED-ek konfigur�l�sa -------------------------------------  
  P1OUT &= ~(LED1+LED2);          //Kezdetben nem vil�g�tanak  
  P1DIR |= LED1+LED2;             //LED1 �s LED2 digit�lis kimenet
//-- Szoftveres UART kimenet konfigur�l�sa --------------------    
  P1OUT |= TXD;                   //TXD alaphelyzete: mark
  P1DIR |= TXD;                   //TXD legyen digit�lis kimenet
//--- SW2 konfigur�l�s, bels� felh�z�s enged�lyez�se ---------- 
  P1DIR &= ~SW2;                  //P1.3 legyen digit�lis bemenet
  P1OUT |= SW2;                   //Felfel� h�zzuk, nem lefel�  
  P1REN |= SW2;                   //Bels� felh�z�s enged�lyez�se   
//-- HC-SR04 vez�rl�s konfigur�l�sa ---------------------------
  P1OUT &= ~TRIGGER;              //Kezdetben legyen alacsony  
  P1DIR |= TRIGGER;               //Trigger kimenet HC-SR04 sz�m�ra
  P1DIR &= ~ECHO;                 //ECHO bemenet (biztons�g kedv��rt felh�zzuk)
  P1OUT |= ECHO;                  //Felfel� h�zzuk, nem lefel�  
  P1REN |= ECHO;                  //Bels� felh�z�s enged�lyez�se   
  P1SEL ______________            //TA0CCI1A enged�lyez�se
  P1SEL2 &= ECHO;                 //a P1.2 bemeneten (ECHO)
  while(1) {
    delta = 0;                    //Null�zzuk a munkav�ltoz�t
//-- Timer0 �s TA0CCR1 konfigur�l�sa ---------------------------
    TA0CTL = ________ |           //Timer_A forr�sa SMCLK legyen
             ________ |           //Bemeneti oszt� = 1:1
             ________ |           //Folyamatos sz�ml�l�s m�d
                 TACLR;           //A sz�ml�l�t t�r�lj�k    
   TA0CCR1 = 0;                   //CCR1 adatregiszter�t t�r�lj�k
   TA0CCTL1 =  ______ |           //Felfut� �lre �rz�keny 
               ______ |           //TA0CCI1A bemenet kiv�laszt�sa
                  SCS |           //Szinkroniz�lt capture
                  CAP |           //Capture m�d
                  CCIE;           //CCR1 megszak�t�s enged�lyez�se     
//-- Trigger impulzussal elind�tjuk a m�r�st           ____________
    P1OUT |= TRIGGER;             //Trigger impulzus _| min. 10us  |___      
    __delay_cycles(TEN_US);
    P1OUT &= ~TRIGGER;
    WDTCTL = WDT_MRST_32;              //"Nem v�rok holnapig...."  
    ____________________;              //V�runk a Capture ciklus lezajl�s�ra
    if((delta>1000) && (delta<5000)) {
      P1OUT |= LED2;                   //Z�ld LED: tartom�nyon bel�l  
    }  
    else {
      P1OUT |= LED1;                   //Piros LED: tartom�nyon k�v�l
    }     
      pulse_width = delta/FCPU_MHZ;    //id�tartam mikroszekundumokban
      distance = (delta*17L)/(FCPU_MHZ*100L);  //t�vols�g millim�terekben
      sw_uart_puts("Pulse width = ");
      sw_uart_outdec(pulse_width,3);
      sw_uart_puts(" ms   Distance = ");
      sw_uart_outdec(distance,1);
      sw_uart_puts(" cm\r\n");
      __delay_cycles(FCPU);            //2 s v�rakoz�s
      P1OUT &= ~(LED1+LED2);           //K�zben lekapcsoljuk a LED-eket
      __delay_cycles(FCPU);    
  }
}

//--- TACCR1 megszak�t�s kiszolg�l�sa ---------------------------------
#pragma vector=TIMER0_A1_VECTOR
__interrupt void ta1_isr(void) {
  ______________________;              //TA0CCR1 megszak�t�si jelz�bit t�rl�se
  if(TA0CCTL1&CCI) {
    last_capture = TA0CCR1;            //A felfut� �lhez tartoz� id� r�gz�t�se
     TA0CCTL1 ____________;            //Lefut� �lre �rz�keny�t�nk
  } 
  else {
    TA0CCTL1 &= __________;            //Capture ciklus v�ge
    this_capture = TA0CCR1;            //A lefut� �lhez tartoz� id� r�gz�t�se
    delta = this_capture - last_capture; 
      WDTCTL = ___________;            //Letiltjuk a watchdog id�z�t�t
    __low_power_mode_off_on_exit();    //Fel�bresztj�k a f�programot
  }
}  
       