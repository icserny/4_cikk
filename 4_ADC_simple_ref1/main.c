/**********************************************************************
 *  Launchpad demóprogram: ADC_simple_ref1
 *
 * Egyszerû mérés ADC-vel, az 1,5 V-os belsõ referencia használatával.
 * Az A5 (P1.5 láb) analóg bemenetre 0 - 1,5 V közötti jelet vigyünk.
 * Emellett megmérjük a belsõ hõmérõ jelét is (Chan 10).
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
 *
 *       MSP430G2231, MSP430G2452 vagy MSP430G2553
 *             -----------------
 *         /|\|              XIN|-
 *          | |                 |
 *          --|RST          XOUT|-
 *            |                 |
 *     TxD <--|P1.1         P1.5|<-- 0-1,5 V
 *
 * 
 *   
 *  I. Cserny
 *  MTA ATOMKI, Debrecen
 *  Nov 2012
 *  Fejlesztõi környezet: IAR Embedded Workbench for MSP430 v5.51
 **********************************************************************/
#include "io430.h"
#include "stdint.h"

#define TXD       BIT1                 // TXD a P1.1 lábon

/*-------------------------------------------------------------
 * Egyszeri mérés egy ADC csatornában, 1,5 V a belsõ referencia
 *-------------------------------------------------------------
 * chan - csatornaválasztó bitek (a csatorna sorszáma << 12)
 */
uint16_t ADC_single_meas_REF1_5V(uint16_t chan) {
  ADC10CTL0 &= ~ENC;                   //Az ADC letiltása újrakonfiguráláshoz
  ADC10CTL0 = ADC10SHT_3               //mintavétel: 64 óraütés
             | ADC10ON                 //Az ADC bekapcsolása
             | SREF_1                  //VR+ = VREF+ és VR- = AVSS
             | REFON;                  //Belsõ referencia bekapcsolása
                                       //1,5 V-os referencia kiválasztása
  ADC10CTL1 = ADC10SSEL_0 + chan;      //csatorna = 'chan', ADC10OSC az órajel
  ADC10CTL0 |= ENC + ADC10SC;          //Konverzió engedélyezése és indítása
  while (ADC10CTL1 & BUSY);            //Várakozás a konverzió végére
  return ADC10MEM;                     //Visszatérés a konverzió eredményével 
}


/**------------------------------------------------------------
 *   Késleltetõ eljárás (1 - 65535 ms)
 *-------------------------------------------------------------
 * delay - a késleltetés ms egységben megadva
 */
void delay_ms(uint16_t delay) {
  uint16_t i;
  for(i=0; i<delay; i++) {        //"delay"-szer ismételjük
    __delay_cycles(1000);         //1 ms késleltetés
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
 * data - a kiírandó szám (elõjelesen)
 * ndigits - a kiírandó tizedesek száma
 */
void sw_uart_outdec(int32_t data, uint8_t ndigits) {
  static char sign, s[12];
  int8_t i;
  i=0; sign=' ';
  if(data<0) {                         //Az elõjel meghatározása
    sign='-'; 
    data = -data;
  }
    do {
      s[i]=data%10 + '0';              //A számjegyek meghatározása
      data=data/10;                    //hátulról visszafelé
      i++;
      if(i==ndigits) {s[i]='.'; i++;}  //A tizedespont beszúrása
    } while(data>0 || i<(ndigits+2));
    sw_uart_putc(sign);                //Az elõjel kiírása
    do {
      sw_uart_putc(s[--i]);            //A számjegyek kiírása
    } while(i);
}

void main(void) {
uint16_t data;
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
  ADC10AE0 |= BIT5;                    //P1.5 legyen analóg bemenet
  while(1) {
    delay_ms(1000);
    data = ADC_single_meas_REF1_5V(INCH_5);
    sw_uart_puts("chan 5 = ");
    sw_uart_out4hex(data);
    temp = (int32_t)data*1500L/1023;   //A mV-okban mért feszültség
    sw_uart_outdec(temp,3);            //kiírás 3 tizedesre
    sw_uart_puts("V chan 10 = ");      
    data = ADC_single_meas_REF1_5V(INCH_10);
    sw_uart_out4hex(data);   
    temp = ((uint32_t)data*270687L - 182026240L) >> 16;  
    sw_uart_puts(" temp = "); 
    sw_uart_outdec(temp,1);       
    sw_uart_puts(" C\r\n");   
    delay_ms(1000);    
  }
}
