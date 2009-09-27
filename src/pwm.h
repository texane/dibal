/*
** Made by fabien le mentec <texane@gmail.com>
** 
** Started on  Sun Sep 27 19:55:59 2009 texane
** Last update Sun Sep 27 20:38:02 2009 texane
*/



#ifndef PWM_H_INCLUDED
# define PWM_H_INCLUDED



void do_pwm(unsigned int);
void do_epwm(unsigned int);
void stop_pwm(void);
void stop_epwm(void);



#endif /* ! PWM_H_INCLUDED */
