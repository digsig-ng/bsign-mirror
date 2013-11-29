/* exec.cxx
     $Id: exec.cxx,v 1.7 2002/01/15 06:38:20 elf Exp $

   written by Oscar Levi
   8 December 1998

   This file is part of the project BSIGN.  See the file README for
   more information.

   Copyright (C) 1998 The Buici Company.

   This program is free software; you can redistribute it and/or modify 
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA

   -----------
   DESCRIPTION
   -----------

   Support for executing a child process and managing IO streams for
   that child.

*/

#include "standard.h"
#include "exec.h"

#include "sys/types.h"
#include "sys/wait.h"
#include "sys/time.h"
#include "errno.h"

int ExStream::exec (const char* szCommand, ExStream** rgpexs, int cStream)
{
  //  extern bool g_fDebug;
  //  if (g_fDebug)
  //    fprintf (stderr, "execute: '%s'\n", szCommand);

#if 0
  for (int i = 0; i < cStream; ++i) {
    ExStream& exs = *rgpexs[i];
    fprintf (stderr, "%sing %d %d\n", exs.m_fWrite ? "writ" : "read", 
	     exs.m_rgfd[0], exs.m_rgfd[1]);
  }
#endif

  int pid = fork ();
  int status = 0;		// Child's exit status

  if (pid == 0) {		// Child
    for (int i = 0; i < cStream; ++i) {	// Perform redirection
      ExStream& exs = *rgpexs[i];
      if (exs.m_fdTarget != -1) {
	//	fprintf (stderr, "%d: child closing %d\n", i, exs.m_fdTarget);
	close (exs.m_fdTarget);
	//	fprintf (stderr, "%d: child duping %d to %d\n", i, 
	//		 exs.fd_child (), exs.m_fdTarget);
	int result = dup2 (exs.fd_child (), exs.m_fdTarget);
	//	if (result == -1)
	//	  fprintf (stderr, "dup2 failed %d %d\n", result, errno);
	//	fprintf (stderr, "%d: child closing %d\n", i, exs.fd_child ());
	close (exs.fd_child ());
      }
    //      fprintf (stderr, "%d: child closing parent's %d\n", i, exs.fd_parent ());
      close (exs.fd_parent ());
    }

    //    sleep (1);
    //    fprintf (stderr, "child command '%s'\n", szCommand);
    int result = execl ("/bin/sh", "sh", "-c", szCommand, NULL);
    fprintf (stderr, "child terminated unexpectedly  result %d  errno %d\n", 
	     result, errno);
    _exit (127);		// *** FIXME, why this constant?
  }
  else {			// Parent
    //    sleep (1);
    for (int i = 0; i < cStream; ++i) {	// Make everything non-block
      ExStream& exs = *rgpexs[i];
      //      fprintf (stderr, "parent O_NONBLOCKING %d\n", exs.fd_parent ());
      fcntl (exs.fd_parent (), F_SETFL, O_NONBLOCK);
      //      fprintf (stderr, "parent closing %d\n", exs.fd_child ());
      close (exs.fd_child ());
    }

    bool fQuitChild = false;
    bool fQuitIO = false;
    while (!fQuitChild || !fQuitIO) {
      fd_set fdsetRead;
      FD_ZERO (&fdsetRead);
      fd_set fdsetWrite;
      FD_ZERO (&fdsetWrite);
      int n = 0;		// Highest numbered descriptor for select

      for (int i = 0; i < cStream; ++i) { // Make everything non-block
	ExStream& exs = *rgpexs[i];
	if (!exs.is_active ())
	  continue;
	FD_SET (exs.fd_parent (), exs.m_fWrite ? &fdsetWrite : &fdsetRead);
	if (exs.fd_parent () > n)
	  n = exs.fd_parent ();
      }

				// Work the fd's until we kenna do it no more
      struct timeval tv;
      tv.tv_sec = 0;
      tv.tv_usec = 100000;	// 1/10th of a second
    
      int result =  select (n + 1, &fdsetRead, &fdsetWrite, NULL, &tv);
      if (result == 0 && fQuitChild)
	fQuitIO = true;

      for (int i = 0; i < cStream; ++i) { // Make everything non-block
	ExStream& exs = *rgpexs[i];
	if (FD_ISSET (exs.fd_parent (), 
		      exs.m_fWrite ? &fdsetWrite : &fdsetRead)) {
	  if (exs.m_fWrite) {
	    int cb = write (exs.fd_parent (), exs.m_pb + exs.m_cbUsed,
			    exs.m_cb - exs.m_cbUsed);
	    //	    fprintf (stderr, "parent writing to %d (%d bytes)\n",
	    //		     exs.fd_parent (), cb);
	    exs.m_cbUsed += cb;
	    if (exs.m_cb == exs.m_cbUsed) {
	      //	      fprintf (stderr, "parent closing to %d (EOF)\n", 
	      //		       exs.fd_parent ());
	      close (exs.fd_parent ());
	    }
	    
	  }
	  else {
	    int cb = read (exs.fd_parent (), exs.m_pb + exs.m_cbUsed,
			   exs.m_cb - exs.m_cbUsed);
//	    fprintf (stderr, "parent reading from %d (%d bytes, %d total)\n",
//		     exs.fd_parent (), cb, exs.m_cbUsed + cb);
	    exs.m_cbUsed += cb;
	    if (cb == 0)
	      exs.m_fClosed = true;
	  }
	}
      }

      if (waitpid (pid, &status, WNOHANG) == pid) {
	fQuitChild = true;
	//	fprintf (stderr, "child exited with status 0x%x (%d)\n", 
	//		 status, status);
	status = WEXITSTATUS (status);
      }
    } // while 
  }
  return status;
}
