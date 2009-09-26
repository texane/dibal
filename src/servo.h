/*
** Made by fabien le mentec <texane@gmail.com>
** 
** Started on  Sat Sep 26 13:21:43 2009 texane
** Last update Sat Sep 26 13:54:30 2009 texane
*/



#ifndef SERVO_H_INCLUDED
# define SERVO_H_INCLUDED



struct servo
{
  volatile unsigned char* addr;
  unsigned char shift;
};

typedef struct servo servo_t;



void servo_setup_l(servo_t*);
void servo_setup_r(servo_t*);
void servo_rotate(servo_t*, unsigned char);



#endif /* ! SERVO_H_INCLUDED */
