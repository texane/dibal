/*
** Made by fabien le mentec <texane@gmail.com>
** 
** Started on  Sun Nov 30 04:33:07 2008 texane
** Last update Sun Sep 27 20:37:47 2009 texane
*/



#include <pic18fregs.h>



/* pwm

   . Txxx are expressed in seconds while Fxxx are expressed
   in hertz. for instance, if Fpwm is 500 then Tpwm 0.002
   . to convert from time to period: p = 1 / t

   . period register (PR2)
   Tpwm = (PR2 + 1) * Tosc * 4 * TMR2_prescaler
   thus
   PR2 = (Tpwm / (Tosc * 4 * TMR2_prescaler)) - 1

   example: if the period is to be 2ms and cpu freq is
   8mhz, then pr2 = (0.002) / ((1 / 8000000) * 4 * 16) - 1
   TMR2_prescaler must be set to 16 to avoid PR2 overflow

   . duty register (CCPR2L : CCP2CON<5:4>)
   Tduty = DUTY_register * (Tosc * TMR2_prescaler)
   thus
   DUTY_register = Tduty / (Tosc * TMR2_prescaler)
 */


static unsigned int get_duty_cycle(int dir)
{
  /* dir the direction
   */

#define CPU_Fosc 8000000.f
#define CPU_Tosc (1 / (CPU_Fosc))

#define TMR2_prescaler 16

#define COMPUTE_PWM_DUTY(Tduty) ((Tduty) / ((CPU_Tosc) * (TMR2_prescaler)))

  static int duty_table[] =
    {
      /* backward */
      COMPUTE_PWM_DUTY(0.0011),

      /* median */
      COMPUTE_PWM_DUTY(0.0015),

      /* forward */
      COMPUTE_PWM_DUTY(0.0019)
    };

  return duty_table[dir + 1];
}


/* exported */

void do_pwm(unsigned int value)
{
  /* value, the analog value */

  unsigned int dcycle;

  dcycle = get_duty_cycle(value);

  /* operations are done in this order
     as mentionned in the datahseet */

  /* period register */
#define COMPUTE_PWM_PERIOD(Tpwm) (((Tpwm) / ((CPU_Tosc) * 4 * (TMR2_prescaler))) - 1)

  PR2 = COMPUTE_PWM_PERIOD(0.002);

  /* duty cycle msbs */
  CCPR2L = (unsigned char)(dcycle >> 2);

  /* pin selected by conf bits */
#if 1 /* RC1 */
  {
    TRISCbits.TRISC1 = 0;
  }
#else /* RB3 */
  {
    TRISBbits.TRISB3 = 0;
  }
#endif

  /* enable timer2, prescaler == 16 */
#if 0
  {
    T2CONbits.TMR2ON = 1;
    T2CONbits.T2CKPS0 = 0;
    T2CONbits.T2CKPS1 = 1;
  }
#else
  {
    T2CON = (1 << 2) | 0x2;
  }
#endif

  /* configure control register */
#if 0 
  {
    /* pwm mode */
    CCP2CONbits.CCP2M0 = 0;
    CCP2CONbits.CCP2M1 = 0;
    CCP2CONbits.CCP2M2 = 1;
    CCP2CONbits.CCP2M3 = 1;

    /* duty cycle lsbs */
    CCP2CONbits.DC2B0 = ((unsigned char)dcycle >> 0) & 1;
    CCP2CONbits.DC2B1 = ((unsigned char)dcycle >> 1) & 1;
  }
#else
  {
    CCP2CON = (((unsigned char)(dcycle & 0x3)) << 4) | 0x0c;
  }
#endif
 
}


void do_epwm(unsigned int value)
{
  /* used in standard mode */

  unsigned int dcycle;

  dcycle = get_duty_cycle(value);

/*   PR2 = COMPUTE_PWM_PERIOD(0.002); */

  CCPR1L = (unsigned char)(dcycle >> 2);

  TRISCbits.TRISC2 = 0;

  ECCP1DEL = 0;

/*   T2CON = (1 << 2) | 0x2; */

  CCP1CON = (((unsigned char)(dcycle & 0x3)) << 4) | 0x0c;
}


void stop_pwm(void)
{
  CCP2CON = 0;
}


void stop_epwm(void)
{
  CCP1CON = 0;
}
