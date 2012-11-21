/* Launchpad project
 * Copyright (c) 2012 Istvan Cserny (cserny@atomki.hu)
 *
 */
#include <msp430.h>
#include <stdint.h>
#include "hw_uart_buf.h"
/** \file
 *  Ez az �llom�ny defini�lja azokat a f�ggv�nyeket, amelyekkel az
 *  alkalmaz�i programb�l a hardveres UART kapcsolat kezelhetj�k.
 */

/* GLOB�LIS V�LTOZ�K az UART kommunik�ci�hoz **************************/
#define RX_BUF_SIZE 8                  // UART vev�oldali buffer m�rete (2 hatv�nya legyen!)
#define TX_BUF_SIZE 8                  // UART  ad�oldali buffer m�rete (2 hatv�nya legyen!)

uint8_t RX_inp;                        // UART vev�oldali buffer bemeneti mutat�
uint8_t RX_outp;                       // UART vev�oldali buffer kimeneti mutat�
uint8_t TX_inp;                        // UART  ad�oldali buffer bemeneti mutat�
uint8_t TX_outp;                       // UART  ad�oldali buffer kimeneti mutat�
uint8_t RX_buffer[RX_BUF_SIZE];	       // UART vev�oldali buffer (ebb�l olvasunk)
uint8_t TX_buffer[TX_BUF_SIZE];	       // UART  ad�oldali buffer (ebbe �runk)

/** Annak ellen�rz�se, hogy van-e kiolvasatlan karakter a bemeneti bufferben
 *  A vizsg�lathoz elegend� �sszehasonl�tani a bemeneti �s a kimeneti mutat�kat 
 */
char uart_kbhit(void) {
  return (char)(RX_inp!=RX_outp);
}

/** Egy karakter ki�r�sa a kimeneti bufferbe.
 *  Blokkol� t�pus� f�ggv�ny, addig v�rakozik, am�g 
 *  szabad helyet nem tal�l a kimenti bufferben.
 *  \param c a ki�rand� karakter
 *  \return a ki�rt karakter k�dja int t�puss� konvert�lva
 */
void uart_putc(uint8_t c) {
  uint8_t x;
  //-- Kritikus tartom�ny, vigy�zz a buffer konkurens kezel�s�re!
  TX_buffer[TX_inp] = c;
  x = (TX_inp+1) & (TX_BUF_SIZE-1);    //TX_inp leend� �rt�ke
  //-- Kritikus tartom�ny v�ge --------
  while(x == TX_outp);                 //V�runk, ha utol�rt�k a kimeneti mutat�t
  TX_inp = x;
  IE2 |= UCA0TXIE;                    //Tx megszak�t�s enged�lyez�se 
}


/** Beolvas egy karaktert az input bufferb�l, vagy v�rakozik, ha az �res.
 * \return c, a beolvasott karakter k�dja (uint8_t t�pus�)
 */
uint8_t uart_getc(void) {
  uint8_t c;
  while(RX_inp == RX_outp);            //V�runk egy karakter be�rkez�s�re
  //-- Kritikus tartom�ny, vigy�zz a buffer konkurens kezel�s�re!
  c =RX_buffer[RX_outp];               //A soron k�vetkez� karakter el�v�tele
  RX_outp=(RX_outp+1)&(RX_BUF_SIZE-1); //A kiv�teli mutat� l�ptet�se
  //-- Kritikus tartom�ny v�ge --------
  return (c);
} 


