/*
** Made by fabien le mentec <texane@gmail.com>
** 
** Started on  Sat Oct  3 07:04:33 2009 texane
** Last update Sat Oct  3 09:10:45 2009 texane
*/



#include <pic18fregs.h>



/* oscillator */

void osc_setup(void)
{
  /* 8Mhz */

  OSCCONbits.IRCF = 7;

  /* internal osc used */

  OSCCONbits.SCS = 2;

  /* idle mode enable so that peripherals are
     clocked with SCS when cpu is sleeping. */

  OSCCONbits.IDLEN = 1;

  /* wait for stable freq */

  while (!OSCCONbits.IOFS)
    ;
}
