/*
** Made by fabien le mentec <texane@gmail.com>
** 
** Started on  Sun Jun  7 09:36:51 2009 texane
** Last update Sun Sep 27 19:36:51 2009 texane
*/



#ifndef SRF04_H_INCLUDED
# define SRF04_H_INCLUDED



enum srf04_state
  {
    SRF04_STATE_TRIGGER = 0,
    SRF04_STATE_WAIT_ECHO_HIGH,
    SRF04_STATE_WAIT_ECHO_LOW,
    SRF04_STATE_DONE
  };


extern volatile enum srf04_state srf04_state;



void srf04_setup(void);
unsigned int srf04_get_distance(void);
int srf04_handle_interrupt(void);



#endif /* ! SRF04_H_INCLUDED */
