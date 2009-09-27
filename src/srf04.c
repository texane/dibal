/*
** Made by fabien le mentec <texane@gmail.com>
** 
** Started on  Sun Jun  7 09:36:34 2009 texane
** Last update Sun Sep 27 19:36:43 2009 texane
*/



#include <pic18fregs.h>
#include "./serial.h"
#include "./timer.h"



enum srf04_state
  {
    SRF04_STATE_TRIGGER = 0,
    SRF04_STATE_WAIT_ECHO_HIGH,
    SRF04_STATE_WAIT_ECHO_LOW,
    SRF04_STATE_DONE
  };


volatile enum srf04_state srf04_state;


/* #define SRF04_TRIGGER_PIN LATBbits.LATB0 */
#define SRF04_TRIGGER_PIN PORTBbits.RB0
#define SRF04_ECHO_PIN PORTBbits.RB1



void srf04_setup(void)
{
  TRISBbits.TRISB0 = 0;
  TRISBbits.TRISB1 = 1;

  ADCON1bits.PCFG0 = 1;
  ADCON1bits.PCFG1 = 1;
  ADCON1bits.PCFG2 = 1;
  ADCON1bits.PCFG3 = 1;
}



/* static void write_int(int n) */
/* { */
/*   serial_write((const void*)&n, sizeof(n)); */
/* } */



unsigned int srf04_get_distance(void)
{
  unsigned int n;

  srf04_state = SRF04_STATE_TRIGGER;

  SRF04_ECHO_PIN = 0;

  /* enable interrupt */

  INTCONbits.RBIE = 1;

  /* trigger pulse 10us (8mhz) */

  SRF04_TRIGGER_PIN = 0;
  SRF04_TRIGGER_PIN = 1;

  timer_loop(10);

  SRF04_TRIGGER_PIN = 0;

  while (!SRF04_ECHO_PIN)
    ;

  timer_start();

  while (SRF04_ECHO_PIN)
    ;

  n = timer_stop();

  /* at 8mhz, 2000000 cycles per sec */

/*   while (srf04_state <= SRF04_STATE_WAIT_ECHO_LOW) */
/*     ; */

/*   for (n = 0; srf04_state != SRF04_STATE_DONE; ++n) */
/*     ; */

  INTCONbits.RBIE = 0;

  return n;
}


int srf04_handle_interrupt(void)
{
  unsigned char value;

  if (!INTCONbits.RBIF)
    return -1;

  switch (srf04_state)
    {
    case SRF04_STATE_TRIGGER:
      srf04_state = SRF04_STATE_WAIT_ECHO_HIGH;
      break;

    case SRF04_STATE_WAIT_ECHO_HIGH:
      srf04_state = SRF04_STATE_WAIT_ECHO_LOW;
      break;

    case SRF04_STATE_WAIT_ECHO_LOW:
      srf04_state = SRF04_STATE_DONE;
      break;

    default:
      srf04_state = SRF04_STATE_DONE;
      break;
    }

  value = PORTB;

  INTCONbits.RBIF = 0;

  return 0;
}
