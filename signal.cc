/* signal.cc
     $Id: signal.cc,v 1.1 2002/01/17 06:24:29 elf Exp $

   written by Marc Singer
   16 Jan 2002

   Copyright (C) 2002 The Buici Company

   -----------
   DESCRIPTION
   -----------

   Signal handlers to make us resilient.

*/


#include "standard.h"
#include <signal.h>
#include "exitstatus.h"

void cleanup_temp_verify (bool fSignal);
void cleanup_temp_hash (bool fSignal);

void signal_handler_cleanup (int);

void setup_signals (void)
{
  struct sigaction sa;

				// Setup SIGHUP handler
  bzero (&sa, sizeof (sa));
  sa.sa_handler = signal_handler_cleanup;
#if defined (SIGHUP)
  sigaction (SIGHUP, &sa, NULL);
#endif
#if defined (SIGINT)
  sigaction (SIGINT, &sa, NULL);
#endif
#if defined (SIGQUIT)
  sigaction (SIGQUIT, &sa, NULL);
#endif
#if defined (SIGABRT)
  sigaction (SIGABRT, &sa, NULL);
#endif
#if defined (SIGPIPE)
  sigaction (SIGPIPE, &sa, NULL);
#endif
#if defined (SIGTERM)
  sigaction (SIGTERM, &sa, NULL);
#endif
}

void signal_handler_cleanup (int signal)
{
  cleanup_temp_verify (true);
  cleanup_temp_hash (true);

  //  fprintf (stderr, "exit on signal %d\n", signal);

  exit (quit);
}
