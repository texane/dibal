/*
** Made by fabien le mentec <texane@gmail.com>
** 
** Started on  Sat Oct  3 16:14:50 2009 texane
** Last update Sat Oct  3 17:32:25 2009 texane
*/



#if 1 /* unit testing */



#include <pic18fregs.h>
#include "config.h"
#include "move.h"
#include "light.h"
#include "sched.h"
#include "int.h"
#include "adc.h"
#include "osc.h"



static volatile unsigned char is_done;

static void wait_handler(void)
{
  is_done = 1;
}


static unsigned int delta_to_freq(unsigned short delta)
{
#define QUADRAN_FREQ 2
  return (delta * QUADRAN_FREQ) / ADC_QUANTIZE_5_10(2.5);
}


static void wait_and_stop(unsigned int freq)
{
  sched_timer_t* timer;

  is_done = 0;

  timer = sched_add_timer(freq, wait_handler, 1);

  while (!is_done);

  move_stop();

  sched_del_timer(timer);
}


static void wait_and_stop2(unsigned int cur_delta)
{
  /* small version. cur_delta <= 512. */

  unsigned int i;

  for (i = 0; i < cur_delta * 100; ++i)
    __asm nop __endasm;

  move_stop();
}


static void detect_light(void)
{
  void (*rotate)(void);
  unsigned short cur_level;
  unsigned short prev_delta = ADC_MAX_VALUE;
  unsigned short cur_delta;
  unsigned int freq;

  while (1)
    {
      cur_level = adc_read(LIGHT_ADC_CHANNEL);

      if (cur_level >= ADC_QUANTIZE_5_10(2.3))
	if (cur_level <= ADC_QUANTIZE_5_10(2.7))
	  return ;

      if (cur_level >= ADC_QUANTIZE_5_10(2.5))
	{
	  cur_delta = cur_level - ADC_QUANTIZE_5_10(2.5);
	  rotate = move_rotate_left;
	}
      else
	{
	  cur_delta = ADC_QUANTIZE_5_10(2.5) - cur_level;
	  rotate = move_rotate_right;
	}

      if (prev_delta < cur_delta)
	return ;

      freq = delta_to_freq(cur_delta);

      prev_delta = cur_delta;

      rotate();

      if (freq)
	wait_and_stop(freq);
      else
	wait_and_stop2(cur_delta);
    }
}


void main(void)
{
  osc_setup();
  int_setup();

  sched_setup();

  sched_enable();

  {
  redo:
    detect_light();
    goto redo;
  }
}


#endif /* unit testing */
