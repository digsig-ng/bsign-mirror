/* conversion.h
   $Id: conversion.h,v 1.2 1998/12/12 06:53:02 elf Exp $
   
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

#if !defined (__CONVERSION_H__)
#    define   __CONVERSION_H__

/* ----- Includes */

/* ----- Globals */

extern bool g_fOppositeSex;

unsigned32 _v (unsigned32 l);
unsigned16 _v (unsigned16 s);
int32 _v (int32 l);
int16 _v (int16 s);
unsigned32 _vl (unsigned32 l);
unsigned16 _vl (unsigned16 s);
int32 _vl (int32 l);
int16 _vl (int16 s);
unsigned32 _vm (unsigned32 l);
unsigned16 _vm (unsigned16 s);
int32 _vm (int32 l);
int16 _vm (int16 s);


#endif  /* __CONVERSION_H__ */
