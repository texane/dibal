/*
** Made by fabien le mentec <texane@gmail.com>
** 
** Started on  Sat Sep 26 13:23:18 2009 texane
** Last update Sat Sep 26 14:12:05 2009 texane
*/



#include <pic18fregs.h>
#include "servo.h"



/* bits */

static unsigned char make_mask(servo_t* servo)
{
/*   return (1 << 7) >> servo->shift; */

  return 1 << servo->shift;
}


static void set_bit(servo_t* servo)
{
  *servo->addr |= make_mask(servo);
}


static void clear_bit(servo_t* servo)
{
  *servo->addr &= ~make_mask(servo);
}



/* exported */

void servo_setup_l(servo_t* servo)
{
  TRISBbits.TRISB0 = 0;

  servo->addr = &LATB;
  servo->shift = 0;
}


void servo_setup_r(servo_t* servo)
{
  TRISBbits.TRISB1 = 0;

  servo->addr = &LATB;
  servo->shift = 1;
}


void servo_rotate(servo_t* servo, unsigned char rounds)
{
  rounds;

  set_bit(&servo[0]);
  clear_bit(&servo[1]);

  {
    unsigned int i;
    for (i = 0; i < 300; ++i)
      ;
  }

  clear_bit(&servo[0]);
  set_bit(&servo[1]);
}
