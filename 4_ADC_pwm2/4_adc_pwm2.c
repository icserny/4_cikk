/**********************************************************************
 *  Launchpad dem�program: ADC_pwm2
 *
 * A P1.6 kimenetre k�t�tt z�ld LED f�nyerej�t impulzus-sz�less�g
 * modul�ci�val (PWM) v�ltoztatjuk, melyhez a vez�rl� jelet a P1.5
 * bemenetre kapcsolt anal�g fesz�lts�gjel adja, amit a be�p�tett
 * 10 bites ADC-vel m�rj�k meg. Az ADC konverzi� �rt�ke szabja meg 
 * a kit�lt�s m�rt�k�t. 
 *
 * Az ADC referencia fesz�lts�ge a t�pfesz�lts�g lesz (VSS, VCC).
 * A PWM frekvencia kb. 125 Hz (az alap�rtelmezett DCO frekvenci�t
 * 8-cal �s 1024-gyel osztjuk le.
 * A PWM kit�lt�s friss�t�se az ADC megszak�t�sokban t�rt�nik.
 * A konverzi�kat a CCR0 csatorna kimen� jel�vel ind�tjuk (OUT0).
 *
 * Hardver felt�telek:
 *  - MSP430 Launchpad k�rtya (MSP430G2231, MSP430G2452 vagy MSP430G2553)
 *  - Anal�g potm�ter, cs�szk�ja a P1.5-re k�tve (k�t v�ge pedig 
 *    VCC-re �s VSS-re.
 *
 *       MSP430G2231, MSP430G2452 vagy MSP430G2553
 *             -----------------
 *    Vcc  /|\|              XIN|-
 *     |    | |                 |
 *     _    --|RST          XOUT|-
 *    | |     |                 |
 *    | |<----|P1.5         P1.6|--> LED
 *    |_|     |                 |
 *     |
 *    Vss
 * Szerz�: J H Davies, 2007-08-30; IAR Kickstart version 3.42A
 * Launchpad-ra adapt�lta: I Cserny, 2012-11-23; IAR version 5.51
 **********************************************************************/
#include "io430.h"

void main (void) {
    WDTCTL = WDTPW | WDTHOLD;          // Letiltjuk a watchdog id�z�t�t
    P1OUT = 0;                         // A nem haszn�lt kimeneteket leh�zzuk
    P1DIR = ~(BIT2+BIT3+BIT5);         // Csak RXD, SW2 �s AN5 legyen bemenet
    P1SEL |= BIT6;                     // P1.6 legyen TA0.1 kimenet
//-- P1.3 bels� felh�z�s bekapcsol�sa ------
    P1OUT |= BIT3;                     // Felfel� h�zzuk, nem lefel�
    P1REN |= BIT3;                     // Bels� felh�z�s bekapcsol�sa P1.3-ra
//-- Anal�g bemenet enged�lyez�se ----------    
    ADC10AE0 = BIT5;                   // Enable analog input on AN5/P1.5
//-- Timer_A PWM 125Hz, OUT1 kimeneten, SMCLK/8 �rajellel, "Felfel� sz�ml�l�s"
    TACCR0 = BITA - 1;                 // A peri�dus ADC10-hez illeszked� (1023)
    TACCTL0 = OUTMOD_4;                // Toggle m�d (OUT0 ind�tja az ADC-t)
    TACCR1 = BIT9;                     // CCR1 indul�skor 50%-ra (512)
    TACCTL1 = OUTMOD_7;                // Reset/set PWM m�d be�ll�t�sa
    TACTL = TASSEL_2 |                 // SMCLK az �rajel forr�sa
                ID_3 |                 // 1:8 oszt�s bekapcsol�sa
                MC_1 |                 // Felfel� sz�ml�l� m�d
                TACLR;                 // TAR t�rl�se
//-- ADC  konfigur�l�s ---------------------
    ADC10CTL0 = ADC10SHT_0             // mintav�tel: 4 �ra�t�s
               | ADC10ON               // Az ADC bekapcsol�sa
               | SREF_0                // VR+ = AVCC �s VR- = AVSS
               | ADC10IE;              // Programmegszak�t�s enged�lyez�se
    ADC10CTL1 = INCH_5                 // A5 csatorna kiv�laszt�sa
               | SHS_2                 // Hatdveres triggerel�s (TA0.0)     
               | ADC10SSEL_0           // ADC10OSC adja az �rajelet (~5 MHz)
               | CONSEQ_2;             // Ism�telt egycsatorn�s konverzi�
    ADC10CTL0 |= ENC;                  // A konverzi� enged�lyez�se
        __low_power_mode_0();          // A tov�bbi tev�kenys�g megszak�t�sban
}

//----------------------------------------------------------------------
// ADC10 megszak�t�s kiszolg�l�sa: PWM kit�lt�s aktualiz�l�sa
//----------------------------------------------------------------------
#pragma vector = ADC10_VECTOR
__interrupt void ADC10_ISR (void)      // A jelz�bit automatikusan t�rl�dik
{
    TACCR1 = ADC10MEM;                 // PWM kit�lt�s = ADC konverzi� eredm�nye
}