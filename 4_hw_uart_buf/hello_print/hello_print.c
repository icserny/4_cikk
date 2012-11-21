/**********************************************************************
 *  Launchpad dem�program: hello_print.c
 *
 * Egyszer� mintaprogram a szoftveresen bufferelt hardveres UART kezel�s 
 * bemutat�s�ra. Az �dv�zl� sz�veg ki�rat�sa ut�n a be�rkez� karaktereket 
 * �s azok ASCII k�dj�t ki�ratjuk. 
 *
 * Param�terek: 9600 baud, 8 adatbit, nincs parit�sbit, 1 stopbit 
 * A gy�rilag kalibr�lt 1 MHz-es DCO �rajelet haszn�ljuk: 
 * ACLK = n/a, MCLK = SMCLK = DCO 1 MHz
 *
 * Megjegyz�sek: 
 *  - A f�programba be kell csatolni a hw_uart_buf.h fejl�c �llom�nyt
 *  - A projektbe fel kell venni a hw_uart_buf.c �llom�nyt
 *  - A hw_uart_buf.c �llom�nyban defini�lt I/O f�ggv�nyek mellett
 *    a printf f�ggv�nyt is haszn�lhatjuk, ha az ebben a mintaprogramban
 *    bemutatott m�don �jradefini�ljuk a putchar() f�ggv�nyt (IAR saj�toss�g!)
 *  - A mem�ria takar�kos felhaszn�l�s c�lj�b�l a projekt opci�kn�l �ll�tsuk be, 
 *    hogy a printf() k�nyvt�ri f�ggv�ny t�pusa "Auto" legyen (a linker ne csatoljon
 *    be f�l�slegesen bonyolult form�tumlezel�seket - ez is IAR saj�toss�g)    
 *
 * 
 * Hardver k�vetelm�nyek:
 *  - Launchpad MSP430G253 mikrovez�rl�vel
 *  - Az �jabb kiad�s� (v1.5) k�rty�n az RXD,TXD �tk�t�seket 
 *    SW �ll�sba kell helyezni (a t�bbi �tk�t�ssel p�rhuzamosan)
 *
 *                MSP430G2553
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
 *  Nov 2012
 *  Fejleszt�i k�rnyezet: IAR Embedded Workbench for MSP430 v5.51 
 **********************************************************************/
#include <msp430.h>
#include <stdint.h>
#include <stdio.h>
#include "hw_uart_buf.h"

/** A k�nyvt�ri putchar() f�ggv�ny �radefini�l�sa ut�n
 *  a printf() f�ggv�ny is haszn�lhat� a soros porton t�rt�n� ki�r�sra.
 *  Ez IAR saj�toss�g, a CCS eset�ben m�sk�pp kell megoldani az �tir�ny�t�st!
 */ 
int putchar(int outChar)
{
  uart_putc((uint8_t)outChar);
  return outChar;
}

char c;
int main( void ) {
  WDTCTL = WDTPW + WDTHOLD;            //Letiltjuk a watchdog id�z�t�t
  DCOCTL = CALDCO_1MHZ;                //DCO be�ll�t�sa a gy�rilag kalibr�lt 
  BCSCTL1 = CALBC1_1MHZ;               //1 MHz-es frekvenci�ra  
  uart_init(BPS_9600);
  printf("\r\nIsten hozott a Launchpad projekthez!\r\n");
  printf("hello_print program (hw_uart_buf)\r\n");
  while (1) {
    c=uart_getc();
    printf("Vett karakter: %c = %d\r\n",c,c);
  }
}
