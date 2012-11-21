/*******************************************************************************
*                     Demo Program for LaunchPad Board                         *
*             Using S1 at nRST/NMI Pin as Input for Application                *
*                                OCY Oct 2010                                  *
*******************************************************************************/
#include <msp430.h>
#define FLIP_HOLD (0x3300 | WDTHOLD) /* flip HOLD while preserving other bits */
//
#define S1_MAKE P1OUT |= BIT0 /* turn on LED1 for demo */
#define S1_BREAK P1OUT &= ~BIT0 /* turn off LED1 for demo */
//
void main (void)
{
  // We use the red LED1 at P1.0 to demostrate that we can detect
  //    the switch s1 at nRST/NMI pin as an input
  P1OUT &= ~BIT0;
  P1DIR |= BIT0;
  // The WDT will be used exclusively to debounce s1
  WDTCTL = WDTPW | WDTHOLD | WDTNMIES | WDTNMI;
  IFG1 &= ~(WDTIFG | NMIIFG);
  IE1 |= WDTIE | NMIIE;
  // The CPU is free to do other tasks, or go to sleep
  __bis_SR_register(CPUOFF | GIE);
}
//==============================================================================
// isr to detect make/break of s1 at the nRST/NMI pin
#pragma vector = NMI_VECTOR
__interrupt void nmi_isr(void)
{
  if (IFG1 & NMIIFG) // nmi caused by nRST/NMI pin
  {
    IFG1 &= ~NMIIFG;
    if (WDTCTL & WDTNMIES) // falling edge detected
    {
      S1_MAKE; // tell the rest of the world that s1 is depressed
      WDTCTL = WDT_MDLY_32 | WDTNMI; // debounce and detect rising edge
    }
    else // rising edge detected
    {
      S1_BREAK; // tell the rest of the world that s1 is released
      WDTCTL = WDT_MDLY_32 | WDTNMIES | WDTNMI; // debounce and detect falling edge
    }
  } // Note that NMIIE is cleared; wdt_isr will set NMIIE 32msec later
  else {/* add code here to handle other kinds of nmi, if any */}
}
//==============================================================================
// WDT is used exclusively to debounce s1 and re-enable NMIIE
#pragma vector = WDT_VECTOR
__interrupt void wdt_isr(void)
{
  WDTCTL ^= FLIP_HOLD; // Flip the HOLD bit only, others remain unchanged
  IFG1 &= NMIIFG; // It may have been set by switch bouncing, clear it
  IE1 |= NMIIE; // Now we can enable nmi to detect the next edge
}
//==============================================================================