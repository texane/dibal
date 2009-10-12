/*
** Made by fabien le mentec <texane@gmail.com>
** 
** Started on  Sun Jun  7 09:36:34 2009 texane
** Last update Mon Oct 12 10:50:30 2009 texane
*/



#include <pic18fregs.h>
#include "srf04.h"



/* pic18f4550 timer1 */

static unsigned int tmr1_read(void)
{
  return (TMR1H << 8) | TMR1L;
}


static void tmr1_stop(void)
{
  T1CONbits.TMR1ON = 0;
  PIR1bits.TMR1IF = 0;
}


static void tmr1_start(unsigned int counter)
{
  T1CONbits.TMR1ON = 0; /* disable timer1 */

  PIE1bits.TMR1IE = 0;
  PIR1bits.TMR1IF = 0;

  T1CONbits.RD16 = 0; /* read/write in 2 8 bits operations */

  T1CONbits.T1CKPS0 = 0; /* 1:1 prescaler */  
  T1CONbits.T1CKPS1 = 0; /* 1:1 prescaler */  
  T1CONbits.T1OSCEN = 0; /* t1 osc shut off */
  T1CONbits.TMR1CS = 0; /* internal clock */

  TMR1H = (counter >> 8) & 0xff;
  TMR1L = (counter >> 0) & 0xff;

  T1CONbits.TMR1ON = 1; /* enable timer1 */
}


static void tmr1_loop(unsigned int usecs)
{
  /* the timer is set to be incremented at
     each insn cycle. an instruction cycle
     occurs at fosc / 4. at a fosc of 8mhz
     there are 2 insns per micro seconds.
   */

  tmr1_start(0xffff - usecs * 2);

  /* wait for interrupt */

  while (1)
    {
      if (PIR1bits.TMR1IF)
	{
	  PIR1bits.TMR1IF = 0;
	  break;
	}
    }

  /* disable timer */

  tmr1_stop();
}


/* pins */

#define SRF04_TRIGGER_PIN PORTBbits.RB0
#define SRF04_ECHO_PIN PORTBbits.RB1


/* exported */

void srf04_setup(void)
{
  TRISBbits.TRISB0 = 0;
  TRISBbits.TRISB1 = 1;

  ADCON1bits.PCFG0 = 1;
  ADCON1bits.PCFG1 = 1;
  ADCON1bits.PCFG2 = 1;
  ADCON1bits.PCFG3 = 1;
}


unsigned int srf04_get_distance(unsigned int limit)
{
  unsigned int counter = 0xffff - limit;
  unsigned int dist = SRF04_INVALID_DISTANCE;

  SRF04_ECHO_PIN = 0;

  /* trigger pulse 10us (8mhz) */

  SRF04_TRIGGER_PIN = 0;
  SRF04_TRIGGER_PIN = 1;

  tmr1_loop(10);

  SRF04_TRIGGER_PIN = 0;

  while (!SRF04_ECHO_PIN)
    ;

  /* set timer to the limit value. if a timeroverflow
     occurs prior the pin level changes, then limit
     is reached and invalid distance returned */

  tmr1_start(counter);

  while (1)
    {
      if (!SRF04_ECHO_PIN)
	{
	  dist = tmr1_read() - counter;
	  break;
	}

      /* limit reached */

      if (PIR1bits.TMR1IF)
	break;
    }

  tmr1_stop();

  return dist;
}
