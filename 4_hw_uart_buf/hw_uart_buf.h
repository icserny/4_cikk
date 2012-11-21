/* Launchpad project
 * Copyright (c) 2012 Istvan Cserny (cserny@atomki.hu)
 *
 */
#include <msp430.h>
#include <stdint.h>
/** \file
 *  Ez az �llom�ny deklar�lja azokat a f�ggv�nyeket, amelyekkel az
 *  alkalmaz�i programb�l a hardveres USART kapcsolat kezelhetj�k.
 *
 */

#ifndef LAUNCHPAD_HW_USART
#define LAUNCHPAD_HW_USART

//-- Makr�k az adatsebess�g be�ll�t�s�hoz SMCLK = 1 MHz, UCOS16 = 0 eset�n
#define BPS_1200      0x3412           //UCBR0=833 UCBRS0=2, UCBRFX=0
#define BPS_2400      0x1A05           //UCBR0=416 UCBRS0=5, UCBRFX=0
#define BPS_4800      0x0D02           //UCBR0=208 UCBRS0=2, UCBRFX=0
#define BPS_9600      0x0681           //UCBR0=104 UCBRS0=1, UCBRFX=0


/* F�GGV�NY PROTOT�PUSOK *********************************************/
char uart_kbhit(void);                 /**< Annak ellen�rz�se, hogy van-e kiolvasatlan karakter a bemeneti bufferben **/ 
void uart_init(uint16_t usart_baud);   /**< Rendszer inicializ�l�sa (USART csatlakoztat�s) **/
void uart_putc(uint8_t c);             /**< Egy karakter ki�r�sa a soros porton kereszt�l */
void uart_puts(uint8_t* p_str);        /**< Karakterf�z�r ki�rat�sa */
uint8_t uart_getc(void);               /**< Egy karakter beolvas�sa soros portr�l */
void uart_outdec(int32_t data, uint8_t ndigits); /**< Decim�lis ki�rat�s adott sz�m� tizedesjegyre */
void uart_out4hex(uint16_t t);         /**< Egy 16 bites sz�m ki�r�sa hexadecim�lisan */
uint16_t uart_get4hex(void);           /**< N�gyjegy� hexadecim�lis sz�m beolvas�sa */ 
uint8_t uart_get2hex(void);            /**< K�tjegy� hexadecim�lis sz�m beolvas�sa */
 
#endif //LAUNCHPAD_HW_USART