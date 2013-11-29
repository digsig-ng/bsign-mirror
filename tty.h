/* tty.h
   $Id: tty.h,v 1.2 1999/05/23 05:58:56 elf Exp $
   
   written by Oscar Levi
   13 Dec 1998

*/

#if !defined (__TTY_H__)
#    define   __TTY_H__

/* ----- Includes */

#include <stdio.h>

/* ----- Globals */


void disable_echo (int fh);
void restore_echo (int fh);
int open_user_tty (void);

#endif  /* __TTY_H__ */
