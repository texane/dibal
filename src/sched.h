/*
** Made by fabien le mentec <texane@gmail.com>
** 
** Started on  Fri Oct  2 21:35:17 2009 texane
** Last update Sat Oct  3 09:00:34 2009 texane
*/



#ifndef SCHED_H_INCLUDED
# define SCHED_H_INCLUDED



typedef struct sched_timer sched_timer_t;



void sched_setup(void);

void sched_handle_interrupt(void);

void sched_enable(void);
void sched_disable(void);

sched_timer_t* sched_add_timer(unsigned int, void (*handler)(void));
void sched_enable_timer(sched_timer_t*);
void sched_disable_timer(sched_timer_t*);



#endif /* ! SCHED_H_INCLUDED */
