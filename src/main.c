/*
** Made by fabien le mentec <texane@gmail.com>
** 
** Started on  Sun Sep 20 14:08:30 2009 texane
** Last update Sat Oct  3 13:44:20 2009 texane
*/



#include <pic18fregs.h>
#include "config.h"
#include "int.h"
#include "osc.h"
#include "pwm.h"
#include "srf04.h"
#include "timer.h"
#include "serial.h"
#include "adc.h"



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
