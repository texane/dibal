/*
** Made by fabien le mentec <texane@gmail.com>
** 
** Started on  Mon Sep 21 08:51:53 2009 texane
** Last update Mon Sep 21 15:15:35 2009 texane
*/



#ifndef SERIAL_H_INCLUDED
# define SERIAL_H_INCLUDED



void serial_setup(void);
void serial_handle_int(void);
void serial_read(unsigned char*, unsigned char);
void serial_write(unsigned char*, unsigned char);
void serial_writei(unsigned int);
void serial_writeb(unsigned char);



#endif /* ! SERIAL_H_INCLUDED */
