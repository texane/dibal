/*
** Made by fabien le mentec <texane@gmail.com>
** 
** Started on  Sat Oct  3 07:05:54 2009 texane
** Last update Sat Oct  3 10:47:12 2009 texane
*/



#include <pic18fregs.h>

#ifdef CONFIG_ENABLE_SCHED
# include "sched.h"
#endif

#ifdef CONFIG_ENABLE_SERIAL
# include "serial.h"
#endif

#ifdef CONFIG_ENABLE_TIMER
# include "timer.h"
#endif

#ifdef CONFIG_ENABLE_SRF04
# include "srf04.h"
#endif



void on_low_interrupt(void) __interrupt 2;


void on_low_interrupt(void) __interrupt 2
{
#ifdef CONFIG_ENABLE_SCHED
  sched_handle_interrupt();
#endif

#ifdef CONFIG_ENABLE_SRF04
  srf04_handle_interrupt();
#endif

#ifdef CONFIG_ENABLE_TIMER
  timer_handle_interrupt();
#endif

#ifdef CONFIG_ENABLE_SERIAL
  serial_handle_interrupt();
#endif
}


void int_setup(void)
{
  /* disable high prio ints */

  RCONbits.IPEN = 0;
  INTCONbits.PEIE = 1;
  INTCONbits.GIE = 1;
  INTCON2bits.RBIP = 0;
}


void int_disable(unsigned char* prev_state)
{
  *prev_state = INTCONbits.GIE;
}


void int_restore(unsigned char prev_state)
{
  INTCONbits.GIE = prev_state;
}
