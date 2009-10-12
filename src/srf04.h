/*
** Made by fabien le mentec <texane@gmail.com>
** 
** Started on  Sun Jun  7 09:36:51 2009 texane
** Last update Mon Oct 12 10:49:37 2009 texane
*/



#ifndef SRF04_H_INCLUDED
# define SRF04_H_INCLUDED



#define SRF04_INVALID_DISTANCE ((unsigned int)-1)


void srf04_setup(void);
unsigned int srf04_get_distance(unsigned int);



#endif /* ! SRF04_H_INCLUDED */
