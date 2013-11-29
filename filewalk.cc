/* filewalk.cc
     $Id: filewalk.cc,v 1.6 2002/01/17 01:22:28 elf Exp $

   written by Marc Singer
   14 Jan 2002

   This file is part of the project BSIGN.  See the file README for
   more information.

   Copyright (c) 2002 The Buici Company.

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

   Wrapper for FTS. 


   Memory Allocation
   -----------------

   With respect to memory, there are memory leaks in the code.  Before
   getting excited, think about how this code is used.  While we free
   the memory allocated for FileWalk::rgsz, there is little reason to
   do so.  The memory leak is bounded by the fact that the program
   processes a single list of files and terminates.  On termination,
   all memory is returned to the operating system.  But there is a
   more compelling reason for the leak.  While we could reallocate
   memory for every string that is passed to FileWalk, nearly every
   string that is passed in will come from the command line and,
   therefore, already be available for the duration of the program.
   In the one case where we allocate memory, there is no efficient way
   to mark which items were allocated and which were not, so we cannot
   easily know which to free.  In the end, the memory allocation
   policy is consistent with proper execution of the program.

*/

#include "standard.h"
#include "filewalk.h"


/* append_to_list

   adds the given string to the array (list) of strings.  The pointer
   is stored as it without being copied.  Be warned.  Callers passing
   pointers that are volatile will cause unpredictable behavior.

*/

void append_to_list (FileWalk::List& list, const char* sz)
{
				// Make room, leave a NULL at end
  if (list.cUsed + 1 >= list.cMax) {
    int cNew = list.cMax + 20;
    char** rgsz = new char*[cNew];
    bzero (rgsz, sizeof (char*)*cNew);
    if (list.cUsed) {
      bcopy (list.rgsz, rgsz, sizeof (char*)*list.cUsed);
      delete list.rgsz;
    }
    list.rgsz = rgsz;
    list.cMax = cNew;
  }
				// Store new string
  int cch = strlen (sz);
  char* szNew = new char[cch + 1];
  strcpy (szNew, sz);
  list.rgsz[list.cUsed++] = szNew;
}

void release_list (FileWalk::List& list)
{
  if (list.cMax) {
    delete list.rgsz;
    list.rgsz = 0;
    list.cMax = 0;
  }
}


void FileWalk::exclude (const char* sz)
{
  // Cope with terminating, superfluous trailing /
  int cch = strlen (sz);
  if (cch > 1 && sz[cch - 1] == '/') {
    char* szNew = new char [cch + 1];
    strcpy (szNew, sz);
    szNew[cch - 1] = 0;
    sz = (const char*) szNew;
  }
  append_to_list (m_listExclude, sz);
}

void FileWalk::include (const char* sz)
{
  append_to_list (m_listInclude, sz);
}

void FileWalk::insert (const char* sz)
{
  append_to_list (m_listInsert, sz);
}


/* FileWalk::is_only_one

   returns true if there is one and only one file to enumerate.  Since
   some of the enumeration methods cannot guarantee that condition a
   priori, it returns true only when this condition is guaranteed.

*/

bool FileWalk::is_only_one (void)
{
  return is_ready () && m_listInsert.cUsed == 1;
}


/* FileWalk::is_ready

   returns true if this object is ready for enumeration.  We permit
   one and only one method of specifying the files to enumerate.

*/

bool FileWalk::is_ready (void)
{
  int c = 0;
  if (m_listInsert.cUsed)
    ++c;
  if (m_listInclude.cUsed)
    ++c;
  if (m_szPathSource)
    ++c;
  return (c == 1);
}


bool FileWalk::is_excluded (void)
{
  for (int i = 0; i < m_listExclude.cUsed; ++i)
    if (strcmp (m_pent->fts_path, m_listExclude.rgsz[i]) == 0)
      return true;
  return false;
}



/* FileWalk::next and variants


   these functions perform the enumeration.  There are three variants
   depending on the way that this object was initialized. 

*/


const char* FileWalk::next (void)
{
  const char* sz = NULL;
  m_pStat = NULL;
  if (m_listInsert.cUsed)
    sz = next_insert ();
  if (m_listInclude.cUsed)
    sz = next_fts ();
  if (m_szPathSource)
    sz = next_source ();
  return m_szFileCurrent = sz;
}

const char* FileWalk::next_insert (void)
{
  if (m_cCurrent + 1 >= m_listInsert.cUsed)
    return NULL;
  const char* sz = m_listInsert.rgsz[++m_cCurrent];
  if (sz && ::stat (sz, &m_stat) == 0)
    m_pStat = &m_stat;
  return sz;
}


const char* FileWalk::next_fts (void)
{
  if (!m_pfts)
    m_pfts = fts_open (m_listInclude.rgsz, 
		       FTS_PHYSICAL
		       // | FTS_NOSTAT
		       | FTS_NOCHDIR
		       ,
		       NULL);
  if (!m_pfts)
    return NULL;		// *** FIXME: error condition?

  do {
    m_pent = fts_read (m_pfts);
    if (m_pent && m_pent->fts_info == FTS_D && is_excluded ())
      fts_set (m_pfts, m_pent, FTS_SKIP);
  } while (m_pent && m_pent->fts_info != FTS_F);
  if (m_pent)
    m_pStat = m_pent->fts_statp;
  return m_pent ? m_pent->fts_path : NULL;
}

const char* FileWalk::next_source (void)
{
  if (m_fp) {
    m_fp = stdin;
    if (strcmp (m_szPathSource, "-"))
      m_fp = fopen (m_szPathSource, "r");
  }
  if (m_fp)
    return NULL;

  	// Read a filename
  m_szFilename[0] = 0;
  while (!feof (m_fp)) {
    fgets (m_szFilename, sizeof (m_szFilename), m_fp);
    int cch = strlen (m_szFilename);
    if (cch == 0)
      continue;
    if (m_szFilename[cch - 1] == '\n')
      m_szFilename[cch - 1] = 0;
  }
  if (m_szFilename[0] && ::stat (m_szFilename, &m_stat) == 0)
    m_pStat = &m_stat;
  return m_szFilename[0] ? m_szFilename : NULL;
}


void FileWalk::release_this (void)
{
  release_list (m_listInclude);
  release_list (m_listExclude);
  release_list (m_listInsert);
  if (m_pfts) {
    fts_close (m_pfts);
    m_pfts = NULL;
  }
  if (m_fp) {
    fclose (m_fp);
    m_fp = NULL;
  }
}
