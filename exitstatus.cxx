/* exitstatus.cxx
     $Id: exitstatus.cxx,v 1.2 2002/01/16 02:32:34 elf Exp $

   written by Oscar Levi
   23 May 1999

   Copyright (C) 1999 The Buici Company

   -----------
   DESCRIPTION
   -----------

*/

#include "standard.h"
#include "exitstatus.h"

#include <stdarg.h>

char g_szExitStatus[256];
eExitStatus g_exitStatus;	// Return code for the program

void set_exitstatus (eExitStatus exitStatus, const char* sz, ...)
{
  g_exitStatus = exitStatus;

  if (sz) {
    va_list ap;
    va_start (ap, sz);
    vsnprintf (g_szExitStatus, sizeof (g_szExitStatus), sz, ap);
    va_end (ap);
  }
  else
    g_szExitStatus[0] = 0;
}

void set_exitstatus_reported (void)
{
  g_szExitStatus[0] = 0;
}
