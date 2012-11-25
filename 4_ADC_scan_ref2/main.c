/**********************************************************************
 *  Launchpad dem�program: ADC_scan_ref2
 *
 * Egyszeri csatornap�szt�z� m�r�sek ADC-vel, �s a 2,5 V-os bels� referencia 
 * haszn�lat�val. Az A7 (P1.7 l�b), A6 (P1.6 l�b) �s A5 (P1.5 l�b) anal�g 
 * bemenetekre 0 - 2,5 V k�z�tti jelet vigy�nk.
 *
 * A m�r�s eredm�nyeit egyir�ny� szoftveres UART kezel�ssel (csak adatk�ld�s) 
 * ki�ratjuk. Param�terek: 9600 baud, 8 adatbit, nincs parit�sbit, 2 stopbit
 * 
 * A gy�rilag kalibr�lt 1 MHz-es DCO �rajelet haszn�ljuk: 
 * ACLK = n/a, MCLK = SMCLK = DCO 1 MHz
 * 
 * Hardver k�vetelm�nyek:
 *  - Launchpad MSP430G2xxx mikrovez�rl�vel
 *  - Az �jabb kiad�s� (v1.5) k�rty�n az RXD,TXD �tk�t�seket 
 *    SW �ll�sba kell helyezni (a t�bbi �tk�t�ssel p�rhuzamosan)
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
 *  Fejleszt�i k�rnyezet: IAR Embedded Workbench for MSP430 v5.51
 **********************************************************************/
#include  "io430.h"
#include "stdint.h"
#define TXD       BIT1                      //TXD a P1.1 l�bon

uint16_t adc_data[16];                      //Ide ker�lnek a m�r�si adatok

/*-------------------------------------------------------------
 * Egyszeri csatornap�szt�z�s, ADC a 2.5V-os bels� referenci�val
 *-------------------------------------------------------------
 * chan - a kezd� csatorna sorsz�ma << 12
 * pbuf - a kimeneti adatbuffer kezd�c�me
 * ndat - a p�szt�zand� csatorn�k sz�ma
 */
void ADC_scan_meas_REF2_5V(uint16_t chan, uint16_t *pbuf, uint8_t ndat) {
  ADC10CTL0 &= ~ENC;                   //Az ADC letilt�sa �jrakonfigur�l�shoz
  while (ADC10CTL1 & BUSY);            //V�rakoz�s a foglalts�g v�g�re
  ADC10CTL0 = ADC10SHT_3               //mintav�tel: 64 �ra�t�s
             | ADC10ON                 //Az ADC bekapcsol�sa
             | SREF_1                  //VR+ = VREF+ �s VR- = AVSS
             | REFON                   //Bels� referencia bekapcsol�sa
             | REF2_5V                 //2,5 V-os referencia kiv�laszt�sa
             | MSC;                    //T�bbsz�r�s konverzi� egy triggerjelre   
  ADC10CTL1 = ADC10SSEL_0              //ADC10OSC kiv�laszt�sa 
             | CONSEQ_1                //Egyszeri csatornap�szt�z�s
             | chan;                   //kezd� csatorna megad�sa
  ADC10DTC0 &= ~(ADC10TB+ADC10CT);     //Egy blokk m�d
  ADC10DTC1 = ndat;                    //ndat konverzi�t v�gz�nk  
  ADC10SA = (unsigned short)pbuf;      //adatok ment�se a mutat�val jelzett helyre 
  ADC10CTL0 |= ENC + ADC10SC;          //Konverzi� enged�lyez�se �s ind�t�sa
  while (!(ADC10CTL0 & ADC10IFG));     //V�rakoz�s a konverzi� v�g�re
}

/**------------------------------------------------------------
 *   K�sleltet� elj�r�s (1 - 65535 ms)
 *-------------------------------------------------------------
 * delay - a k�sleltet�s ms egys�gben megadva
 */
void delay_ms(uint16_t delay) {
  uint16_t i;
  for(i=0; i<delay; i++) {             //"delay"-szer ism�telj�k
    __delay_cycles(1000);              //1 ms k�sleltet�s
  }
}

/**------------------------------------------------------------
 *   Egy karakter kik�ld�se a soros portra
 *   SW_UART: 9600 bit/s, 8, N, 2 form�tum
 *   DCO = 1 MHz (Bitid� = 104.167 usec)
 *-------------------------------------------------------------
 * c - a kik�ldeni k�v�nt karakter k�dja
 */
void sw_uart_putc(char c) {
  uint8_t i;
  uint16_t TXData;                     //Adatregiszter (11 bites)
  TXData = (uint16_t)c | 0x300;        //2 Stop bit (mark)
  TXData = TXData << 1;                //Start bit (space)
  for(i=0; i<11; i++) {                //11 bitet k�ld�nk ki
    if(TXData & 0x0001) {              //Soron k�vetkez� bit vizsg�lata
      P1OUT |= TXD;                    //Ha '1'
    } else {
      P1OUT &= ~TXD;                   //Ha '0'
    }
    TXData = TXData >> 1;              //Adatregiszter l�ptet�s jobbra
    __delay_cycles(89);                //<== Itt kell hangolni!
  }
  P1OUT |= TXD;                        //Az alaphelyzet: mark
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
 * El�jel n�lk�li eg�sz sz�m ki�r�sa hexadecim�lis form�ban
 *-------------------------------------------------------------
 * t - a ki�rand� sz�m (uint16_t t�pus�)
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
uint8_t i;
int32_t temp;
  WDTCTL = WDTPW + WDTHOLD;            //Letiltjuk a watchdog id�z�t�t
  DCOCTL = CALDCO_1MHZ;                // DCO be�ll�t�sa a gy�rilag kalibr�lt 
  BCSCTL1 = CALBC1_1MHZ;               // 1 MHz-es frekvenci�ra  
  P1DIR |= TXD;                        //TXD legyen digit�lis kimenet
  P1OUT |= TXD;                        //TXD alaphelyzete: mark
//--- P1.3 bels� felh�z�s enged�lyez�se -----------------------  
  P1DIR &= ~BIT3;                      //P1.3 legyen digit�lis bemenet
  P1OUT |= BIT3;                       //Felfel� h�zzuk, nem lefel�  
  P1REN |= BIT3;                       //Bels� felh�z�s enged�lyez�se   
//--- Anal�g csatorn�k enged�lyez�se --------------------------
  ADC10AE0 |= BIT5+BIT6+BIT7;          //P1.5, P1.6, P1.7 legyen anal�g bemenet
  while(1) {
    delay_ms(2000);
//--- P�szt�z� m�r�s az A7, A6 �s A5 csatorn�kban -------------   
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