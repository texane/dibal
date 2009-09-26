/*
** Made by texane <texane@gmail.com>
** 
** Started on  Sun Nov 16 15:10:47 2008 texane
** Last update Mon Sep 21 09:09:47 2009 texane
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/select.h>
#include "serial.h"



struct buf
{
  unsigned char* first;
  unsigned char* last; /* excl */

  unsigned char data[32];
};


static void buf_init(struct buf* buf)
{
  memset(buf, 0, sizeof(struct buf));

  buf->first = buf->data;
  buf->last = buf->data;
}


static size_t buf_size(const struct buf* buf)
{
  if (buf->first == buf->last)
    return 0;

  return buf->last - buf->first;
}


#if 0
static unsigned char* __attribute__((unused)) buf_data(struct buf* buf)
{
  return buf->first;
}
#endif


static size_t __attribute__((unused)) buf_offset(const struct buf* buf)
{
  return buf->first - buf->data;
}


static size_t buf_space(const struct buf* buf)
{
  return sizeof(buf->data) - (buf_offset(buf) + buf_size(buf));
}


static int buf_write(struct buf* buf, unsigned char* data, size_t size)
{
  if (size > buf_space(buf))
    return -1;

  memcpy(buf->last, data, size);
  buf->last += size;

  return 0;
}


static int buf_read(struct buf* buf, void* data, size_t size)
{
  if (size > buf_size(buf))
    return -1;

  memcpy(data, buf->first, size);
  buf->first += size;

  if (!buf_size(buf))
    buf_init(buf);

  return 0;
}


static int __attribute__((unused)) wait_for_read(int fd)
{
  fd_set rds;

  FD_ZERO(&rds);
  FD_SET(fd, &rds);

  if (select(fd + 1, &rds, NULL, NULL, NULL) != 1)
    return -1;

  return 0;
}



static int __attribute__((unused)) wait_for_write(int fd)
{
  fd_set wds;

  FD_ZERO(&wds);
  FD_SET(fd, &wds);

  if (select(fd + 1, NULL, &wds, NULL, NULL) != 1)
    return -1;

  return 0;
}



int main(int ac, char** av)
{
  int res = -1;
  unsigned short tmp;
  struct buf buf;
  serial_handle_t h;

  static const serial_conf_t c = { 9600, 8, 0, 1 };

  buf_init(&buf);

  if (serial_open(&h, av[1]) == -1)
    return -1;

  if (serial_set_conf(&h, &c) == -1)
    goto on_error;

#if _DEBUG
  serial_print(&h);
#endif

  while (1)
    {
      size_t size;

      if (wait_for_read(h.fd) == -1)
	goto on_error;

      if (serial_read(&h, (void*)&tmp, sizeof(tmp), &size) == -1)
	goto on_error;

      buf_write(&buf, (void*)&tmp, size);

      while (buf_read(&buf, &tmp, sizeof(tmp)) != -1)
	printf("0x%04x\n", tmp);
    }

  /* success
   */

  res = 0;

 on_error:

  serial_close(&h);

  return res;
}
