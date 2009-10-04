/*
** Made by fabien le mentec <texane@gmail.com>
** 
** Started on  Fri Oct  2 21:28:28 2009 texane
** Last update Sun Oct  4 03:59:31 2009 texane
*/



#include <pic18fregs.h>
#include "osc.h"
#include "sched.h"



/* sched software timer */

struct sched_timer
{
  unsigned int current_countdown;
  unsigned int saved_countdown;

#define TIMER_FLAG_IS_ALLOCATED (1 << 0)
#define TIMER_FLAG_IS_ENABLED (1 << 1)

#define TIMER_SET_FLAG( T, F ) do { (T)->flags |= TIMER_FLAG_ ## F; } while (0)
#define TIMER_CLEAR_FLAG( T, F ) do { (T)->flags &= ~TIMER_FLAG_ ## F; } while (0)
#define TIMER_HAS_FLAG( T, F ) ((T)->flags & TIMER_FLAG_ ## F)

#define TIMER_HAS_FLAGS2(T, FA, FB)				\
  ( ((T)->flags & (TIMER_FLAG_ ## FA | TIMER_FLAG_ ## FB)) ==	\
    (TIMER_FLAG_ ## FA | TIMER_FLAG_ ## FB) )

  unsigned char flags;

  void (*handler)(void);
};



/* pic18f4550 tmr0 */

static void rearm_tmr0(void)
{
  /* writing to the register must occur
     in this order since writing the low
     part make the real update. */

#define TMR0_FREQ ((OSC_FREQ) / (4 * 256))
#define TMR0_COUNTER ((0xffff) - TMR0_FREQ / SCHED_MAX_FREQ)

  TMR0H = (TMR0_COUNTER >> 8) & 0xff;
  TMR0L = (TMR0_COUNTER >> 0) & 0xff;
}


static void setup_tmr0(void)
{
  /* use 1:256 prescaler. assumed the insn clock being
     8mhz, then Fosc/4 is 2mhz, giving a timer freq of
     7,8125khz frequency. */

#if 0
  {
    /* prescaler */
    T0CONbits.PSA = 0;
    T0CONbits.T0PS0 = 1;
    T0CONbits.T0PS1 = 1;
    T0CONbits.T0PS2 = 1;

    /* inc on low to high */
    T0CONbits.T0SE = 0;

    /* use insn clock (Fosc/4) */
    T0CONbits.T0CS = 0;    

    /* 16 bits counter */
    T0CONbits.T08BIT = 0;
  }
#else
  {
    T0CON = 7;
  }
#endif

  /* enable interrupt */

  INTCONbits.TMR0IE = 1;
}


static unsigned char disable_tmr0_int(void)
{
  unsigned char prev_state;

  prev_state = INTCONbits.TMR0IE;
  INTCONbits.TMR0IE = 0;

  return prev_state;
}


static void restore_tmr0_int(unsigned char prev_state)
{
  INTCONbits.TMR0IE = prev_state;
}


/* wrapper */

static void set_timer_freq(sched_timer_t* timer, unsigned int freq)
{
#define FREQ_TO_COUNTDOWN(F) ((SCHED_MAX_FREQ) / (F))
  timer->current_countdown = FREQ_TO_COUNTDOWN(freq);
  timer->saved_countdown = timer->current_countdown;
}


/* exported */

#define SCHED_TIMER_COUNT 4

static struct sched_timer timers[SCHED_TIMER_COUNT];


void sched_setup(void)
{
  sched_timer_t* timer;
  unsigned int i;

  /* timer allocator */

  for (i = 0, timer = timers; i < SCHED_TIMER_COUNT; ++i, ++timer)
    timer->flags = 0;

  /* timer0 */

  setup_tmr0();
  rearm_tmr0();
}


void sched_enable(void)
{
  T0CONbits.TMR0ON = 1;
}


void sched_disable(void)
{
  T0CONbits.TMR0ON = 0;
}


sched_timer_t* sched_add_timer(unsigned int freq, void (*handler)(void), int is_enabled)
{
  unsigned char prev_state;
  sched_timer_t* timer;
  unsigned int i;

  prev_state = disable_tmr0_int();

  for (i = 0, timer = timers; i < SCHED_TIMER_COUNT; ++i, ++timer)
    if (!TIMER_HAS_FLAG(timer, IS_ALLOCATED))
      {
	TIMER_SET_FLAG(timer, IS_ALLOCATED);
	break;
      }

  restore_tmr0_int(prev_state);

  if (i == SCHED_TIMER_COUNT)
    {
#ifndef NULL
# define NULL (void*)0
#endif
      return NULL;
    }

  set_timer_freq(timer, freq);
  timer->handler = handler;

  if (is_enabled)
    TIMER_SET_FLAG(timer, IS_ENABLED);

  return timer;
}


void sched_del_timer(sched_timer_t* timer)
{
  /* todo: locking */

  TIMER_CLEAR_FLAG(timer, IS_ALLOCATED);
}


void sched_set_timer_freq(sched_timer_t* timer, unsigned int freq)
{
  set_timer_freq(timer, freq);
}


void sched_enable_timer(sched_timer_t* timer)
{
  TIMER_SET_FLAG(timer, IS_ENABLED);
}


void sched_disable_timer(sched_timer_t* timer)
{
  TIMER_CLEAR_FLAG(timer, IS_ENABLED);
}


void sched_reset_timer(sched_timer_t* timer)
{
  timer->current_countdown = timer->saved_countdown;
}


void sched_handle_interrupt(void)
{
  /* interrupts are disabled */

  sched_timer_t* timer;
  unsigned int i;

  if (!INTCONbits.TMR0IF)
    return ;

  INTCONbits.TMR0IF = 0;

  rearm_tmr0();

  for (timer = timers, i = 0; i < SCHED_TIMER_COUNT; ++i, ++timer)
    {
      if ( ! TIMER_HAS_FLAGS2(timer, IS_ALLOCATED, IS_ENABLED) )
	continue ;

      if (timer->current_countdown--)
	continue ;

      timer->handler();

      timer->current_countdown = timer->saved_countdown;
    }
}



#if 0 /* unit testing */


#include <pic18fregs.h>
#include "config.h"
#include "serial.h"
#include "int.h"



static volatile unsigned char led0_signal;

static void led0_handler(void)
{
  static unsigned char n = 0;

  led0_signal = 1;

  TRISAbits.TRISA0 = 0;
  LATAbits.LATA0 = n;

  n ^= 1;
}


static void led1_handler(void)
{
  static unsigned char n = 0;

  TRISAbits.TRISA1 = 0;
  LATAbits.LATA1 = n;

  n ^= 1;
}


static void led2_handler(void)
{
  static unsigned char n = 0;

  TRISAbits.TRISA2 = 0;
  LATAbits.LATA2 = n;

  n ^= 1;
}


void main(void)
{
  sched_timer_t* timers[3];

  osc_setup();
  int_setup();

  serial_setup();

  sched_setup();

  timers[0] = sched_add_timer( 1, led0_handler );
  timers[1] = sched_add_timer( 5, led1_handler );
  timers[2] = sched_add_timer( 10, led2_handler );

  sched_disable_timer(timers[1]);

  sched_enable();

  {
    unsigned int i = 0;

  redo:

    while (!led0_signal)
      ;

    led0_signal = 0;

    serial_writei(i++);

    goto redo;
  }
}


#endif /* unit testing */
