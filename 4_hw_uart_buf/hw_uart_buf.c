/* Launchpad project
 * Copyright (c) 2012 Istvan Cserny (cserny@atomki.hu)
 *
 */
#include <msp430.h>
#include <stdint.h>
#include "hw_uart_buf.h"
/** \file
 *  Ez az állomány definiálja azokat a függvényeket, amelyekkel az
 *  alkalmazói programból a hardveres UART kapcsolat kezelhetjük.
 */

/* GLOBÁLIS VÁLTOZÓK az UART kommunikációhoz **************************/
#define RX_BUF_SIZE 8                  // UART vevõoldali buffer mérete (2 hatványa legyen!)
#define TX_BUF_SIZE 8                  // UART  adóoldali buffer mérete (2 hatványa legyen!)

uint8_t RX_inp;                        // UART vevõoldali buffer bemeneti mutató
uint8_t RX_outp;                       // UART vevõoldali buffer kimeneti mutató
uint8_t TX_inp;                        // UART  adóoldali buffer bemeneti mutató
uint8_t TX_outp;                       // UART  adóoldali buffer kimeneti mutató
uint8_t RX_buffer[RX_BUF_SIZE];	       // UART vevõoldali buffer (ebbõl olvasunk)
uint8_t TX_buffer[TX_BUF_SIZE];	       // UART  adóoldali buffer (ebbe írunk)

/** Annak ellenõrzése, hogy van-e kiolvasatlan karakter a bemeneti bufferben
 *  A vizsgálathoz elegendõ összehasonlítani a bemeneti és a kimeneti mutatókat 
 */
char uart_kbhit(void) {
  return (char)(RX_inp!=RX_outp);
}

/** Egy karakter kiírása a kimeneti bufferbe.
 *  Blokkoló típusú függvény, addig várakozik, amíg 
 *  szabad helyet nem talál a kimenti bufferben.
 *  \param c a kiírandó karakter
 *  \return a kiírt karakter kódja int típussá konvertálva
 */
void uart_putc(uint8_t c) {
  uint8_t x;
  //-- Kritikus tartomány, vigyázz a buffer konkurens kezelésére!
  TX_buffer[TX_inp] = c;
  x = (TX_inp+1) & (TX_BUF_SIZE-1);    //TX_inp leendõ értéke
  //-- Kritikus tartomány vége --------
  while(x == TX_outp);                 //Várunk, ha utolértük a kimeneti mutatót
  TX_inp = x;
  IE2 |= UCA0TXIE;                    //Tx megszakítás engedélyezése 
}


/** Beolvas egy karaktert az input bufferbõl, vagy várakozik, ha az üres.
 * \return c, a beolvasott karakter kódja (uint8_t típusú)
 */
uint8_t uart_getc(void) {
  uint8_t c;
  while(RX_inp == RX_outp);            //Várunk egy karakter beérkezésére
  //-- Kritikus tartomány, vigyázz a buffer konkurens kezelésére!
  c =RX_buffer[RX_outp];               //A soron következõ karakter elõvétele
  RX_outp=(RX_outp+1)&(RX_BUF_SIZE-1); //A kivételi mutató léptetése
  //-- Kritikus tartomány vége --------
  return (c);
} 


/** Kiír egy nullával lezárt szöveget a kimeneti bufferbe.
 *  Ez a függvény a blokkoló típusú uart_putc() függvényt hívja!  
 *  Sorvége esetén automatukusan beiktat egy Carriage Return karaktert.
 *  \param p_str mutató, a kiíranó szöveghez
 */
void uart_puts(uint8_t* p_str) {
  uint8_t c;
  while ((c=*p_str))     {
    if (c == '\n') {
      uart_putc(0x0D);
    }
    uart_putc(c);
    p_str++;
  }
}

/** Decimális kiíratás adott számú tizedesjegyre. 
 * \param data a kiírandó szám (elõjelesen)
 * \param ndigits a kiírandó tizedesek száma
 */
void uart_outdec(int32_t data, uint8_t ndigits) {
  static uint8_t sign, s[12];
  uint8_t i;
  i=0; sign=' ';
  if(data<0) { sign='-'; data = -data;}
  do {
    s[i]=data%10 + '0';
    data=data/10;
    i++;
    if(i==ndigits) {s[i]='.'; i++;}
  } while(data > 0 || i < ndigits+2);
  uart_putc(sign);
  do {
    uart_putc(s[--i]);
  } while(i);
}

/** Négy hexadecimális számjegy beolvasása és átalakítása uint16 típusra.
 * Ez a függvény blokkoló típusú, addig vár, amíg be nem érkezik négy kararakter,
 * amelyeket az uart_putc() eljárással azonnal vissza is tükrözünk.
 */
uint16_t uart_get4hex(void) {
  uint8_t c,i;
  uint16_t t;
  t=0;
  for (i=0; i<4; i++) {
    c=uart_getc();
    uart_putc(c);
    if (c>0x40) {
      c -=7;
    }
    t= (t<<4) + (c & 0x0F);
  }
  return t;
}

