/**********************************************************************
 *  Launchpad demóprogram: ADC_scan_ref2
 *
 * Egyszeri csatornapásztázó mérések ADC-vel, és a 2,5 V-os belsõ referencia 
 * használatával. Az A7 (P1.7 láb), A6 (P1.6 láb) és A5 (P1.5 láb) analóg 
 * bemenetekre 0 - 2,5 V közötti jelet vigyünk.
 *
 * A mérés eredményeit egyirányú szoftveres UART kezeléssel (csak adatküldés) 
 * kiíratjuk. Paraméterek: 9600 baud, 8 adatbit, nincs paritásbit, 2 stopbit
 * 
 * A gyárilag kalibrált 1 MHz-es DCO órajelet használjuk: 
 * ACLK = n/a, MCLK = SMCLK = DCO 1 MHz
 * 
 * Hardver követelmények:
 *  - Launchpad MSP430G2xxx mikrovezérlõvel
 *  - Az újabb kiadású (v1.5) kártyán az RXD,TXD átkötéseket 
 *    SW állásba kell helyezni (a többi átkötéssel párhuzamosan)
 *
 *       MSP430G2231, MSP430G2452 vagy MSP430G2553
 *             -----------------
 *         /|\|              XIN|-
 *          | |                 |
 *          --|RST          XOUT|-
 *            |                 |
 *     TxD <--|P1.1         P1.7|<-- 0-2,5 V
 *            |                 |
 *   0-2.5V-->|P1.5         P1.6|<-- 0-2,5 V  
 * 
 *   
 *  I. Cserny
 *  MTA ATOMKI, Debrecen
 *  Nov 2012
 *  Fejlesztõi környezet: IAR Embedded Workbench for MSP430 v5.51
 **********************************************************************/
#include  "io430.h"
#include "stdint.h"
#define TXD       BIT1                      //TXD a P1.1 lábon

uint16_t adc_data[16];                      //Ide kerülnek a mérési adatok

/*-------------------------------------------------------------
 * Egyszeri csatornapásztázás, ADC a 2.5V-os belsõ referenciával
 *-------------------------------------------------------------
 * chan - a kezdõ csatorna sorszáma << 12
 * pbuf - a kimeneti adatbuffer kezdõcíme
 * ndat - a pásztázandó csatornák száma
 */
void ADC_scan_meas_REF2_5V(uint16_t chan, uint16_t *pbuf, uint8_t ndat) {
  ADC10CTL0 &= ~ENC;                   //Az ADC letiltása újrakonfiguráláshoz
  while (ADC10CTL1 & BUSY);            //Várakozás a foglaltság végére
  ADC10CTL0 = ADC10SHT_3               //mintavétel: 64 óraütés
             | ADC10ON                 //Az ADC bekapcsolása
             | SREF_1                  //VR+ = VREF+ és VR- = AVSS
             | REFON                   //Belsõ referencia bekapcsolása
             | REF2_5V                 //2,5 V-os referencia kiválasztása
             | MSC;                    //Többszörös konverzió egy triggerjelre   
  ADC10CTL1 = ADC10SSEL_0              //ADC10OSC kiválasztása 
             | CONSEQ_1                //Egyszeri csatornapásztázás
             | chan;                   //kezdõ csatorna megadása
  ADC10DTC0 &= ~(ADC10TB+ADC10CT);     //Egy blokk mód
  ADC10DTC1 = ndat;                    //ndat konverziót végzünk  
  ADC10SA = (unsigned short)pbuf;      //adatok mentése a mutatóval jelzett helyre 
  ADC10CTL0 |= ENC + ADC10SC;          //Konverzió engedélyezése és indítása
  while (!(ADC10CTL0 & ADC10IFG));     //Várakozás a konverzió végére
}

/**------------------------------------------------------------
 *   Késleltetõ eljárás (1 - 65535 ms)
 *-------------------------------------------------------------
 * delay - a késleltetés ms egységben megadva
 */
void delay_ms(uint16_t delay) {
  uint16_t i;
  for(i=0; i<delay; i++) {             //"delay"-szer ismételjük
    __delay_cycles(1000);              //1 ms késleltetés
  }
}

/**------------------------------------------------------------
 *   Egy karakter kiküldése a soros portra
 *   SW_UART: 9600 bit/s, 8, N, 2 formátum
 *   DCO = 1 MHz (Bitidõ = 104.167 usec)
 *-------------------------------------------------------------
 * c - a kiküldeni kívánt karakter kódja
 */
void sw_uart_putc(char c) {
  uint8_t i;
  uint16_t TXData;                     //Adatregiszter (11 bites)
  TXData = (uint16_t)c | 0x300;        //2 Stop bit (mark)
  TXData = TXData << 1;                //Start bit (space)
  for(i=0; i<11; i++) {                //11 bitet küldünk ki
    if(TXData & 0x0001) {              //Soron következõ bit vizsgálata
      P1OUT |= TXD;                    //Ha '1'
    } else {
      P1OUT &= ~TXD;                   //Ha '0'
    }
    TXData = TXData >> 1;              //Adatregiszter léptetés jobbra
    __delay_cycles(89);                //<== Itt kell hangolni!
  }
  P1OUT |= TXD;                        //Az alaphelyzet: mark
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
 * Elõjel nélküli egész szám kiírása hexadecimális formában
 *-------------------------------------------------------------
 * t - a kiírandó szám (uint16_t típusú)
 */
void sw_uart_out4hex(uint16_t t)
{
  char c;
  c=(char)((t>>12) & 0x0F);
  if (c>9) c+=7;
  sw_uart_putc(c+'0');
  c=(char)((t>>8) & 0x0F);
  if (c>9) c+=7;
  sw_uart_putc(c+'0');
  c=(char)((t>>4) & 0x0F);
  if (c>9) c+=7;
  sw_uart_putc(c+'0');
  c=(char)(t & 0x0F);
  if (c>9) c+=7;
  sw_uart_putc(c+'0');
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
uint8_t i;
int32_t temp;
  WDTCTL = WDTPW + WDTHOLD;            //Letiltjuk a watchdog idõzítõt
  DCOCTL = CALDCO_1MHZ;                // DCO beállítása a gyárilag kalibrált 
  BCSCTL1 = CALBC1_1MHZ;               // 1 MHz-es frekvenciára  
  P1DIR |= TXD;                        //TXD legyen digitális kimenet
  P1OUT |= TXD;                        //TXD alaphelyzete: mark
//--- P1.3 belsõ felhúzás engedélyezése -----------------------  
  P1DIR &= ~BIT3;                      //P1.3 legyen digitális bemenet
  P1OUT |= BIT3;                       //Felfelé húzzuk, nem lefelé  
  P1REN |= BIT3;                       //Belsõ felhúzás engedélyezése   
//--- Analóg csatornák engedélyezése --------------------------
  ADC10AE0 |= BIT5+BIT6+BIT7;          //P1.5, P1.6, P1.7 legyen analóg bemenet
  while(1) {
    delay_ms(2000);
//--- Pásztázó mérés az A7, A6 és A5 csatornákban -------------   
    ADC_scan_meas_REF2_5V(INCH_7, adc_data,3);  
    for(i=0; i<3; i++) {
      temp = (int32_t)adc_data[i]*2500L/1023L;
      sw_uart_putc(i+48);
      sw_uart_puts(". csat = ");
      sw_uart_outdec(temp,3);
      sw_uart_puts(" V  ");
    }
    sw_uart_puts("\r\n");  
  }
}