/** Ki�r egy null�val lez�rt sz�veget a kimeneti bufferbe.
 *  Ez a f�ggv�ny a blokkol� t�pus� uart_putc() f�ggv�nyt h�vja!  
 *  Sorv�ge eset�n automatukusan beiktat egy Carriage Return karaktert.
 *  \param p_str mutat�, a ki�ran� sz�veghez
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

/** Decim�lis ki�rat�s adott sz�m� tizedesjegyre. 
 * \param data a ki�rand� sz�m (el�jelesen)
 * \param ndigits a ki�rand� tizedesek sz�ma
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

/** N�gy hexadecim�lis sz�mjegy beolvas�sa �s �talak�t�sa uint16 t�pusra.
 * Ez a f�ggv�ny blokkol� t�pus�, addig v�r, am�g be nem �rkezik n�gy kararakter,
 * amelyeket az uart_putc() elj�r�ssal azonnal vissza is t�kr�z�nk.
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

/** Egy el�jel n�lk�li eg�sz sz�mot hexadecim�lis form�ban ki�r
 * a kimeneti bufferbe. Ez a f�ggv�ny megh�vja a blokkol� t�pus�
 *  _UART_putc() f�ggv�nyt! 
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

/** Egy el�jel n�lk�li eg�sz sz�mot hexadecim�lis form�ban ki�r
 * a kimeneti bufferbe. Ez a f�ggv�ny megh�vja a blokkol� t�pus�
 *  uart_putc() f�ggv�nyt! 
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

/** K�t hexadecim�lis sz�mjegy beolvas�sa �s �talak�t�sa uint8 t�pusra
 *  Ez a f�ggv�ny blokkol� t�pus�, addig v�r, am�g be nem �rkezik k�t kararakter,
 *  amelyeket az uart_putc() elj�r�ssal vissza is t�kr�z�nk. 
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

/** UART m�dba konfigur�lja az USCI0_A modult
 *  �s null�zza a buffer mutat�kat
 *  \param UART_baud a baudrate be�ll�t�s�hoz el�re defini�lt
 *  param�ter (l�sd usart.h-ban)
 */
void uart_init(uint16_t UART_baud) {
//-- UART TX �s RX buffer mutat�k inicializ�l�sa
  RX_inp  = 0;
  RX_outp = 0;
  TX_inp  = 0;
  TX_outp = 0;
  UCA0CTL1 |= UCSWRST;                      // Az USCI_A modul letilt�sa a konfigur�l�s idej�re
  P1SEL = BIT1 + BIT2 ;                     // P1.1 = RXD, P1.2=TXD
  P1SEL2 = BIT1 + BIT2 ;                    // P1.1 = RXD, P1.2=TXD
  UCA0CTL0 = UCMODE_0;                      // UART m�d, 8-bit, no parity, 1 stop bit, LSB first, aszinkron m�d 
  UCA0CTL1 |= UCSSEL_2;                     // SMCLK legyen az �rajel
  UCA0BR0 = (UART_baud&0x0FF0)>>4;          // Baud rate LSB be�r�sa
  UCA0BR1 = UART_baud>>12;                  // Baud rate MSB be�r�sa
  UCA0MCTL = (UART_baud&0x07)<<1;           // Modul�ci� be�r�sa az UCBRSx bitmez�be, UCOS16=0
  UCA0CTL1 &= ~UCSWRST;                     // Az USCI_A modul enged�lyez�se 
  IFG2 &= ~UCA0RXIFG;                       // USCI_A0 RX megszak�t�sjelz� t�rl�se
  IE2 |= UCA0RXIE;                          // USCI_A0 RX megszak�t�s enged�lyez�se 
  IE2 &= ~UCA0TXIE;                         // USCI_A0 TX megszak�t�s tilt�sa
  __enable_interrupt();                     // A megszak�t�sok rendszerszint� enged�lyez�se 
}

//----------------------------------------------------------------------
// Interrupt kiszolg�l� elj�r�s az USCI_A0 TX megszak�t�sok sz�m�ra
//----------------------------------------------------------------------
#pragma vector=USCIAB0TX_VECTOR
__interrupt void TX_interrupt(void) {
  if (TX_inp != TX_outp) {
    UCA0TXBUF = TX_buffer[TX_outp++];      // K�vetkez� karakter kik�ld�se, ha van...
    TX_outp &= (TX_BUF_SIZE - 1);          // A mutat� k�rbefordul, ha el�rte a buffer v�g�t
  } else {
    IE2 &= ~UCA0TXIE;                      // USCI_A0 TX interrupt letilt�sa, ha nincs k�ldenival�
  }  
}

//----------------------------------------------------------------------
// Interrupt kiszolg�l� elj�r�s az USCI_A0 RX megszak�t�sok sz�m�ra
//----------------------------------------------------------------------
#pragma vector=USCIAB0RX_VECTOR
__interrupt void RX_interrupt(void) {
   RX_buffer[RX_inp++] = UCA0RXBUF;        // A be�rkez� karaktert a bufferbe teszi
   RX_inp &= (RX_BUF_SIZE - 1);            // A mutat� k�rbefordul, ha el�rte a buffer v�g�t
}


