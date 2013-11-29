/* conversion.cxx
     $Id: conversion.cxx,v 1.2 1998/12/12 06:53:02 elf Exp $

   written by Oscar Levi
   21 November 1998
   
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

   Inlines for converting between little endian-ness and big endian-ness. 

*/

  /* ----- Includes */

#include "standard.h"


  /* ----- Class Globals/Statics */

bool g_fOppositeSex;		// Host and current input file use
				// other byte ordering. 

  /* ----- Methods */

unsigned32 _v (unsigned32 l)
{
  if (!g_fOppositeSex)
    return l;
  return (  ((l & 0xff)     << 24)
	  | ((l >> 24) & 0xff)
	  | ((l & 0xff00)   << 8)
	  | ((l & 0xff0000) >> 8));
}  /* _v */

unsigned32 _vl (unsigned32 l)
{
#if defined (HOST_LSB)
  return l;
#else
  return (  ((l & 0xff)     << 24)
	  | ((l >> 24) & 0xff)
	  | ((l & 0xff00)   << 8)
	  | ((l & 0xff0000) >> 8));
#endif
}  /* _vl */

unsigned32 _vm (unsigned32 l)
{
#if !defined (HOST_LSB)
  return l;
#else
  return (  ((l & 0xff)     << 24)
	  | ((l >> 24) & 0xff)
	  | ((l & 0xff00)   << 8)
	  | ((l & 0xff0000) >> 8));
#endif
}  /* _vm */

unsigned16 _v (unsigned16 s)
{
  if (!g_fOppositeSex)
    return s;
  return ((s & 0xff) << 8) | ((s >> 8) & 0xff);
}  /* _v */

unsigned16 _vl (unsigned16 s)
{
#if defined HOST_LSB
  return s;
#else
  return ((s & 0xff) << 8) | ((s >> 8) & 0xff);
#endif
}  /* _vl */

unsigned16 _vm (unsigned16 s)
{
#if !defined HOST_LSB
  return s;
#else
  return ((s & 0xff) << 8) | ((s >> 8) & 0xff);
#endif
}  /* _vm */

int32 _v (int32 l)
{
  if (!g_fOppositeSex)
    return l;
  return (  ((l & 0xff)     << 24)
	  | ((l >> 24) & 0xff)
	  | ((l & 0xff00)   << 8)
	  | ((l & 0xff0000) >> 8));
}  /* _v */

int32 _vl (int32 l)
{
#if defined HOST_LSB
  return l;
#else
  return (  ((l & 0xff)     << 24)
	  | ((l >> 24) & 0xff)
	  | ((l & 0xff00)   << 8)
	  | ((l & 0xff0000) >> 8));
#endif
}  /* _vl */

int32 _vm (int32 l)
{
#if !defined HOST_LSB
  return l;
#else
  return (  ((l & 0xff)     << 24)
	  | ((l >> 24) & 0xff)
	  | ((l & 0xff00)   << 8)
	  | ((l & 0xff0000) >> 8));
#endif
}  /* _vm */

int16 _v (int16 s)
{
  if (!g_fOppositeSex)
    return s;
  return ((s & 0xff) << 8) | ((s >> 8) & 0xff);
}  /* _v */

int16 _vl (int16 s)
{
#if defined HOST_LSB
  return s;
#else
  return ((s & 0xff) << 8) | ((s >> 8) & 0xff);
#endif
}  /* _vl */

int16 _vm (int16 s)
{
#if !defined HOST_LSB
  return s;
#else
  return ((s & 0xff) << 8) | ((s >> 8) & 0xff);
#endif
}  /* _vm */

/* -------------------------------------------------- */

const char* _xd (unsigned long l) {
  static char sz[80];
  if (l > 15)
    sprintf (sz, "0x%lx (%ld)", l, l);
  else if (l)
    sprintf (sz, "%ld", l);
  else
    sprintf (sz, "-");
  return sz;
}  /* _xd */

const char* _x_d (unsigned long l) {
  static char sz[80];
  if (l)
    sprintf (sz, "0x%lx (%ld)", l, l);
  else
    sprintf (sz, "-");
  return sz;
}  /* _x_d */

const char* _d (unsigned long l) {
  static char sz[80];
  if (l)
    sprintf (sz, "%ld", l);
  else
    sprintf (sz, "-");
  return sz;
}  /* _d */

const char* _x (unsigned long l) {
  static char sz[80];
  if (l)
    sprintf (sz, "0x%lx", l);
  else
    sprintf (sz, "-");
  return sz;
}  /* _x */

const char* _b (unsigned long l, int bits) 
{
  static char sz[80];
  unsigned long mask = 1 << (bits - 1);
  char* pch = sz;
  for (int bit = 0; bit < bits; ++bit, mask >>= 1) {
    if (bit && (bit % 4) == 0)
      *pch++ = ' ';
    *pch++ = ((l & mask) ? '1' : '0');
  }
  *pch = 0;
  return sz;
} /* _b */

int _print_hex (FILE* fp, int cb)
{
  char szAscii[16];

  int ib;
  for (ib = 0; cb; ++ib, --cb) {
    unsigned char b;
    fread (&b, 1, 1, fp);
    if ((ib % 16) == 0)
      printf ("%04x: ", ib);
    szAscii[ib%16] = isprint (b) ? b : '.';
    printf ("%02x ", b);
    switch (ib%16) {
    case 7:
      printf (" ");
      break;
    case 15:
      printf ("%8.8s %8.8s\n", &szAscii[0], &szAscii[8]);
      break;
    }
  }
  if (ib % 16) {
    int cchOffset = 16 - (ib % 16);
    memset (szAscii + (ib % 16), ' ', cchOffset);
    cchOffset = cchOffset*3 + ((ib % 16) < 8 ? 1 : 0);
    printf ("%*.*s%8.8s %8.8s\n", 
	    cchOffset, cchOffset, "", &szAscii[0], &szAscii[8]);
  }
  
  return 0;
}


int _print_hex (const unsigned char* pb, int cb)
{
  char szAscii[16];

  int ib;
  for (ib = 0; cb; ++ib, --cb) {
    unsigned char b;
    b = *pb++;
    if ((ib % 16) == 0)
      printf ("%04x: ", ib);
    szAscii[ib%16] = isprint (b) ? b : '.';
    printf ("%02x ", b);
    switch (ib%16) {
    case 7:
      printf (" ");
      break;
    case 15:
      printf ("%8.8s %8.8s\n", &szAscii[0], &szAscii[8]);
      break;
    }
  }
  if (ib % 16) {
    int cchOffset = 16 - (ib % 16);
    memset (szAscii + (ib % 16), ' ', cchOffset);
    cchOffset = cchOffset*3 + ((ib % 16) < 8 ? 1 : 0);
    printf ("%*.*s%8.8s %8.8s\n", 
	    cchOffset, cchOffset, "", &szAscii[0], &szAscii[8]);
  }
  
  return 0;
}

#if 0

const char* _name_id (int value, ID_SEARCH* rgids)
{
  for (; rgids->sz; ++rgids)
    if (rgids->value == value)
      return rgids->sz;

  static char sz[80];
  sprintf (sz, "unknown id 0x%x %d", value, value);
  return sz;
}

#endif
