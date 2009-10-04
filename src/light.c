/*
** Made by fabien le mentec <texane@gmail.com>
** 
** Started on  Sat Oct  3 16:14:50 2009 texane
** Last update Sun Oct  4 04:24:34 2009 texane
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
  return (QUADRAN_FREQ * ADC_QUANTIZE_5_10(2.5)) / delta;
}


static unsigned int freq_to_delta(unsigned int freq)
{
  return (QUADRAN_FREQ * ADC_QUANTIZE_5_10(2.5)) / freq;
}


static void wait_and_stop(unsigned short delta)
{
  /* waiting is wrapped inside a loop since the
     delta to be waited for has to be translated
     into a corresponding scheduler timer freq
     and may be longer than the max sched freq
  */

  sched_timer_t* timer;
  unsigned int tmp;

  timer = sched_add_timer(0, wait_handler, 0);

  while (delta)
    {
      /* turn the remaining delta into a freq.
	 resulting freq cannot be < 2 since
	 delta <= 512 and go decreasing.
      */

      tmp = delta_to_freq(delta);
      if (tmp > SCHED_MAX_FREQ)
	tmp = SCHED_MAX_FREQ;

      /* set the new timer freq */

      sched_set_timer_freq(timer, tmp);

      /* substract the delta that being waited for */

      tmp = freq_to_delta(tmp);
      if (tmp > delta)
	tmp = delta;

      delta -= tmp;

      /* wait for the timer to fire */

      is_done = 0;
      sched_enable_timer(timer);
      while (!is_done) ;
      sched_disable_timer(timer);
    }

  move_stop();

  sched_del_timer(timer);
}


static void detect_light(void)
{
  void (*rotate)(void);
  unsigned short cur_level;
  unsigned short prev_delta = ADC_MAX_VALUE;
  unsigned short cur_delta;

  while (1)
    {
      cur_level = adc_read(LIGHT_ADC_CHANNEL);

      if (cur_level >= ADC_QUANTIZE_5_10(2.35))
	if (cur_level <= ADC_QUANTIZE_5_10(2.65))
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

      rotate();

      wait_and_stop(cur_delta);

      prev_delta = cur_delta;
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

    {
      unsigned int i;

      for (i = 0; i < 40000; ++i)
	;

      for (i = 0; i < 40000; ++i)
	;
    }

    goto redo;
  }
}


#endif /* unit testing */