/** Egy elõjel nélküli egész számot hexadecimális formában kiír
 * a kimeneti bufferbe. Ez a függvény meghívja a blokkoló típusú
 *  _UART_putc() függvényt! 
 */
void uart_out4hex(uint16_t t) {
  uint8_t c;
  c=(uint8_t)((t>>12) & 0x0F);
  if (c>9) c+=7;
  uart_putc(c+'0');
  c=(uint8_t)((t>>8) & 0x0F);
  if (c>9) c+=7;
  uart_putc(c+'0');
  c=(uint8_t)((t>>4) & 0x0F);
  if (c>9) c+=7;
  uart_putc(c+'0');
  c=(uint8_t)(t & 0x0F);
  if (c>9) c+=7;
  uart_putc(c+'0');
}

/** Egy elõjel nélküli egész számot hexadecimális formában kiír
 * a kimeneti bufferbe. Ez a függvény meghívja a blokkoló típusú
 *  uart_putc() függvényt! 
 */
void uart_out2hex(uint8_t t) {
  uint8_t c;
  c=(t>>4) & 0x0F;
  if (c>9) c+=7;
  uart_putc(c+'0');
  c=t & 0x0F;
  if (c>9) c+=7;
  uart_putc(c+'0');
}

/** Két hexadecimális számjegy beolvasása és átalakítása uint8 típusra
 *  Ez a függvény blokkoló típusú, addig vár, amíg be nem érkezik két kararakter,
 *  amelyeket az uart_putc() eljárással vissza is tükrözünk. 
 */
uint8_t uart_get2hex(void) {
  uint8_t c,i,t;
  t=0;
  for (i=0; i<2; i++) {
    c=uart_getc();
    uart_putc(c);
    if (c>0x40) {
      c -=7;
    }
    t= (t<<4) + (c & 0x0F);
  }
  return t;
}

/** UART módba konfigurálja az USCI0_A modult
 *  és nullázza a buffer mutatókat
 *  \param UART_baud a baudrate beállításához elõre definiált
 *  paraméter (lásd usart.h-ban)
 */
void uart_init(uint16_t UART_baud) {
//-- UART TX és RX buffer mutatók inicializálása
  RX_inp  = 0;
  RX_outp = 0;
  TX_inp  = 0;
  TX_outp = 0;
  UCA0CTL1 |= UCSWRST;                      // Az USCI_A modul letiltása a konfigurálás idejére
  P1SEL = BIT1 + BIT2 ;                     // P1.1 = RXD, P1.2=TXD
  P1SEL2 = BIT1 + BIT2 ;                    // P1.1 = RXD, P1.2=TXD
  UCA0CTL0 = UCMODE_0;                      // UART mód, 8-bit, no parity, 1 stop bit, LSB first, aszinkron mód 
  UCA0CTL1 |= UCSSEL_2;                     // SMCLK legyen az órajel
  UCA0BR0 = (UART_baud&0x0FF0)>>4;          // Baud rate LSB beírása
  UCA0BR1 = UART_baud>>12;                  // Baud rate MSB beírása
  UCA0MCTL = (UART_baud&0x07)<<1;           // Moduláció beírása az UCBRSx bitmezõbe, UCOS16=0
  UCA0CTL1 &= ~UCSWRST;                     // Az USCI_A modul engedélyezése 
  IFG2 &= ~UCA0RXIFG;                       // USCI_A0 RX megszakításjelzõ törlése
  IE2 |= UCA0RXIE;                          // USCI_A0 RX megszakítás engedélyezése 
  IE2 &= ~UCA0TXIE;                         // USCI_A0 TX megszakítás tiltása
  __enable_interrupt();                     // A megszakítások rendszerszintû engedélyezése 
}

//----------------------------------------------------------------------
// Interrupt kiszolgáló eljárás az USCI_A0 TX megszakítások számára
//----------------------------------------------------------------------
#pragma vector=USCIAB0TX_VECTOR
__interrupt void TX_interrupt(void) {
  if (TX_inp != TX_outp) {
    UCA0TXBUF = TX_buffer[TX_outp++];      // Következõ karakter kiküldése, ha van...
    TX_outp &= (TX_BUF_SIZE - 1);          // A mutató körbefordul, ha elérte a buffer végét
  } else {
    IE2 &= ~UCA0TXIE;                      // USCI_A0 TX interrupt letiltása, ha nincs küldenivaló
  }  
}

//----------------------------------------------------------------------
// Interrupt kiszolgáló eljárás az USCI_A0 RX megszakítások számára
//----------------------------------------------------------------------
#pragma vector=USCIAB0RX_VECTOR
__interrupt void RX_interrupt(void) {
   RX_buffer[RX_inp++] = UCA0RXBUF;        // A beérkezõ karaktert a bufferbe teszi
   RX_inp &= (RX_BUF_SIZE - 1);            // A mutató körbefordul, ha elérte a buffer végét
}


