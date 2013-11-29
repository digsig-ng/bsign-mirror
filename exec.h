/* exec.h							-*- C++ -*-
     $Id: exec.h,v 1.3 1998/12/12 06:53:02 elf Exp $
   
   written by Oscar Levi
   8 Dec 1998

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

#if !defined (__EXEC_H__)
#    define   __EXEC_H__

/* ----- Includes */

/* ----- Globals */

class ExStream {
protected:
  int m_rgfd[2];		// File descriptors for stream read/write
  bool m_fWrite;		// true: parent will use write fd
  int m_fdTarget;		// Target fd for application, -1 for none
  char* m_pb;			// Pointer to read/write data
  size_t m_cb;			// Length of pb buffer
  size_t m_cbUsed;		// Bytes read or written on stream
  bool m_fClosed;		// Closed on other end

public:
  ExStream () { zero (); }
  void zero (void) {
    memset (this, 0, sizeof (*this)); m_fdTarget = -1; }
  ~ExStream () { release_this (); }
  void release_this (void) {
    if (m_rgfd[0])
      close (m_rgfd[0]);
    m_rgfd[0] = 0;
    if (m_rgfd[1])
      close (m_rgfd[1]); 
    m_rgfd[1] = 0; }

  bool is_active (void) {
    return !m_fClosed && m_cbUsed < m_cb; }
  size_t used (void) {
    return m_cbUsed; }

  bool for_reading (char* pb, size_t cb) { 	     // From parent perspective
    if (pipe (m_rgfd))
      return false;
    m_pb = pb;
    m_cb = cb;
    m_fWrite = false;
    return true; }
  bool for_writing (const char* pb, size_t cb) {     // From parent perspective
    if (pipe (m_rgfd))
      return false;
    m_pb = (char*) pb;
    m_cb = cb;
    m_fWrite = true;
    return true; }

  bool for_stdin (const char* pb, size_t cb) {
    return for_writing (pb, cb) && (m_fdTarget = 0, true); }
  bool for_stdout (char* pb, size_t cb) {
    return for_reading (pb, cb) && (m_fdTarget = 1, true); }
  bool for_stderr (char* pb, size_t cb) {
    return for_reading (pb, cb) && (m_fdTarget = 2, true); }

  int fd_read (void) {
    return m_rgfd[0]; }
  int fd_write (void) {
    return m_rgfd[0]; }

  int fd_child (void) {
    return m_rgfd[m_fWrite ? 0 : 1]; }
  int fd_parent (void) {
    return m_rgfd[m_fWrite ? 1 : 0]; }

  static int exec (const char* szCommand, ExStream** rgpexs, int cStream);
};


#endif  /* __EXEC_H__ */
