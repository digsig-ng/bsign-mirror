/* filewalk.h		-*- C++ -*-
     $Id: filewalk.h,v 1.6 2002/01/17 23:56:05 elf Exp $

   written by Marc Singer
   14 Jan 2002

   This file is part of the project BSIGN.  See the file README for
   more information.

   Copyright (C) 2002 The Buici Company

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   in a file called COPYING along with this program; if not, write to
   the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA
   02139, USA.

   -----------
   DESCRIPTION
   -----------

   A wrapper around FTS with support for inclusion and exclusion.  It
   is important to note that the include, exclude and insert functions
   accept and store character pointers without copying the data.  This
   is because the intended use is with parsing command line arguments.
   Callers that cannot cope with this must copy strings before passing
   them to FileWalk.


*/

#if !defined (__FILEWALK_H__)
#    define   __FILEWALK_H__

/* ----- Includes */

#include <fts.h>		/* glibc2 header, required */
#include <limits.h>
#include <sys/stat.h>


/* ----- Types */

class FileWalk {
public:

  typedef struct {
    int cMax;
    int cUsed;
    char** rgsz;
  } List;

protected:

  const char* m_szFileCurrent;
  struct stat* m_pStat;
  
  struct stat m_stat;		// Stat buffer when we need one

		// --- FTS enumeration
  FTS* m_pfts;			// Pointer to fts object
  FTSENT* m_pent;		// Pointer to last returned file object
  List m_listInclude;		// Funky list used to include roots
  List m_listExclude;		// Funky list used to exclude

  		// --- Explicit enumeration
  List m_listInsert;		// Funky list used to store explicit pathnames
  int m_cCurrent;		// Index of this current item in inserted list 

		// --- File source enumeration
  const char* m_szPathSource;	// Read filenames from a file
  FILE* m_fp;			// Buffered I/O handle for reading filenames
  char m_szFilename[PATH_MAX + 1]; // Last read filename

  bool is_excluded (void);
  void release_this (void);
  void zero (void) {
    bzero (this, sizeof (*this)); m_cCurrent = -1; }

  const char* next_insert (void);
  const char* next_fts (void);
  const char* next_source (void);

public:
  FileWalk () { zero (); }
  ~FileWalk () { release_this (); }

  void exclude (const char* sz);
  void include (const char* sz);
  void insert  (const char* sz);
  void source  (const char* szPath) { m_szPathSource = szPath; }

  bool is_ready (void);
  bool is_only_one (void);
  const char* next (void);
  const char* current (void) {
    return m_szFileCurrent; }
  struct stat* stat (void) {
    return m_pStat; }

};

/* ----- Globals */

/* ----- Prototypes */



#endif  /* __FILEWALK_H__ */
