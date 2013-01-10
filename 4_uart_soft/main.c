/************************************************************************************
 *  Launchpad demóprogram: uart_soft
 *
 * Szöveg kiíratása soros kapcsolaton keresztül, egyirányú szoftveres UART kezelés. 
 * Paraméterek: 2400 baud, 8 adatbit, nincs paritásbit, 1 stopbit 
 * A gyárilag kalibrált 1 MHz-es DCO órajelet használjuk: 
 * ACLK = n/a, MCLK = SMCLK = DCO 1 MHz
 * 
 * Hardver követelmények:
 *  - Launchpad MSP430G2xxx mikrovezérlõvel
 *  - Az újabb kiadású (v1.5) kártyán az RXD,TXD átkötéseket 
 *    SW állásba kell helyezni (a többi átkötéssel párhuzamosan)
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
 *  Fejlesztõi környezet: IAR Embedded Workbench for MSP430 v5.30.1 Kickstart Edition
 ************************************************************************************/
#include "io430.h"
#include "stdint.h"

#define TXD       BIT1            // TXD a P1.1 lábon


/**----------------------------------------------
 *   Késleltetõ eljárás (1 - 65535 ms)
 *-----------------------------------------------
 * delay - a késleltetés ms egységben megadva
 */
void delay_ms(uint16_t delay) {
  uint16_t i;
  for(i=0; i<delay; i++) {
    __delay_cycles(1000);         //1 ms késleltetés
  }
}

/**----------------------------------------------
 *   Egy karakter kiküldése a soros portra
 *   SW_UART: 9600 bit/s, 8, N, 1 formátum
 *   DCO = 1 MHz (Bitidõ = 104,17 usec)
 *-----------------------------------------------
 * c - a kiküldeni kívánt karakter kódja
 */
void sw_uart_putc(char c) {
  uint8_t i;
  uint16_t TXData;
  TXData = (uint16_t)c | 0x100;   //Stop bit (mark)
  TXData = TXData << 1;           //Start bit (space)
  for(i=0; i<10; i++) {
    if(TXData & 0x0001) {         //Soron következõ bit vizsgálata
      P1OUT |= TXD;               //Ha '1'
    } else {
      P1OUT &= ~TXD;              //Ha '0'
    }
    TXData = TXData >> 1;         //Adatregiszter léptetés jobbra
    __delay_cycles(89);          //<== Itt kell hangolni!
  }
  P1OUT |= TXD;                   //Az alaphelyzet: mark
}

/**----------------------------------------------
 *  Karakterfüzér kiírása a soros portra
 *-----------------------------------------------
 * p_str - karakterfüzér mutató (nullával lezárt stringre mutat)
 */
void sw_uart_puts(char* p_str) {
  char c;
  while ((c=*p_str)) {            //Amíg a lezáró nullát el nem érjük
      sw_uart_putc(c);            //egy karakter kiírás
      p_str++;                    //mutató léptetése 
  }
}

void main(void) {
  WDTCTL = WDTPW + WDTHOLD;       //Letiltjuk a watchdog idõzítõt
  DCOCTL = CALDCO_1MHZ;           // DCO beállítása a gyárilag kalibrált 
  BCSCTL1 = CALBC1_1MHZ;          // 1 MHz-es frekvenciára  
//--- A TXD kimenet beállítása ----------------------------------  
  P1DIR |= TXD;                   //TXD legyen digitális kimenet
  P1OUT |= TXD;                   //TXD alaphelyzete: mark
//---  P1.3 bemenet legyen, belsõ felhúzását engedélyezzük ------
  P1DIR &= ~BIT3;                 //P1.3 legyen digitális bemenet
  P1OUT |= BIT3;                  //Felfelé húzzuk, nem lefelé  
  P1REN |= BIT3;                  //Belsõ felhúzás engedélyezése    
  while(1) {
    delay_ms(1000);               //1 másodperc várakozás 
    sw_uart_puts("Hello world!\r\n");  //Egy sor kiírása
  }
}

