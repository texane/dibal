/*
** Made by fabien le mentec <texane@gmail.com>
** 
** Started on  Sat Oct  3 13:23:48 2009 texane
** Last update Sat Oct  3 15:56:52 2009 texane
*/



#include "pwm.h"



/* exported */

void move_go_forwrad(void)
{
  do_pwm(1);
  do_epwm(-1);
}


void move_go_backward(void)
{
  do_pwm(-1);
  do_epwm(1);
}


void move_rotate_right(void)
{
  do_pwm(-1);
  do_epwm(-1);
}


void move_rotate_left(void)
{
  do_pwm(1);
  do_epwm(1);
}


void move_stop(void)
{
  stop_pwm();
  stop_epwm();
}



#if 1 /* unit testing */


#include <pic18fregs.h>
#include "sched.h"
#include "config.h"
#include "int.h"
#include "adc.h"
#include "osc.h"



static volatile unsigned int move_ticks;


static void move_handler(void)
{
  ++move_ticks;
}


static volatile unsigned char is_done;

static void wait_handler(void)
{
  is_done = 1;
}


static void wait_500ms(sched_timer_t* timer)
{
  is_done = 0;

  sched_reset_timer(timer);
  sched_enable_timer(timer);

  while (!is_done)
    ;

  sched_disable_timer(timer);
}


static void learn_octant(void)
{
  sched_timer_t* timer;
  unsigned int count;
  unsigned short ref_level = 0;
  unsigned short min_level = 0;
  unsigned short max_level = 0;
  unsigned short cur_level;

  /* find a stable light level */

  timer = sched_add_timer(2, wait_handler, 0);

  for (count = 0; count < 10; ++count)
    {
      move_rotate_right();
      wait_500ms(timer);
      move_stop();

#define LIGHT_ADC_CHANNEL 0
      ref_level = adc_read(LIGHT_ADC_CHANNEL);
      wait_500ms(timer);
      cur_level = adc_read(LIGHT_ADC_CHANNEL);

#define QUANTIZE_5_10(V) ((unsigned short)(((V) * 1024.f) / 5.f))
      if (ref_level > QUANTIZE_5_10(0.02))
	min_level = ref_level - QUANTIZE_5_10(0.02);
      else
	min_level = 0;

      max_level = ref_level + QUANTIZE_5_10(0.02);

      if ((cur_level >= min_level) && (cur_level <= max_level))
	break;
    }

  sched_del_timer(timer);

  if (count == 5)
    return ;

  /* learn ticks per octant */

  move_ticks = 0;

  timer = sched_add_timer(10, move_handler, 1);

  move_rotate_left();

  while (1)
    {
      if (move_ticks < 3)
	continue;

      cur_level = adc_read(LIGHT_ADC_CHANNEL);

      if ((cur_level >= min_level) && (cur_level <= max_level))
	break;
    }

  move_stop();

  sched_del_timer(timer);
}


void main(void)
{
  osc_setup();
  int_setup();

  sched_setup();

  sched_enable();

  {
    learn_octant();

  redo:
    goto redo;
  }
}


#endif /* unit testing */
