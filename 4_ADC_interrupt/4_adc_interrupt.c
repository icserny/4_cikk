//******************************************************************************
//  MSP430G2x31 Demo - ADC10, A1 mérése, Vref+ = Vcc, P1.0 ég, ha A1 > 0.5*Vcc
//
//  Programleírás: Egycsatornás mérés az A1 mebeneten, Vcc referenciával.
//  A konverziót szoftveresen indítjuk ADC10SC '1'-be állításával - ADC10SC
//  automatikusan törlõdik a konverzió végén. ADC10 saját oszcillátora ütemezi
//  a mintavételezést (64 órajel periódus) és a konverziót.
//  A fõprogram végtelen ciklusában a mikrovezérlõ LPM0 energiatakarékos módban 
//  várakozik, amíg a konverzió be nem fejezõdik. Az ADC megszakításának
//  kiszolgálásakor ébresztjük fel újra a mikrovezérlõt. 
//  Ha A1 > 0.5*Vcc, akkor a P1.0-ra kötött LED világít, egyébként lekapcsoljuk.
//
//                MSP430G2x31
//             -----------------
//         /|\|              XIN|-
//          | |                 |
//          --|RST          XOUT|-
//            |                 |
//        >---|P1.1/A1      P1.0|-->LED
//
//  Szerzõ: D. Dang, Texas Instruments Inc., Oct 2010 (msp430g2x31_adc10_01.c)
//  Magyar változat: I. Cserny, MTA ATOMKI, Jan 2013 (4_adc_interrupt.c)
//                   IAR Embedded Workbench Version: 5.51
//******************************************************************************
#include "msp430.h"

void main(void)
{
  WDTCTL = WDTPW + WDTHOLD;                 // Letiltjuk a watchdog idõzítõt
  ADC10CTL0 = ADC10SHT_3                    // mintavétel: 64 óraütés
             | ADC10ON                      // Az ADC bekapcsolása
             | SREF_0                       // VR+ = AVCC és VR- = AVSS  
             | ADC10IE;                     // ADC10 megszakítás engedélyezése   
  ADC10CTL1 = ADC10SSEL_0 | INCH_1;         // ADC10OSC az órajel, A1 a bemenet
  ADC10AE0 |= BIT1;                         // P1.1 analóg funkció engedélyezése
  P1DIR |= BIT0;                            // P1.0 kimenet legyen (LED1)

  for (;;)
  {
    ADC10CTL0 |= ENC + ADC10SC;             // Konverzió engedélyezése és indítása
    __low_power_mode_0();                   // Alvás a konverzió végéig
    if (ADC10MEM < 0x1FF)                   // Ha A1 < 0.5*Vcc 
      P1OUT &= ~0x01;                       // P1.0 LED1-et kikapcsoljuk
    else
      P1OUT |= 0x01;                        // P1.0 LED1-et bekapcsoljuk
  }
}

//--- ADC10 megszakításkiszolgáló eljárás -----
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void)
{
  __low_power_mode_off_on_exit();           // Felébresztjük az alvó CPU-t
}
