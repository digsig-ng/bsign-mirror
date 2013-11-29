/* version.h
     $Id: version.h,v 1.2 1998/12/12 06:53:02 elf Exp $

   written by Oscar Levi
   1 December 1998

   This file is part of the project BSIGN.  See the file README for
   more information.

   Copyright (C) 1998 Oscar Levi

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

   This file declares C statics for the current version number.

*/

#if defined (DECLARE_VERSION)

/* _version.h contains a single #define for SZ_VERSION.  It is created
   automatically from .version_{major,minor,patch} files in the build
   process.  The define takes this form.

   #define SZ_VERSION "1.2.3"
*/

# include "_version.h"
# define SZ_COPYRIGHT "Copyright (C) 1998 The Buici Company."

 const char* g_szVersion = SZ_VERSION;
 const char* g_szCopyright = SZ_COPYRIGHT;

#else

 extern const char* g_szVersion;
 extern const char* g_szCopyright;

#endif
