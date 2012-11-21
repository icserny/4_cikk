/* Launchpad project
 * Copyright (c) 2012 Istvan Cserny (cserny@atomki.hu)
 *
 */
#include <msp430.h>
#include <stdint.h>
/** \file
 *  Ez az állomány deklarálja azokat a függvényeket, amelyekkel az
 *  alkalmazói programból a hardveres USART kapcsolat kezelhetjük.
 *
 */

#ifndef LAUNCHPAD_HW_USART
#define LAUNCHPAD_HW_USART

//-- Makrók az adatsebesség beállításához SMCLK = 1 MHz, UCOS16 = 0 esetén
#define BPS_1200      0x3412           //UCBR0=833 UCBRS0=2, UCBRFX=0
#define BPS_2400      0x1A05           //UCBR0=416 UCBRS0=5, UCBRFX=0
#define BPS_4800      0x0D02           //UCBR0=208 UCBRS0=2, UCBRFX=0
#define BPS_9600      0x0681           //UCBR0=104 UCBRS0=1, UCBRFX=0


/* FÜGGVÉNY PROTOTÍPUSOK *********************************************/
char uart_kbhit(void);                 /**< Annak ellenõrzése, hogy van-e kiolvasatlan karakter a bemeneti bufferben **/ 
void uart_init(uint16_t usart_baud);   /**< Rendszer inicializálása (USART csatlakoztatás) **/
void uart_putc(uint8_t c);             /**< Egy karakter kiírása a soros porton keresztül */
void uart_puts(uint8_t* p_str);        /**< Karakterfüzér kiíratása */
uint8_t uart_getc(void);               /**< Egy karakter beolvasása soros portról */
void uart_outdec(int32_t data, uint8_t ndigits); /**< Decimális kiíratás adott számú tizedesjegyre */
void uart_out4hex(uint16_t t);         /**< Egy 16 bites szám kiírása hexadecimálisan */
uint16_t uart_get4hex(void);           /**< Négyjegyû hexadecimális szám beolvasása */ 
uint8_t uart_get2hex(void);            /**< Kétjegyû hexadecimális szám beolvasása */
 
#endif //LAUNCHPAD_HW_USART