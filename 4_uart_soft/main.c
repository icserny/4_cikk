/************************************************************************************
 *  Launchpad dem�program: uart_soft
 *
 * Sz�veg ki�rat�sa soros kapcsolaton kereszt�l, egyir�ny� szoftveres UART kezel�s. 
 * Param�terek: 2400 baud, 8 adatbit, nincs parit�sbit, 1 stopbit 
 * A gy�rilag kalibr�lt 1 MHz-es DCO �rajelet haszn�ljuk: 
 * ACLK = n/a, MCLK = SMCLK = DCO 1 MHz
 * 
 * Hardver k�vetelm�nyek:
 *  - Launchpad MSP430G2xxx mikrovez�rl�vel
 *  - Az �jabb kiad�s� (v1.5) k�rty�n az RXD,TXD �tk�t�seket 
 *    SW �ll�sba kell helyezni (a t�bbi �tk�t�ssel p�rhuzamosan)
 *
 *                MSP430G2xxx
 *             -----------------
 *         /|\|              XIN|-
 *          | |                 |
 *          --|RST          XOUT|-
 *            |                 |
 *     TxD <--|P1.1             |
 *
 * 
 *  I. Cserny
 *  MTA ATOMKI, Debrecen
 *  June 2012
 *  Fejleszt�i k�rnyezet: IAR Embedded Workbench for MSP430 v5.30.1 Kickstart Edition
 ************************************************************************************/
#include "io430.h"
#include "stdint.h"

#define TXD       BIT1            // TXD a P1.1 l�bon


/**----------------------------------------------
 *   K�sleltet� elj�r�s (1 - 65535 ms)
 *-----------------------------------------------
 * delay - a k�sleltet�s ms egys�gben megadva
 */
void delay_ms(uint16_t delay) {
  uint16_t i;
  for(i=0; i<delay; i++) {
    __delay_cycles(1000);         //1 ms k�sleltet�s
  }
}

/**----------------------------------------------
 *   Egy karakter kik�ld�se a soros portra
 *   SW_UART: 9600 bit/s, 8, N, 1 form�tum
 *   DCO = 1 MHz (Bitid� = 104,17 usec)
 *-----------------------------------------------
 * c - a kik�ldeni k�v�nt karakter k�dja
 */
void sw_uart_putc(char c) {
  uint8_t i;
  uint16_t TXData;
  TXData = (uint16_t)c | 0x100;   //Stop bit (mark)
  TXData = TXData << 1;           //Start bit (space)
  for(i=0; i<10; i++) {
    if(TXData & 0x0001) {         //Soron k�vetkez� bit vizsg�lata
      P1OUT |= TXD;               //Ha '1'
    } else {
      P1OUT &= ~TXD;              //Ha '0'
    }
    TXData = TXData >> 1;         //Adatregiszter l�ptet�s jobbra
    __delay_cycles(89);          //<== Itt kell hangolni!
  }
  P1OUT |= TXD;                   //Az alaphelyzet: mark
}

/**----------------------------------------------
 *  Karakterf�z�r ki�r�sa a soros portra
 *-----------------------------------------------
 * p_str - karakterf�z�r mutat� (null�val lez�rt stringre mutat)
 */
void sw_uart_puts(char* p_str) {
  char c;
  while ((c=*p_str)) {            //Am�g a lez�r� null�t el nem �rj�k
      sw_uart_putc(c);            //egy karakter ki�r�s
      p_str++;                    //mutat� l�ptet�se 
  }
}

void main(void) {
  WDTCTL = WDTPW + WDTHOLD;       //Letiltjuk a watchdog id�z�t�t
  DCOCTL = CALDCO_1MHZ;           // DCO be�ll�t�sa a gy�rilag kalibr�lt 
  BCSCTL1 = CALBC1_1MHZ;          // 1 MHz-es frekvenci�ra  
//--- A TXD kimenet be�ll�t�sa ----------------------------------  
  P1DIR |= TXD;                   //TXD legyen digit�lis kimenet
  P1OUT |= TXD;                   //TXD alaphelyzete: mark
//---  P1.3 bemenet legyen, bels� felh�z�s�t enged�lyezz�k ------
  P1DIR &= ~BIT3;                 //P1.3 legyen digit�lis bemenet
  P1OUT |= BIT3;                  //Felfel� h�zzuk, nem lefel�  
  P1REN |= BIT3;                  //Bels� felh�z�s enged�lyez�se    
  while(1) {
    delay_ms(1000);               //1 m�sodperc v�rakoz�s 
    sw_uart_puts("Hello world!\r\n");  //Egy sor ki�r�sa
  }
}

