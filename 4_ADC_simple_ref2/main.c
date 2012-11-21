/**********************************************************************
 *  Launchpad dem�program: ADC_simple_ref2
 *
 * Egyszer� m�r�s ADC-vel, a 2,5 V-os bels� referencia haszn�lat�val.
 * Az A5 (P1.5 l�b) anal�g bemenetre 0 - 2,5 V k�z�tti jelet vigy�nk.
 * Emellett megm�rj�k a f�l t�pfesz�lts�get (Chan 11) �s a bels�
 * h�m�r� jel�t (Chan 10).
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
 *
 *                MSP430G2xxx
 *             -----------------
 *         /|\|              XIN|-
 *          | |                 |
 *          --|RST          XOUT|-
 *            |                 |
 *     TxD <--|P1.1         P1.5|<-- 0-2,5 V
 *
 *   
 *  I. Cserny
 *  MTA ATOMKI, Debrecen
 *  Nov 2012
 *  Fejleszt�i k�rnyezet: IAR Embedded Workbench for MSP430 v5.51
 **********************************************************************/
#include "io430.h"
#include "stdint.h"

#define TXD       BIT1                 // TXD a P1.1 l�bon

/*-------------------------------------------------------------
 * Egyszeri m�r�s egy ADC csatorn�ban, 2,5 V a bels� referencia
 *-------------------------------------------------------------
 * chan - csatornav�laszt� bitek (a csatorna sorsz�ma << 12)
 */
uint16_t ADC_single_meas_REF2_5V(uint16_t chan) {
  ADC10CTL0 &= ~ENC;                   //Az ADC letilt�sa �jrakonfigur�l�shoz
  ADC10CTL0 = ADC10SHT_3               //mintav�tel: 64 �ra�t�s
             | ADC10ON                 //Az ADC bekapcsol�sa
             | SREF_1                  //VR+ = VREF+ �s VR- = AVSS
             | REFON                   //Bels� referencia bekapcsol�sa
             | REF2_5V;                //2,5 V-os referencia kiv�laszt�sa
  ADC10CTL1 = ADC10SSEL_0 + chan;      //csatorna = 'chan', ADC10OSC az �rajel
  ADC10CTL0 |= ENC + ADC10SC;          //Konverzi� enged�lyez�se �s ind�t�sa
  while (ADC10CTL1 & BUSY);            //V�rakoz�s a konverzi� v�g�re
  return ADC10MEM;                     //Visszat�r�si �rt�k a konverzi� eredm�nye 
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
uint16_t data;
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
  ADC10AE0 |= BIT5;                    //P1.5 legyen anal�g bemenet
  while(1) {
    delay_ms(1000);
//--- Az A5 csatorna (P1.5) bemen� fesz�lts�g�nek m�r�se ------    
    data = ADC_single_meas_REF2_5V(INCH_5);
    sw_uart_puts("\r\nchan 5 = ");
    sw_uart_out4hex(data);
    temp = (int32_t)data*2500L/1023;   //A mV-okban m�rt fesz�lts�g
    sw_uart_outdec(temp,3);            //ki�r�s 3 tizedesre
    sw_uart_puts("V chan 11 = ");    
//--- Chan 11 = VDD/2 m�r�se ----------------------------------   
    data = ADC_single_meas_REF2_5V(INCH_11);
    sw_uart_out4hex(data);
    temp = (int32_t)data*5000L/1023;   //VDD mV-okban m�rve
    sw_uart_outdec(temp,3);            //ki�r�s 3 tizedesre
    sw_uart_puts("V chan 10 = ");  
//--- Chan 10 a bels� h�m�r� ----------------------------------    
    data = ADC_single_meas_REF2_5V(INCH_10);
    sw_uart_out4hex(data);    
    temp = ((uint32_t)data*1762L - 711040L) >> 8;   
    sw_uart_puts(" temp = ");
    sw_uart_outdec(temp,1);            //Ki�r�s 1 tizedesre
    sw_uart_puts(" C");   
    delay_ms(1000);    
  }
}
