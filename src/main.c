/*
** Made by fabien le mentec <texane@gmail.com>
** 
** Started on  Sun Sep 20 14:08:30 2009 texane
** Last update Sun Sep 27 19:51:56 2009 texane
*/



#include <pic18fregs.h>
#include "srf04.h"
#include "timer.h"
#include "config.h"
#include "serial.h"



/* oscillator */

static void osc_setup(void)
{
  /* 8Mhz
   */

  OSCCONbits.IRCF = 7;

  /* internal osc used
   */

  OSCCONbits.SCS = 2;

  /* idle mode enable so that peripherals are
     clocked with SCS when cpu is sleeping.
   */

  OSCCONbits.IDLEN = 1;

  /* wait for stable freq
   */

  while (!OSCCONbits.IOFS)
    ;
}


/* adc module */

static void adc_setup(void)
{
  /* an0 channel
   */

  ADCON0bits.CHS0 = 0;

  /* a[0-7] analog inputs
   */

  ADCON1bits.PCFG0 = 1;
  ADCON1bits.PCFG1 = 1;
  ADCON1bits.PCFG2 = 1;
  ADCON1bits.PCFG3 = 0;

#if 1

  /* vref-: vss, vref+: vdd
   */

  ADCON1bits.VCFG0 = 0;
  ADCON1bits.VCFG1 = 0;

#else

  /* vref-: an2, vref+: an3
   */

  ADCON1bits.VCFG0 = 1;
  ADCON1bits.VCFG1 = 1;

#endif

  /* result format left aligned
   */

  ADCON2bits.ADFM = 0;

  /* acquisition time. osc is 8 Mhz.
   */

  ADCON2bits.ACQT0 = 1;
  ADCON2bits.ACQT1 = 0;
  ADCON2bits.ACQT2 = 0;

  /* conversion clock: fosc / 2
   */

  ADCON2bits.ADCS0 = 0;
  ADCON2bits.ADCS1 = 0;
  ADCON2bits.ADCS2 = 0;

  /* turn on analog module
   */

  ADCON0bits.ADON = 1;
}


static unsigned short adc_read(void)
{
  unsigned short i;

  /* wait required time, for the
     analog value to be captured
   */

  for (i = 0; i < 0x1000; ++i)
    __asm nop __endasm;

  /* start the conversion
   */

  ADCON0bits.GO = 1;

  /* wait for conversion
   */

  while (!ADCON0bits.GO)
    ;

  return (((unsigned short)ADRESH << 8) | ADRESL) & 0x3ff;
}


#if 0

static int adc_peek(unsigned short* value)
{
  /* if a conversion occured, get the value
     and the
   */
}


static void adc_handle_interrupt(void)
{
  if ()
    ;
}

#endif


#if 0 /* event handling */

struct state
{
  
};


struct event
{
};


static void on_event(struct event* event)
{
  struct state state;

  switch (event->type)
    {
    case EVENT_TYPE_ADC:
      {
      }
      break;
	

    }
}

#endif


/* interrupt */

void on_low_interrupt(void) __interrupt 2;


void on_low_interrupt(void) __interrupt 2
{
  srf04_handle_interrupt();
  timer_handle_interrupt();
}


static void int_setup(void)
{
  /* disable high prio ints */

  RCONbits.IPEN = 0;
  INTCONbits.PEIE = 1;
  INTCONbits.GIE = 1;
  INTCON2bits.RBIP = 0;
}


static void do_wait(void)
{
  unsigned int i;
  unsigned int j;

  for (j = 0; j < 10; ++j)
    for (i = 0; i < 10000; ++i)
      ;
}


/* main */

void main(void)
{
  osc_setup();
  int_setup();
  srf04_setup();
  serial_setup();

  serial_writei(0xf00f);

 redo:
  {
    unsigned int old_d = 0xa5a5;
    unsigned int new_d;

    while (1)
      {
#define MIN_DISTANCE_VALUE 0x0a00 /* 20 cms */

	new_d = srf04_get_distance();
	if (new_d != old_d)
	  {
	    serial_writei(new_d);
	    old_d = new_d;
	  }

	if (new_d <= MIN_DISTANCE_VALUE)
	  {
	    turn_left();
	  }

	do_wait();
      }
  }
  goto redo;

}
