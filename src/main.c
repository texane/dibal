/*
** Made by fabien le mentec <texane@gmail.com>
** 
** Started on  Sun Sep 20 14:08:30 2009 texane
** Last update Sat Sep 26 13:53:56 2009 texane
*/



#include <pic18fregs.h>
#include "config.h"
#include "serial.h"

#include "FreeRTOS.h"
#include "task.h"
#include "servo.h"



#define SERVO_TASK_PRIORITY ( tskIDLE_PRIORITY + 1)



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


/* interrupt */

static void int_setup(void)
{
  /* disable high prio ints */

  RCONbits.IPEN = 0;
  INTCONbits.PEIE = 0;
  INTCONbits.GIE = 0;
}


/* tasks */

static void wait(unsigned int n)
{
#if 0
  while (n--)
    ;
#else
  n;
  vTaskDelay(1000 / portTICK_RATE_MS);
#endif
}


portTASK_FUNCTION_PROTO(servo, params)
{
  struct servo servos[2];

  params;

  servo_setup_l(&servos[0]);
  servo_setup_r(&servos[1]);

 redo:

  servo_rotate(&servos[0], 1);

  goto redo;
}


/* main */

void main(void)
{
  signed portBASE_TYPE error = 0;

  vPortInitialiseBlocks();

  osc_setup();
  int_setup();

  serial_setup();

  error = xTaskCreate(servo, (const portCHAR* const)"servo", configMINIMAL_STACK_SIZE, NULL, SERVO_TASK_PRIORITY, NULL);
  if (error != pdPASS)
    goto on_error;

  vTaskStartScheduler();

 on_error:
  while (1);
}
