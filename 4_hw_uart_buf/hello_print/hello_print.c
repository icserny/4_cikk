/**********************************************************************
 *  Launchpad demóprogram: hello_print.c
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
 *  - A hw_uart_buf.c állományban definiált I/O függvények mellett
 *    a printf függvényt is használhatjuk, ha az ebben a mintaprogramban
 *    bemutatott módon újradefiniáljuk a putchar() függvényt (IAR sajátosság!)
 *  - A memória takarékos felhasználás céljából a projekt opcióknál állítsuk be, 
 *    hogy a printf() könyvtári függvény típusa "Auto" legyen (a linker ne csatoljon
 *    be fölöslegesen bonyolult formátumlezeléseket - ez is IAR sajátosság)    
 *
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
#include <stdio.h>
#include "hw_uart_buf.h"

/** A könyvtári putchar() függvény úradefiniálása után
 *  a printf() függvány is használható a soros porton történõ kiírásra.
 *  Ez IAR sajátosság, a CCS esetében másképp kell megoldani az átirányítást!
 */ 
int putchar(int outChar)
{
  uart_putc((uint8_t)outChar);
  return outChar;
}

char c;
int main( void ) {
  WDTCTL = WDTPW + WDTHOLD;            //Letiltjuk a watchdog idõzítõt
  DCOCTL = CALDCO_1MHZ;                //DCO beállítása a gyárilag kalibrált 
  BCSCTL1 = CALBC1_1MHZ;               //1 MHz-es frekvenciára  
  uart_init(BPS_9600);
  printf("\r\nIsten hozott a Launchpad projekthez!\r\n");
  printf("hello_print program (hw_uart_buf)\r\n");
  while (1) {
    c=uart_getc();
    printf("Vett karakter: %c = %d\r\n",c,c);
  }
}
