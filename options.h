/* options.h
     $Id: options.h,v 1.2 1998/12/12 06:53:02 elf Exp $

   written by Oscar Levi
   20 April 1996
   
   This file is part of the project BSIGN.  See the file README for
   more information.

   Copyright (C) 1996 Oscar Levi

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

*/

#ifndef __OPTIONS_H__
#define __OPTIONS_H__

// ----- Inclusions

// ----- Constants

// ----- Typedefs

typedef enum {
  OPTION_F_NONOPTION	= 0x0001,	// Option to use for non-options
  OPTION_F_DEFAULT	= 0x0002,	// Option to use when no other found
  OPTION_F_ARGUMENT	= 0x0004,	// Option has an argument
  OPTION_F_COMMAND	= 0x0008,	// Option is a command, no dash prefix
  OPTION_F_SET_TYPE	= 0x0f00,
  OPTION_F_SET_MASK	= 0xff00,
  OPTION_F_SET_STRING	= 0x0100,
  OPTION_F_SET_INT	= 0x0200,
  OPTION_F_SET_SHORT	= 0x0400,
  OPTION_F_SET_LONG	= 0x0800,
  OPTION_F_CLEAR	= 0x8000,
} E_OPTION_F;

typedef enum {
  OPTION_ERR_OK		= 0,
  OPTION_ERR_FAIL	= 1,		// Option function failed
  OPTION_ERR_UNRECOGNIZED = 2,		// Unrecognized option
  OPTION_ERR_NOARGUMENT	= 3,		// Argument missing
  OPTION_ERR_BADOPTION	= 4,		// Error in option descriptions
  OPTION_ERR_EXIT	= 9,		// Used internally for quick exit
} E_OPTION_ERR;

struct _OPTION;

typedef int (*PFN_OPTION) (struct _OPTION* pOption, const char* pch);

typedef struct _OPTION {
  const char* sz;		// Text of option
  unsigned flags;
  void* pv;			// Pointer to option result
  PFN_OPTION pfn;		// Callback function
} OPTION;

// ----- Classes

// ----- Macros

// ----- Globals / Externals

// ----- Prototypes

char* parse_application (char* pch);
int parse_options (int argc, char** argv, OPTION* rgOptions, int* argc_used);


// ----- Inline

#endif		// __OPTIONS_H__
