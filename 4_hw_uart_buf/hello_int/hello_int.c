/**********************************************************************
 *  Launchpad dem�program: hello_int.c
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
 *  - A takar�kos mem�riahaszn�lat �rdek�ben a printf() f�ggv�ny helyett a 
 *    hw_uart_buf.c �llom�nyban defini�lt I/O f�ggv�nyeket haszn�ljuk
 *  - Az uart_puts() f�ggv�ny az "�j sor" karakterek el� automatikusan bef�z
 *    egy "kocsi vissza" karaktert.
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
#include "hw_uart_buf.h"

char c;
int main( void ) {
  WDTCTL = WDTPW + WDTHOLD;            //Letiltjuk a watchdog id�z�t�t
  DCOCTL = CALDCO_1MHZ;                //DCO be�ll�t�sa a gy�rilag kalibr�lt 
  BCSCTL1 = CALBC1_1MHZ;               //1 MHz-es frekvenci�ra  
  uart_init(BPS_9600);
  uart_puts("\nIsten hozott a Launchpad projekthez!\n");
  uart_puts("hello_int program (hw_uart_buf)\n");
  while (1) {
    c=uart_getc();
    uart_puts("Vett karakter: ");
    uart_putc(c);
    uart_puts(" = ");
    uart_outdec((int32_t)c,0);
    uart_puts("\n");
  }
}
