/*
** Made by fabien le mentec <texane@gmail.com>
** 
** Started on  Sun Sep 20 14:08:30 2009 texane
** Last update Fri Oct  2 20:32:59 2009 texane
*/



#include <pic18fregs.h>
#include "pwm.h"
#include "srf04.h"
#include "timer.h"
#include "config.h"
#include "serial.h"
#include "adc.h"



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


/* motion */

struct move_context
{
  enum move_state
    {
      MOVE_STATE_STOP = 0,
      MOVE_STATE_FORWARD,
      MOVE_STATE_BACKWARD,
      MOVE_STATE_TURN_L,
      MOVE_STATE_TURN_R
    } state;
};


static void move_forward(void)
{
#ifdef USE_SERIAL
  serial_writei(0x00);
#endif

  do_pwm(1);
  do_epwm(-1);
}


static void move_backward(void)
{
#ifdef USE_SERIAL
  serial_writei(0x01);
#endif

  do_pwm(-1);
  do_epwm(1);
}


static void move_stop(void)
{
#ifdef USE_SERIAL
  serial_writei(0x02);
#endif

  do_pwm(0);
  do_epwm(0);
}


static void move_turn_left(void)
{
#ifdef USE_SERIAL
  serial_writei(0x03);
#endif

  do_pwm(1);
  do_epwm(1);
}


static void move_turn_right(void)
{
#ifdef USE_SERIAL
  serial_writei(0x04);
#endif

  do_pwm(-1);
  do_epwm(-1);
}


/* main */

void main(void)
{
  osc_setup();
  int_setup();

#ifdef USE_SERIAL
  serial_setup();
#endif

  srf04_setup();

 redo:
  {
    move_forward();

    while (1)
      {

#if 0

#define MIN_DISTANCE_VALUE 0x0a00 /* 20 cms */
	if (srf04_get_distance() <= MIN_DISTANCE_VALUE)
	  {
	    move_stop();
	    do_wait();

	    move_turn_left();
	    do_wait();
	    do_wait();
	    do_wait();
	    do_wait();
	    do_wait();
	    do_wait();

	    move_forward();
	  }
#else
	{
	  unsigned short value = adc_read(0);

	  /* 5 volts, 10 bits */
#define QUANTIZE_5_10(V) ((unsigned short)(((V) * 1024.f) / 5.f))

	  if (value <= QUANTIZE_5_10(2.2))
	    {
	      move_turn_right();

	      do_wait();
	      do_wait();

	      move_stop();
	    }
	  else if (value >= QUANTIZE_5_10(2.8))
	    {
	      move_turn_left();

	      do_wait();
	      do_wait();

	      move_stop();
	    }
	  else
	    {
	    }
	}
#endif
	
	do_wait();
      }
  }
  goto redo;
}
