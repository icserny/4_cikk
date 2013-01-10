//******************************************************************************
//  MSP430G2x31 Demo - ADC10, A1 m�r�se, Vref+ = Vcc, P1.0 �g, ha A1 > 0.5*Vcc
//
//  Programle�r�s: Egycsatorn�s m�r�s az A1 mebeneten, Vcc referenci�val.
//  A konverzi�t szoftveresen ind�tjuk ADC10SC '1'-be �ll�t�s�val - ADC10SC
//  automatikusan t�rl�dik a konverzi� v�g�n. ADC10 saj�t oszcill�tora �temezi
//  a mintav�telez�st (64 �rajel peri�dus) �s a konverzi�t.
//  A f�program v�gtelen ciklus�ban a mikrovez�rl� LPM0 energiatakar�kos m�dban 
//  v�rakozik, am�g a konverzi� be nem fejez�dik. Az ADC megszak�t�s�nak
//  kiszolg�l�sakor �bresztj�k fel �jra a mikrovez�rl�t. 
//  Ha A1 > 0.5*Vcc, akkor a P1.0-ra k�t�tt LED vil�g�t, egy�bk�nt lekapcsoljuk.
//
//                MSP430G2x31
//             -----------------
//         /|\|              XIN|-
//          | |                 |
//          --|RST          XOUT|-
//            |                 |
//        >---|P1.1/A1      P1.0|-->LED
//
//  Szerz�: D. Dang, Texas Instruments Inc., Oct 2010 (msp430g2x31_adc10_01.c)
//  Magyar v�ltozat: I. Cserny, MTA ATOMKI, Jan 2013 (4_adc_interrupt.c)
//                   IAR Embedded Workbench Version: 5.51
//******************************************************************************
#include "msp430.h"

void main(void)
{
  WDTCTL = WDTPW + WDTHOLD;                 // Letiltjuk a watchdog id�z�t�t
  ADC10CTL0 = ADC10SHT_3                    // mintav�tel: 64 �ra�t�s
             | ADC10ON                      // Az ADC bekapcsol�sa
             | SREF_0                       // VR+ = AVCC �s VR- = AVSS  
             | ADC10IE;                     // ADC10 megszak�t�s enged�lyez�se   
  ADC10CTL1 = ADC10SSEL_0 | INCH_1;         // ADC10OSC az �rajel, A1 a bemenet
  ADC10AE0 |= BIT1;                         // P1.1 anal�g funkci� enged�lyez�se
  P1DIR |= BIT0;                            // P1.0 kimenet legyen (LED1)

  for (;;)
  {
    ADC10CTL0 |= ENC + ADC10SC;             // Konverzi� enged�lyez�se �s ind�t�sa
    __low_power_mode_0();                   // Alv�s a konverzi� v�g�ig
    if (ADC10MEM < 0x1FF)                   // Ha A1 < 0.5*Vcc 
      P1OUT &= ~0x01;                       // P1.0 LED1-et kikapcsoljuk
    else
      P1OUT |= 0x01;                        // P1.0 LED1-et bekapcsoljuk
  }
}

//--- ADC10 megszak�t�skiszolg�l� elj�r�s -----
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void)
{
  __low_power_mode_off_on_exit();           // Fel�bresztj�k az alv� CPU-t
}
