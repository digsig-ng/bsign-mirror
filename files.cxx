/* files.cxx
     $Id: files.cxx,v 1.11 2002/01/17 06:24:29 elf Exp $

   written by Oscar Levi
   10 December 1998

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

   Simple routines for manipulating files in the filesystem.

*/

#include "standard.h"
#include "sys/stat.h"
#include "sys/types.h"
#include "limits.h"
#include "utime.h"
#include "errno.h"
#include "files.h"


/* dup_status

   copies the status of one file to another.  This is meant to
   replicate the ownership and permissions of the file.

   When chmod is before chown and we are attempting to set the setuid
   bit, the chown call clears that bit.  Transposing these calls makes
   it work properly.

*/

bool dup_status (const char* szFileNew, const char* szFile)
{
  struct stat st;
  if (::stat (szFile, &st))
    return false;
  struct utimbuf t;
  t.actime = st.st_atime;
  t.modtime = st.st_mtime;
  if (utime (szFileNew, &t))
    return false;
  if (chown (szFileNew, st.st_uid, st.st_gid))
    return false;
  if (chmod (szFileNew, st.st_mode))
    return false;
  return true;
}

#if 0
/* is_file

   returns true if the named file is a regular file or a link to a
   regular file. 

*/

bool is_file (const char* szPath)
{
  struct stat st;
  memset (&st, 0, sizeof (struct stat));
  if (stat (szPath, &st))
    return false;
  return S_ISREG (st.st_mode);
}


/* is_symlink

   returns true of the last component of szPath is a symbolic link.
   Directory components that are symbolic links are not detected. 

*/

bool is_symlink (const char* szPath)
{
  struct stat st;
  memset (&st, 0, sizeof (struct stat));
  if (lstat (szPath, &st))
    return false;
  return S_ISLNK (st.st_mode);
}
#endif

char* path_of (const char* szPathOriginal)
{
  char* sz = resolve_link (szPathOriginal);
  if (!sz) {			// No link to resolve
    sz = new char [strlen (szPathOriginal) + 3];
    strcpy (sz, szPathOriginal);
  }

  char* pch = rindex (sz, '/');
  if (!pch) {
    strcpy (sz, ".");
    return sz;
  }
  *pch = 0;

  return sz;
}


char* resolve_link (const char* szPathOriginal)
{
  if (!szPathOriginal || !*szPathOriginal 
      || strlen (szPathOriginal) >= PATH_MAX)
    return NULL;

  static char szPath[PATH_MAX];
  strcpy (szPath, szPathOriginal);
  
  struct stat st;
  
				// Find where the link really points
  do {
    memset (&st, 0, sizeof (struct stat));
    if (lstat (szPath, &st))
      return NULL;
    if (S_ISLNK (st.st_mode)) {
      static char sz[PATH_MAX + 1];
      int cb = readlink (szPath, sz, sizeof (sz) - 1);
      if (cb == -1)
	return NULL;
      sz[cb] = 0;
      if (*sz == '/')		// Absolute path link
	strcpy (szPath, sz);
      else {			// Relative path link
	if (strlen (szPath) + strlen (sz) >= PATH_MAX)
	  return NULL;
	char* pch = rindex (szPath, '/');
	strcpy (pch ? (pch + 1) : szPath, sz);
      }
    }
  } while (S_ISLNK (st.st_mode));

  char* sz = new char[strlen (szPath) + 1];
  strcpy (sz, szPath);
  return sz;
}


/* replace_file

   function for replacing the old file with the new one.  Because we
   create our temporary output file in the same directory as the
   output target, it is extremely improbably that we will not be able
   to replace the old file with the new.  This function guarantees
   that at no time will the old file be deleted before the new file
   replaces it.

*/

bool replace_file (const char* szFileOutputOriginal, const char* szFile)
{
  char* szFileOutput = resolve_link (szFileOutputOriginal);
  if (!szFileOutput)		// Link resolution error
    return false;

  int result = rename (szFile, szFileOutput); // One step replacement 

  free (szFileOutput);
  return result == 0;
}

