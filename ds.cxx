/* ds.cxx
     $Id: ds.cxx,v 1.16 2002/01/29 23:29:03 elf Exp $

   written by Oscar Levi
   6 December 1998

   This file is part of the project BSIGN.  See the file README for
   more information.

   Copyright (c) 1998 The Buici Company.

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

   Interface for generating digital signature certificates.  

   The first round uses gpg for generating signatures.

*/

#define USE_BATCHMODE		// Silent operation of gpg

#include "standard.h"
#include "exec.h"
#include "ds.h"
#include "exitstatus.h"
#include "tty.h"

extern int g_fDebug;
extern const char* g_szApplication;
extern const char* g_szPGOptions;
extern int g_fVerbose;

const char* fetch_passphrase (void);

#define TEMPLATE_VERIFY "/tmp/bsignv-XXXXXX"
char g_szPathTempVerify[] = TEMPLATE_VERIFY;

#define PG_PROGRAM "gpg"

/* cleanup_temp_verify

   function to remove the temporary used during verification.  Until
   we can verify without writing the data to disk, this is the best we
   can do.  This function is so declared because it is called from the
   signal handlers.

*/

void cleanup_temp_verify (bool fSignal)
{
  if (g_szPathTempVerify[0]) {
    int result = unlink (g_szPathTempVerify);
    //    if (result == 0 && fSignal)
    //      fprintf (stderr, "unlinked temp %s\n", g_szPathTempVerify);
  }
  if (!fSignal)
    g_szPathTempVerify[0] = 0;
}


/* create_digital_signature

   takes the cb byte data block at pb, writes it to a temporary file
   invokes the appropriate software to sign it, and returns the data
   in a memory block.  The first two bytes of the data block are the
   MSB ordered length of the signature.

   The return value is NULL if the signature could not be created.

*/

char* create_digital_signature (const char* pb, size_t cb)
{
  //  if (g_fDebug)
  //    fprintf (stderr, "options: '%s'\n", g_szPGOptions);

				// Create control streams
  char rgbSignature[512];
  char rgbErr[1024];		// *** FIXME: we need a ring buffer maybe?
  const char* szPass = fetch_passphrase ();
  if (!szPass)
    return NULL;
  ExStream exsIn, exsOut, exsPassPhrase, exsErr;
  if (   !exsIn.for_stdin (pb, cb)
      || !exsOut.for_stdout (rgbSignature, sizeof (rgbSignature))
      || !exsPassPhrase.for_writing (szPass, strlen (szPass))
      || !exsErr.for_stderr (rgbErr, sizeof (rgbErr))
      )
    return NULL;

  char sz[256];
  sprintf (sz, PG_PROGRAM " --no-greeting "
#if defined (USE_BATCHMODE)
	   "--batch "
#endif
	   "-sb -o - --passphrase-fd %d %s", 
	   exsPassPhrase.fd_write (), g_szPGOptions ? g_szPGOptions : "");

  ExStream* rgexs[] = { &exsIn, &exsOut, &exsPassPhrase, &exsErr };
  int result = ExStream::exec (sz, rgexs, sizeof (rgexs)/sizeof (ExStream*));

  if (result) {
    if (g_fVerbose)
      fprintf (stderr, "%s\n", rgbErr);
    return NULL;
  }

  cb = exsOut.used ();
  char* pbReturn = (char*) malloc (cb + 2);
  pbReturn[0] = ((cb >> 8) & 0xff);
  pbReturn[1] = (cb & 0xff);
  memcpy (pbReturn + 2, rgbSignature, cb);

  return pbReturn;
}

eExitStatus verify_digital_signature (const char* pbData, size_t cbData,
				      const char* pbCert, size_t cbCert)
{
  if (cbCert == 0)		// Kind of a sanity check.  We also
    return badsignature;	// verify this at the next layer up.
				// This is here in case we call from
				// some other place.

				// Create control streams
  char rgbErr[1024];
  ExStream exsIn, exsErr;
  if (   !exsIn.for_stdin (pbCert, cbCert)
      || !exsErr.for_stderr (rgbErr, sizeof (rgbErr)))
    return toomanyopenfiles;

#if 1
  strcpy (g_szPathTempVerify, TEMPLATE_VERIFY);
  int fh = mkstemp (g_szPathTempVerify);
				// *** FIXME, more error handling?
  if (g_fDebug)
    fprintf (stderr, "%s: creating temporary '%s' for data on verify\n", 
	     g_szApplication, g_szPathTempVerify);

  write (fh, pbData, cbData);
  close (fh);
#endif

  char sz[256];
  sprintf (sz, PG_PROGRAM " --no-greeting "
#if defined (USE_BATCHMODE)
	   "--batch "
#endif
	   "%s --verify - %s", 
	   g_szPGOptions ? g_szPGOptions : "", g_szPathTempVerify);
  ExStream* rgexs[] = { &exsIn, &exsErr };
  int result = ExStream::exec (sz, rgexs, sizeof (rgexs)/sizeof (ExStream*));

  cleanup_temp_verify (false);

  if (result && g_fVerbose)
    fprintf (stderr, "error result %d\n%s\n", result, rgbErr);
  if (!result)
    return noerror;
  switch (result) {
  default:
    return badsignature;
  case 127:
    return programnotfound;
  }
}


/* fetch_passphrase

   ask the user for the passphrase.  We cache this phrase and return
   it for every subsequenct call.  This is fine, for now.  It is
   possible that we'll do something more sophisticated later, but it
   is probably not necessary.

*/

const char* fetch_passphrase (void)
{
  static char* g_szPassPhrase;

  if (g_szPassPhrase)
    return g_szPassPhrase;

  g_szPassPhrase = (char*) malloc (512);

				// *** FIXME make this raw or
				// something so we don't ECHO.  We do
				// want to continue to input until we
				// receive a newline, though.

  int fh = open_user_tty ();
  if (fh == -1)
    return NULL;

  disable_echo (fh);
  char szMessage[] = "\nEnter pass phrase: ";
  write (fh, szMessage, strlen (szMessage));
  //  fflush (stdout);
  size_t cb = read (fh, g_szPassPhrase, 511);
  restore_echo (fh);
  write (fh, "\n", 1);
  close (fh);
  g_szPassPhrase[cb] = 0;

  return g_szPassPhrase;
}
