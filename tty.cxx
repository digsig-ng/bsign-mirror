/* tty.cxx
     $Id: tty.cxx,v 1.2 1999/05/23 05:58:56 elf Exp $

   written by Oscar Levi
   13 December 1998

   Copyright (C) 1998 The Buici Company

   -----------
   DESCRIPTION
   -----------

   TTY IO control functions.

*/

#include "standard.h"
#include <termios.h>
#include "tty.h"
#include <errno.h>

extern const char* g_szApplication;

struct termios g_termiosSave;

void disable_echo (int fh)
{
  struct termios termios;

  if (tcgetattr (fh, &g_termiosSave))
    fprintf (stderr, "%s: tcgetattr failed on %d (%s)\n", 
	     g_szApplication, fh, strerror (errno));
  //        restore_termios = 1;
  termios = g_termiosSave;
  termios.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
  if (tcsetattr (fh, TCSAFLUSH, &termios))
    fprintf (stderr, "%s: tcsetattr failed on %d (%s)\n", 
	     g_szApplication, fh, strerror (errno));
}

void restore_echo (int fh)
{
  if (tcsetattr (fh, TCSAFLUSH, &g_termiosSave))
    fprintf (stderr, "%s: tcsetattr failed on %d (%s)\n", 
	     g_szApplication, fh, strerror (errno));
}

int open_user_tty (void)
{
  return open ("/dev/tty", O_RDWR);
}
