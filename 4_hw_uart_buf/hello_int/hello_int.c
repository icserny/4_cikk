/**********************************************************************
 *  Launchpad demóprogram: hello_int.c
 *
 * Egyszerû mintaprogram a szoftveresen bufferelt hardveres UART kezelés 
 * bemutatására. Az üdvözlõ szöveg kiíratása után a beérkezõ karaktereket 
 * és azok ASCII kódját kiíratjuk. 
 *
 * Paraméterek: 9600 baud, 8 adatbit, nincs paritásbit, 1 stopbit 
 * A gyárilag kalibrált 1 MHz-es DCO órajelet használjuk: 
 * ACLK = n/a, MCLK = SMCLK = DCO 1 MHz
 *
 * Megjegyzések: 
 *  - A fõprogramba be kell csatolni a hw_uart_buf.h fejléc állományt
 *  - A projektbe fel kell venni a hw_uart_buf.c állományt
 *  - A takarékos memóriahasználat érdekében a printf() függvény helyett a 
 *    hw_uart_buf.c állományban definiált I/O függvényeket használjuk
 *  - Az uart_puts() függvény az "új sor" karakterek elé automatikusan befûz
 *    egy "kocsi vissza" karaktert.
 * 
 * Hardver követelmények:
 *  - Launchpad MSP430G253 mikrovezérlõvel
 *  - Az újabb kiadású (v1.5) kártyán az RXD,TXD átkötéseket 
 *    SW állásba kell helyezni (a többi átkötéssel párhuzamosan)
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
 *  Fejlesztõi környezet: IAR Embedded Workbench for MSP430 v5.51 
 **********************************************************************/
#include <msp430.h>
#include <stdint.h>
#include "hw_uart_buf.h"

char c;
int main( void ) {
  WDTCTL = WDTPW + WDTHOLD;            //Letiltjuk a watchdog idõzítõt
  DCOCTL = CALDCO_1MHZ;                //DCO beállítása a gyárilag kalibrált 
  BCSCTL1 = CALBC1_1MHZ;               //1 MHz-es frekvenciára  
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
