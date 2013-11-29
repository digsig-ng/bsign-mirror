/* bsign.h						-*- C++ -*-
   $Id: bsign.h,v 1.8 2002/01/16 22:36:59 elf Exp $
   
   written by Oscar Levi
   1 Dec 1998

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

*/

#if !defined (__BSIGN_H__)
#    define   __BSIGN_H__

/* ----- Includes */

#include "exitstatus.h"

/* ----- Globals */

bool is_elf (char* pb, size_t cb);
bool is_elf_header (const char* pb, size_t cb);
bool is_elf_signed (char* pb, size_t cb, 
		    size_t* pibSignature, size_t* pcbSignature);
eExitStatus hash_elf (char* pb, size_t cb, int fhNew, bool fSign);
eExitStatus check_elf (char* pb, size_t cb, bool fExpectSignature);
int size_elf_header (void);

#endif  /* __BSIGN_H__ */
