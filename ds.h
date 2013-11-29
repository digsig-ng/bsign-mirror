/* ds.h						-*- C++ -*-
     $Id: ds.h,v 1.4 1998/12/12 19:24:46 elf Exp $
   
   written by Oscar Levi
   21 Nov 1998

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

#if !defined (__DS_H__)
#    define   __DS_H__

/* ----- Includes */

#include "exitstatus.h"

/* ----- Globals */


char* create_digital_signature (const char* pb, size_t cb);
eExitStatus verify_digital_signature (const char* pbData, size_t cbData,
				      const char* pbCert, size_t cbCert);


#endif  /* __DS_H__ */
