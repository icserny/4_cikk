/**********************************************************************
 *  Launchpad demóprogram: ADC_pwm2
 *
 * A P1.6 kimenetre kötött zöld LED fényerejét impulzus-szélesség
 * modulációval (PWM) változtatjuk, melyhez a vezérlõ jelet a P1.5
 * bemenetre kapcsolt analóg feszültségjel adja, amit a beépített
 * 10 bites ADC-vel mérjük meg. Az ADC konverzió értéke szabja meg 
 * a kitöltés mértékét. 
 *
 * Az ADC referencia feszültsége a tápfeszültség lesz (VSS, VCC).
 * A PWM frekvencia kb. 125 Hz (az alapértelmezett DCO frekvenciát
 * 8-cal és 1024-gyel osztjuk le.
 * A PWM kitöltés frissítése az ADC megszakításokban történik.
 * A konverziókat a CCR0 csatorna kimenõ jelével indítjuk (OUT0).
 *
 * Hardver feltételek:
 *  - MSP430 Launchpad kártya (MSP430G2231, MSP430G2452 vagy MSP430G2553)
 *  - Analóg potméter, csúszkája a P1.5-re kötve (két vége pedig 
 *    VCC-re és VSS-re.
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
 * Szerzõ: J H Davies, 2007-08-30; IAR Kickstart version 3.42A
 * Launchpad-ra adaptálta: I Cserny, 2012-11-23; IAR version 5.51
 **********************************************************************/
#include "io430.h"

void main (void) {
    WDTCTL = WDTPW | WDTHOLD;          // Letiltjuk a watchdog idõzítõt
    P1OUT = 0;                         // A nem használt kimeneteket lehúzzuk
    P1DIR = ~(BIT2+BIT3+BIT5);         // Csak RXD, SW2 és AN5 legyen bemenet
    P1SEL |= BIT6;                     // P1.6 legyen TA0.1 kimenet
//-- P1.3 belsõ felhúzás bekapcsolása ------
    P1OUT |= BIT3;                     // Felfelé húzzuk, nem lefelé
    P1REN |= BIT3;                     // Belsõ felhúzás bekapcsolása P1.3-ra
//-- Analóg bemenet engedélyezése ----------    
    ADC10AE0 = BIT5;                   // Enable analog input on AN5/P1.5
//-- Timer_A PWM 125Hz, OUT1 kimeneten, SMCLK/8 órajellel, "Felfelé számlálás"
    TACCR0 = BITA - 1;                 // A periódus ADC10-hez illeszkedõ (1023)
    TACCTL0 = OUTMOD_4;                // Toggle mód (OUT0 indítja az ADC-t)
    TACCR1 = BIT9;                     // CCR1 induláskor 50%-ra (512)
    TACCTL1 = OUTMOD_7;                // Reset/set PWM mód beállítása
    TACTL = TASSEL_2 |                 // SMCLK az órajel forrása
                ID_3 |                 // 1:8 osztás bekapcsolása
                MC_1 |                 // Felfelé számláló mód
                TACLR;                 // TAR törlése
//-- ADC  konfigurálás ---------------------
    ADC10CTL0 = ADC10SHT_0             // mintavétel: 4 óraütés
               | ADC10ON               // Az ADC bekapcsolása
               | SREF_0                // VR+ = AVCC és VR- = AVSS
               | ADC10IE;              // Programmegszakítás engedélyezése
    ADC10CTL1 = INCH_5                 // A5 csatorna kiválasztása
               | SHS_2                 // Hatdveres triggerelés (TA0.0)     
               | ADC10SSEL_0           // ADC10OSC adja az órajelet (~5 MHz)
               | CONSEQ_2;             // Ismételt egycsatornás konverzió
    ADC10CTL0 |= ENC;                  // A konverzió engedélyezése
        __low_power_mode_0();          // A további tevékenység megszakításban
}

//----------------------------------------------------------------------
// ADC10 megszakítás kiszolgálása: PWM kitöltés aktualizálása
//----------------------------------------------------------------------
#pragma vector = ADC10_VECTOR
__interrupt void ADC10_ISR (void)      // A jelzõbit automatikusan törlõdik
{
    TACCR1 = ADC10MEM;                 // PWM kitöltés = ADC konverzió eredménye
}