/* bsign.cxx
     $Id: bsign.cxx,v 1.26 2003/08/06 21:39:21 elf Exp $

   written by Marc Singer
   1 December 1998 

   This file is part of the project BSIGN.  See the file README for
   more information.

   Copyright (c) 1998,2003 The Buici Company.

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

   Routines to check the signature and hash for a file.  Note that we
   must do some shenanigans to checksum a binary file and put that
   checksum in the file.  Text files are easier since we don't have a
   header to change.  See NOTES for more information about this.

*/

#include "standard.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <assert.h>
#include "version.h"

extern "C" {
#include "sha1.h"
}

#include "conversion.h"
#include "bsign.h"
#include "ds.h"

typedef enum {
  ELF_BITCLASS_NUL		= 0,
  ELF_BITCLASS_32		= 1,
  ELF_BITCLASS_64		= 2,
} E_ELF_BITCLASS;

typedef enum {
  ELF_BYTEORDER_NUL		= 0,
  ELF_BYTEORDER_LSB		= 1,
  ELF_BYTEORDER_MSB		= 2,
} E_ELF_BYTEORDER;

typedef enum {
  ELF_FILETYPE_MASK		= 0xff,
  ELF_FILETYPE_NUL		= 0,
  ELF_FILETYPE_RELOCATABLE	= 1,
  ELF_FILETYPE_EXECUTABLE	= 2,
  ELF_FILETYPE_SHAREDOBJECT	= 3,
  ELF_FILETYPE_CORE		= 4,
} E_ELF_FILETYPE;

typedef enum {
  ELF_CPU_MASK			= 0xff,
  ELF_CPU_NUL			=  0,
  ELF_CPU_WE32K			=  1,	// AT&T WE32100		(MSB)
  ELF_CPU_SPARC			=  2,	// SPARC		(MSB)
  ELF_CPU_I386			=  3,	// Intel 80386		(LSB)
  ELF_CPU_M68K			=  4,	// Motorola 68x000	(MSB)
  ELF_CPU_M88K			=  5,	// Motorola 88000	(LSB)
  ELF_CPU_I486			=  6,	// Intel 80486		(LSB)
  ELF_CPU_I860			=  7,	// Intel 80860		(LSB)
  ELF_CPU_R3000			=  8,	// MIPS RS3000		(MSB)
  ELF_CPU_AMDAHL		=  9,	// Amdahl		(LSB)
  ELF_CPU_R4000			= 10,	// MIPS RS4000		(MSB)
  ELF_CPU_SPARC64		= 11,	// SPARC v9 (64 bit)
  ELF_CPU_HPPA			= 15,	// HP PA RISC
  ELF_CPU_SPARC32P		= 18,	// SPARC v8 plus (32 bit)
  ELF_CPU_PPC			= 20,	// IBM PowerPC
  ELF_CPU_IA64			= 50,   // Intel Itanium (ia64)
  ELF_CPU_PPC_CYG		= 0x9025, // IBM PowerPC by Cygnus Support
} E_ELF_CPU;

typedef enum {
  ELF_SECTION_NUL		=  0,
  ELF_SECTION_PROGBITS		=  1,	// Program specific (private)
  ELF_SECTION_SYMBOLS		=  2,	// Symbol table
  ELF_SECTION_STRINGS		=  3,	// String table
  ELF_SECTION_RELOC_A		=  4,	// Relocation entries with addends
  ELF_SECTION_HASH		=  5,	// Hash table for symbols
  ELF_SECTION_DYNAMIC		=  6,	// Dynamic linking information
  ELF_SECTION_NOTE		=  7,
  ELF_SECTION_NOBITS		=  8,	// Empty section
  ELF_SECTION_RELOC		=  9,	// Relocation entries, no addends
  ELF_SECTION_SHLIB		= 10,	// <Reserved>
  ELF_SECTION_SYMBOLS_D		= 11,	// Dynamic linking symbols
  ELF_SECTION_SIGNATURE		= ((0x80 << 24)|('s' << 16)|('i' << 8)|'g'),
} E_ELF_SECTION;

typedef enum {
  ELF_SECTION_F_WRITABLE	= 0x0001,
  ELF_SECTION_F_ALLOCATE	= 0x0002,
  ELF_SECTION_F_EXECUTABLE	= 0x0004,
} E_ELF_SECTION_F;

typedef enum {
  ELF_PROGRAM_NULL		= 0,
  ELF_PROGRAM_LOAD		= 1, 	// Loadable segment
  ELF_PROGRAM_DYNAMIC		= 2,	// Dynamic linking information
  ELF_PROGRAM_INTERP	        = 3,	// Program interpreter information
  ELF_PROGRAM_NOTE		= 4,	// Auxiliary information
  ELF_PROGRAM_SHLIB		= 5,	// Unspecified semantics (v1.1)
  ELF_PROGRAM_PHDR		= 6,	// Program header table, if loaded
} E_ELF_PROGRAM;

typedef unsigned32 elf32;
typedef unsigned64 elf64;

typedef struct {
  char rgbID[4];		// ID for ELF file "\177ELF"
  unsigned8 bitclass;		// 1, 32 bit; 2, 64 bit
  unsigned8 byteorder;		// 1, LSB; 2, MSB
  char rgbMagic[10];		// Part of the magic number
  unsigned16 filetype;		// 1, reloc; 2, exec; 3, shared; 4, core
  unsigned16 cpu;
  unsigned32 version;
  elf32      addrEntry;		// Virtual address entry point
  elf32	     ibHdrProgram;	// Offset to program header
  elf32	     ibHdrSection;	// Offset to section header
  unsigned32 flags;		// CPU specific flags
  unsigned16 cbHeader;		// Size of header
  unsigned16 cbEntryProgram; 	// Length of each program header entry
  unsigned16 cEntryProgram;	// Count of program header entries
  unsigned16 cbEntrySection; 	// Length of each section header entry
  unsigned16 cEntrySection;	// Count of section header entries
  unsigned16 iSectionNames;	// Section with section names
} HDR_ELF32;

typedef struct {
  char rgbID[4];		// ID for ELF file "\177ELF"
  unsigned8 bitclass;		// 1, 32 bit; 2, 64 bit
  unsigned8 byteorder;		// 1, LSB; 2, MSB
  char rgbMagic[10];		// Part of the magic number
  unsigned16 filetype;		// 1, reloc; 2, exec; 3, shared; 4, core
  unsigned16 cpu;
  unsigned32 version;
  elf64	     addrEntry;		// Virtual address entry point
  elf64	     ibHdrProgram;	// Offset to program header
  elf64	     ibHdrSection;	// Offset to section header
  unsigned32 flags;		// CPU specific flags
  unsigned16 cbHeader;		// Size of header
  unsigned16 cbEntryProgram; 	// Length of each program header entry
  unsigned16 cEntryProgram;	// Count of program header entries
  unsigned16 cbEntrySection; 	// Length of each section header entry
  unsigned16 cEntrySection;	// Count of section header entries
  unsigned16 iSectionNames;	// Section with section names
} HDR_ELF64;

typedef struct {
  unsigned32 programtype;
  unsigned32 ib;
  unsigned32 addrVirtual;
  unsigned32 addrPhysical;
  unsigned32 cbFile;
  unsigned32 cbMemory;
  unsigned32 flags;
  unsigned32 alignment;
} PROGRAM_ELF32;

typedef struct {
  unsigned32 ibName;		// Index to name of section
  unsigned32 sectiontype;
  unsigned32 flags;
  unsigned32 addr;		// Virtual address during execution
  unsigned32 ib;		// Offset to section data
  unsigned32 cb;		// Length of section data
  unsigned32 iLink;		// Index of another section (link?)
  unsigned32 info;
  unsigned32 alignment;
  unsigned32 cbEntry;		// Section table entry size, if applicable
} SECTION_ELF32;
  
typedef struct {
  unsigned32 ibName;		// Index of symbol name
  unsigned32 value;		// Symbol value
  unsigned32 cb;		// Size of symbol
  unsigned8 info;		// Symbol binding info
  unsigned8 other;		// <Reserved>
  unsigned16 iSection;		// Section associated with symbol
} SYMBOL_ELF32;

		// -- FIXUP -- description of change to a file region
typedef struct _FIXUP {
  struct _FIXUP* pNext;		// Link to next fixup
  unsigned32 ib;		// Original location
  unsigned32 cb;		// Original size
  unsigned32 ibNew;		// New location
  unsigned32 cbNew;		// New size
  void* pv;			// Pointer to the new data for this range
  const char* szDescription;	// Debug diagnostic
} FIXUP32;

const char g_szSectionSig[] = "signature"; // Perhaps not the best
					   // name, but it is descriptive
#define CB_SIGNATURE	(512)		   // *** FIXME: hard coded, for now

size_t rewrite_elf32 (char* pb, size_t cb, int fh, FIXUP32& fixup);


void check_byte_sex (const void* pv)
{
#if defined (WORDS_BIGENDIAN)
  g_fOppositeSex = (((HDR_ELF32*)pv)->byteorder != ELF_BYTEORDER_MSB);
#else
  g_fOppositeSex = (((HDR_ELF32*)pv)->byteorder != ELF_BYTEORDER_LSB);
#endif
}


/* sign_file

   generates a hash with or without a digital signature.  The return
   value is a pointer to the data.  In this implementation, the
   signature block is of a fixed length.

   If the return value is NULL, the digital signature could not be
   created. 

*/

char* sign_file (char* pb, size_t cb, size_t ibSignature, size_t cbSignature,
		 bool fCreateCert)
{
  void* pv = malloc (cbSignature);
  memset (pv, 0, cbSignature);

  size_t cbContext; 
  void (*pfn_init)(void*);
  void (*pfn_write)(void*, byte*, size_t);
  void (*pfn_final)(void*);
  byte* (*pfn_read)(void*);
  byte* r_asnoid;		// ?
  int r_asnlen;		// ?
  int r_mdlen;			// Digest length?

  sha1_get_info (2, &cbContext, &r_asnoid, &r_asnlen, &r_mdlen,
		 &pfn_init, &pfn_write, &pfn_final, &pfn_read);

  void* hd = malloc (cbContext);

  (*pfn_init) (hd);
  (*pfn_write) (hd, (byte*) pb, ibSignature);
  (*pfn_write) (hd, (byte*) pv, cbSignature);
  (*pfn_write) (hd, (byte*) pb + ibSignature + cbSignature, 
		cb - ibSignature - cbSignature);
  (*pfn_final) (hd);
  unsigned8* rgb = (*pfn_read) (hd);
  char* rgbSignature = (char*) malloc (CB_SIGNATURE);
  memset (rgbSignature, 0, CB_SIGNATURE);
  char* pbHash = rgbSignature
    + sprintf (rgbSignature, "#1; bsign v%s\n", g_szVersion);
  size_t cbHeader = pbHash - rgbSignature;
  memcpy (pbHash, rgb, 20);
  free (hd);

				// *** generating cert every time
  if (fCreateCert) {
    char* rgbCert = create_digital_signature (rgbSignature, cbHeader + 20);
    if (rgbCert) {
      size_t cbCert = ((unsigned char*)rgbCert)[0]*256
	+ ((unsigned char*)rgbCert)[1] + 2;
      memcpy (pbHash + 20, rgbCert, cbCert);
      free (rgbCert);
    }
    else {
      delete rgbSignature;
      return NULL;
    }
  }

  return rgbSignature;
}


int size_elf_header (void)
{
  return sizeof (HDR_ELF64);	// Largest header size
}


/* is_elf

   returns whether or not we believe we will be successful in
   rewriting the file.  This may require a somewhat indepth
   examination of the elf headers to make sure that all appears well.
   Remember that we expect to find corrupt executables and these may
   be corrupted in subtle ways.  The key, though, is our success is
   rewriting the file.  It is also important to recognize that an file
   with a superficial appearance of being elf that has deeper problems
   is the goal of this project.  Either those problems are simple
   changes to the contents of the file, or the malicious surgery of a
   hacker.  We find it all.

*/

bool is_elf (char* pb, size_t cb)
{
  if (cb < sizeof (HDR_ELF32))
    return false;

  check_byte_sex (pb);

  HDR_ELF32& header = *(HDR_ELF32*) pb;
  if (memcmp (header.rgbID, "\177ELF", 4) != 0
      || header.bitclass < 1 
      || header.bitclass > 2
      || header.byteorder < 1
      || header.byteorder > 2
      || _v (header.filetype) < 1
      || _v (header.filetype) > 4
      || (   _v (header.ibHdrProgram)
	  && _v (header.ibHdrProgram) < sizeof (header))
      || _v (header.ibHdrProgram) >= cb
      || _v (header.ibHdrSection) < sizeof (header)
      || _v (header.ibHdrSection) >= cb
      || _v (header.cbHeader) != sizeof (header)
      || (   _v (header.cbEntryProgram)
	  && _v (header.cbEntryProgram) != sizeof (PROGRAM_ELF32))
      || _v (header.cbEntrySection) != sizeof (SECTION_ELF32)
      || _v (header.iSectionNames) >= _v (header.cEntrySection))
    return false;

  // *** FIXME: I don't recall why we need more than a header test. 

  const PROGRAM_ELF32* rgProgram = (PROGRAM_ELF32*) (pb
						 + _v (header.ibHdrProgram));
  for (int i = 0; i < _v (header.cEntryProgram); ++i)
    if (   _v (rgProgram[i].ib) >= cb
	|| _v (rgProgram[i].ib) + _v (rgProgram[i].cbFile) > cb)
      return false;

  const SECTION_ELF32* rgSection = (SECTION_ELF32*) (pb
						 + _v (header.ibHdrSection));
  for (int i = 1; i < _v (header.cEntrySection); ++i)
    if (_v (rgSection[i].sectiontype) != ELF_SECTION_NOBITS
	&& (_v (rgSection[i].ib) >= cb
	    || _v (rgSection[i].ib) + _v (rgSection[i].cb) > cb))
      return false;

  return true;
}  /* is_elf */


bool is_elf_header (const char* pb, size_t cb)
{
  if (cb < sizeof (HDR_ELF32))
    return false;

  check_byte_sex (pb);

  HDR_ELF32& header = *(HDR_ELF32*) pb;
  if (memcmp (header.rgbID, "\177ELF", 4) != 0
      || header.bitclass < 1 
      || header.bitclass > 2
      || header.byteorder < 1
      || header.byteorder > 2
      || _v (header.filetype) < 1
      || _v (header.filetype) > 4
      || (   _v (header.ibHdrProgram)
	  && _v (header.ibHdrProgram) < sizeof (header))
      || _v (header.ibHdrSection) < sizeof (header)
      || _v (header.cbHeader) != sizeof (header)
      || (   _v (header.cbEntryProgram)
	  && _v (header.cbEntryProgram) != sizeof (PROGRAM_ELF32))
      || _v (header.cbEntrySection) != sizeof (SECTION_ELF32)
      || _v (header.iSectionNames) >= _v (header.cEntrySection))
    return false;
  return true;
}


/* is_elf_signed

   returns true if this file is an elf file and if it is signed.  The
   pibSignature and pcbSignature parameters are set with the position
   and length of the signature block if there is a signature.  Note
   that another call must be made to compute the hash of the file and
   verify the signature.  This function does not distinguish between
   non-elf files and unsigned elf files.

*/

bool is_elf_signed (char* pb, size_t cb, 
		    size_t* pibSignature, size_t* pcbSignature)
{
  if (!is_elf (pb, cb))
    return false;
  const HDR_ELF32& hdr = *(HDR_ELF32*) pb;
  const SECTION_ELF32* rgSection
    = (SECTION_ELF32*) (pb + _v (hdr.ibHdrSection));

  for (int i = 0; i < _v (hdr.cEntrySection); ++i) {
    if (_v (rgSection[i].sectiontype) == unsigned32 (ELF_SECTION_SIGNATURE)) {
      if (pibSignature)
	*pibSignature = _v (rgSection[i].ib);
      if (pcbSignature)
	*pcbSignature = _v (rgSection[i].cb);
      return true;
    }
  }
  return false;
}


/* check_elf

   returns an eAppResult code indicating the success of the
   hash/signature verification.

*/

eExitStatus check_elf (char* pb, size_t cb, bool fExpectSignature)
{
  if (!is_elf (pb, cb))
    return unsupportedfiletype;
  size_t ibSignature;
  size_t cbSignature;
  if (!is_elf_signed (pb, cb, &ibSignature, &cbSignature))
    return fExpectSignature ? nosignature : nohash;
  size_t ibHash = ibSignature;
  for (ibHash = ibSignature; ibHash < ibSignature + cbSignature - 20; ++ibHash)
    if (pb[ibHash] == '\n') {
      ++ibHash;
      break;
    }
  if (fExpectSignature && ibHash == ibSignature + cbSignature - 20)
    return nosignature;		

  char* rgbSignature = sign_file (pb, cb, ibSignature, cbSignature, false);
  char* pbSignature;
  for (pbSignature = rgbSignature;
       pbSignature < rgbSignature + CB_SIGNATURE; ++pbSignature)
    if (*pbSignature == '\n') {
      ++pbSignature;
      break;
    }

  eExitStatus result = 
    ((memcmp (pbSignature, pb + ibHash, 20) != 0) ? badhash : noerror);
  free (rgbSignature);
  if (result || !fExpectSignature)
    return result;

				// Check digital signature if hash OK
  size_t cbCert = (((unsigned char*)pb)[ibHash + 20] << 8) 
    | ((unsigned char*)pb)[ibHash + 21];
  if (cbCert == 0 || cbCert > cbSignature - (ibHash - ibSignature + 20))
    return badsignature;
  return verify_digital_signature (pb + ibSignature,
				   ibHash - ibSignature + 20, 
				   pb + ibHash + 22, cbCert);
}


/* hash_elf

   generates a signature section for a ELF file.  The algorithm
   rewrites the file, removing the old signature if there was one,
   hashes this portion, and appends the new signature to the end.
   This depends only on us knowning the length of the cert.  In
   theory, we could put our signature section anywhere in the file,
   but this layout is convenient at the time.  The most likely
   enhancement from here would be the ability to update a signature in
   place, making this function more efficient, and making the hashing
   a little more difficult.

   *** FIXME: the return value is an error code.  We need to establish
   a set of these so we can determine the cause of failures.  We don't
   want to appears foolish like Microsoft.

*/

eExitStatus hash_elf (char* pb, size_t cb, int fhNew, bool fSign)
{
  check_byte_sex (pb);

  FIXUP32 fixupSignature;
  memset (&fixupSignature, 0, sizeof (fixupSignature));
  fixupSignature.cb = CB_SIGNATURE;

				// Perform rewrite
  size_t cbRewrite = rewrite_elf32 (pb, cb, fhNew, fixupSignature);
  if (cbRewrite == (size_t) -1)
    return rewritefailed;

  char* pbMap = (char*) mmap (NULL, cbRewrite, PROT_READ, MAP_FILE |
			      MAP_PRIVATE, fhNew, 0);
  assert (pbMap);

  char* rgbSignature = sign_file (pbMap, cbRewrite, 
				  fixupSignature.ibNew, fixupSignature.cbNew, 
				  fSign);
  munmap (pbMap, cbRewrite);

  if (rgbSignature) {
    lseek (fhNew, fixupSignature.ibNew, SEEK_SET);
    write (fhNew, rgbSignature, CB_SIGNATURE);
    delete rgbSignature;
    return noerror;
  }
  return badpassphrase;
}


/* new_sectionnames

   returns a buffer containing sections names with the name of the
   signature buffer added to the end.  The first parameter is a
   pointer to the file in memory.

*/

char* new_sectionnames32 (char* pb, size_t cb, 
			  SECTION_ELF32& sectionNames, 
			  SECTION_ELF32& sectionSig)
{
  const HDR_ELF32& hdr = *(HDR_ELF32*) pb;

  sectionSig.ibName = sectionNames.cb;
  sectionSig.alignment = _v (unsigned32 (1));
  sectionSig.sectiontype = _v (unsigned32 (ELF_SECTION_SIGNATURE));
  unsigned32 cbSectionNamesNew = _v (sectionNames.cb)
    + sizeof (g_szSectionSig);
  cbSectionNamesNew = (cbSectionNamesNew + 3) & ~3; // Round to words
  
  char* sz = (char*) malloc (cbSectionNamesNew);
  memcpy (sz, pb + _v (sectionNames.ib), _v (sectionNames.cb));
  memcpy (sz + _v (sectionNames.cb), g_szSectionSig, sizeof (g_szSectionSig));

  sectionNames.cb = _v (cbSectionNamesNew);

  return sz;
}


void fixup_range32 (const unsigned32& ibOld, const unsigned32& cbOld,
		    unsigned32& ib, unsigned32& cb, const FIXUP32& fixup)
{
				// Fixup occurs after this range
  if (fixup.ib >= _v (ibOld) + _v (cbOld))
    return;
  
				// Fixup preceeds this range
  if (_v (ibOld) >= fixup.ib + fixup.cb) {
    ib += fixup.cbNew - fixup.cb;
    return;
  }

				// *** FIXME: weak sanity check
  assert (fixup.ib >= _v (ibOld));
				// elf: in fact, this next check is so
				// weak it is wrong.  In rereading
				// this line, I cannot tell WHAT I was
				// thinking.
  //    assert (fixup.ib + fixup.cb <= _v (ibOld) + _v (cbOld));
      
				// Fixup within this program
  cb += fixup.cbNew - fixup.cb;
}
		  

/* fixup_fixups

   fills the ibNew fields so we can write the output file
   conveniently.  The *best* algorithm would sort the list before
   computing the ibNew fields.  Instead, we implement an O(n^2)
   routine that checks every fixup with every other fixup.  With
   anything less than a dozen, this is certainly OK.

   We make the reasonable assumption that the fixups don't overlap.

   The return value is the number of bytes added to the file due to
   these fixups.

*/

unsigned32 fixup_fixups32 (FIXUP32* pFixupHead)
{
  unsigned32 cb = 0;
  for (FIXUP32* pFixupOuter = pFixupHead; pFixupOuter; 
       pFixupOuter = pFixupOuter->pNext) {
    pFixupOuter->ibNew = pFixupOuter->ib;
    cb += pFixupOuter->cbNew - pFixupOuter->cb;
    for (FIXUP32* pFixupInner = pFixupHead; pFixupInner; 
	 pFixupInner = pFixupInner->pNext)
      if (pFixupInner->ib < pFixupOuter->ib)
	pFixupOuter->ibNew += pFixupInner->cbNew - pFixupInner->cb;
  }

  return cb;
}


void fixup_elf32_programs (const HDR_ELF32& hdr, 
			   PROGRAM_ELF32* rgProgram, 
			   const PROGRAM_ELF32* rgProgramOld,
			   const FIXUP32* rgFixup)
{
  for (int iProgram = 0; iProgram < _v (hdr.cEntryProgram); ++iProgram)
    for (const FIXUP32* pFixup = rgFixup; pFixup; pFixup = pFixup->pNext)
      fixup_range32 (rgProgramOld[iProgram].ib, rgProgramOld[iProgram].cbFile,
		     rgProgram[iProgram].ib, rgProgram[iProgram].cbFile,
		     *pFixup);
}

void fixup_elf32_sections (const HDR_ELF32& hdr, 
			   SECTION_ELF32* rgSection, 
			   const SECTION_ELF32* rgSectionOld,
			   const FIXUP32* rgFixup)
{
  for (int iSection = 0; iSection < _v (hdr.cEntrySection); ++iSection)
    for (const FIXUP32* pFixup = rgFixup; pFixup; pFixup = pFixup->pNext)
      fixup_range32 (rgSectionOld[iSection].ib, rgSectionOld[iSection].cb,
		     rgSection[iSection].ib, rgSection[iSection].cb,
		     *pFixup);
}

void fixup_elf32_header (HDR_ELF32& hdr, const HDR_ELF32& hdrOld, 
			 const FIXUP32* rgFixup)
{
  for (const FIXUP32* pFixup = rgFixup; pFixup; pFixup = pFixup->pNext) {
    unsigned32 cbOld;
    unsigned32 cb;

    if (_v (hdrOld.cEntryProgram)) {
      cbOld = _v (unsigned32 (_v (hdrOld.cbEntryProgram)
			      *_v (hdrOld.cEntryProgram)));
      cb = _v (unsigned32 (_v (hdr.cbEntryProgram)*_v (hdr.cEntryProgram)));
      fixup_range32 (hdrOld.ibHdrProgram, cbOld, 
		     hdr.ibHdrProgram, cb, *pFixup);
    }

    cbOld = _v (unsigned32 (_v (hdrOld.cbEntrySection)
			    *_v (hdrOld.cEntrySection)));
    cb = _v (unsigned32 (_v (hdr.cbEntrySection)*_v (hdr.cEntrySection)));
    fixup_range32 (hdrOld.ibHdrSection, cbOld, hdr.ibHdrSection, cb, *pFixup);
  }
}

void report_elf32 (const HDR_ELF32& hdr, const SECTION_ELF32* rgSection)
{
  fprintf (stderr, "%08x %08x header\n", 0, sizeof (hdr));
  fprintf (stderr, "%08x %08x program header\n", 
	   _v (hdr.ibHdrProgram),
	   _v (hdr.cbEntryProgram)*_v (hdr.cEntryProgram));
  fprintf (stderr, "%08x %08x section header\n", 
	   _v (hdr.ibHdrSection),
	   _v (hdr.cbEntrySection)*_v (hdr.cEntrySection));
  for (int i = 0; i < _v (hdr.cEntrySection); ++i) {
    fprintf (stderr, "%08x %08x section %d\n", 
	     _v (rgSection[i].ib), 
	     _v (rgSection[i].cb), i);
  }
}

void report_fixup (const FIXUP32* pFixup)
{
  fprintf (stderr, "%8d l%8d->%8d %8d l%8d->%8d %s\n",
	   pFixup->ib, pFixup->cb, pFixup->ib + pFixup->cb,
	   pFixup->ibNew, pFixup->cbNew, pFixup->ibNew + pFixup->cbNew,
	   pFixup->szDescription);
}

void report_fixups (const FIXUP32* pFixup)
{
  for (; pFixup; pFixup = pFixup->pNext)
    report_fixup (pFixup);
}


/* rewrite_elf

   rewrites an elf file making the appropriate changes to support a
   signature section.  The fixupSignature section must contain a cb
   value large enough to contain the signature data.  The
   fixupSignature ibNew and cbNew will point to the place in the new
   file where the signature is written.  If the file already has a
   signature section and if that section is at least as large as
   requested, the file will be copied.  The portion of the file to
   hold the signature is *always* initialized to nulls.

   Note, we count on the first fixup being the new signature space
   when we do the complex rewrite.  This is done to make it easy to
   free the blank buffer and to recover the new signature location.

   We return -1 when there's an error during rewrite.

*/

size_t rewrite_elf32 (char* pb, size_t cb, int fh, FIXUP32& fixupSignature)
{
  const HDR_ELF32 hdrOld = *(HDR_ELF32*) pb;

  if (sizeof (hdrOld) != _v (hdrOld.cbHeader)) 		// We must have this
    return 0;
  if (_v (hdrOld.cEntryProgram)
      && sizeof (PROGRAM_ELF32)
         != _v (hdrOld.cbEntryProgram))			// We must have this
    return 0;
  if (_v (hdrOld.cEntrySection)
      && sizeof (SECTION_ELF32) 
         != _v (hdrOld.cbEntrySection)) 		// We must have this
    return 0;

  const SECTION_ELF32* rgSectionOld
    = (SECTION_ELF32*) (pb + _v (hdrOld.ibHdrSection));

  FIXUP32 fixupOldSignature;
  int iSectionOldSignature = 0;
  memset (&fixupOldSignature, 0, sizeof (fixupOldSignature));

				// Check to see if the file has a
				// sufficiently large signature section
  for (int i = 0; i < _v (hdrOld.cEntrySection); ++i) {
    if (_v (rgSectionOld[i].sectiontype)
	== unsigned32 (ELF_SECTION_SIGNATURE)) {
      fixupOldSignature.ib = _v (rgSectionOld[i].ib);
      fixupOldSignature.cb = _v (rgSectionOld[i].cb);
      iSectionOldSignature = i;
      break;
    }
  }
				// Data block for initializing signature
  fixupOldSignature.ibNew = fixupOldSignature.ib;
				// Old signature sufficient, easiest solution
  if (fixupOldSignature.cb >= fixupSignature.cb) {
    void* pv = malloc (fixupOldSignature.cb);
    memset (pv, 0, fixupOldSignature.cb);
    write (fh, pb, fixupOldSignature.ib);
    write (fh, pv, fixupOldSignature.cb);
    write (fh, pb + fixupOldSignature.ib + fixupOldSignature.cb,
	   cb - fixupOldSignature.ib - fixupOldSignature.cb);
    free (pv);
    fixupSignature = fixupOldSignature;
    fixupSignature.cbNew = fixupOldSignature.cb;
    return cb;
  }

				// Make new program and section headers
  HDR_ELF32 hdr = *(HDR_ELF32*) pb;
  const PROGRAM_ELF32* rgProgramOld
    = (PROGRAM_ELF32*) (pb + _v (hdr.ibHdrProgram));
  PROGRAM_ELF32* rgProgram
    = hdr.ibHdrProgram ? new PROGRAM_ELF32[_v (hdr.ibHdrProgram)] : NULL;
  if (rgProgram)
    memcpy (rgProgram, rgProgramOld,
	    _v (hdr.cEntryProgram)*_v (hdr.cbEntryProgram));
  SECTION_ELF32* rgSection = new SECTION_ELF32[_v (hdr.cEntrySection) 
					       + (iSectionOldSignature
						  ? 0 : 1)];
  memcpy (rgSection, rgSectionOld,
	  _v (hdr.cEntrySection)*_v (hdr.cbEntrySection));

  //  report_elf (hdr, rgSectionOld);

  SECTION_ELF32& sectionSig   = rgSection[_v (hdr.cEntrySection)];
  SECTION_ELF32& sectionNames = rgSection[iSectionOldSignature
					  ? iSectionOldSignature 
					  : _v (hdr.iSectionNames)];
  memset (&sectionSig, 0, sizeof (sectionSig));

  char* rgbSectionNames
    = new_sectionnames32 (pb, cb, sectionNames, sectionSig);
  if (!iSectionOldSignature)
    hdr.cEntrySection = _v (_v (hdr.cEntrySection) + (unsigned32) 1);

				// Generate fixup lists
  FIXUP32* pFixup	= new FIXUP32;
  memset (pFixup, 0, sizeof (*pFixup));
  pFixup->ib		= _v (rgSectionOld[_v (hdr.iSectionNames)].ib);
  pFixup->cb		= _v (rgSectionOld[_v (hdr.iSectionNames)].cb);
  pFixup->cbNew		= _v (rgSection[_v (hdr.iSectionNames)].cb);
  pFixup->pv		= rgbSectionNames;
  pFixup->szDescription = "section names";
  FIXUP32* pFixupHead	= pFixup;

  pFixup		= new FIXUP32;
  memset (pFixup, 0, sizeof (*pFixup));
  pFixup->pNext	        = pFixupHead;
  pFixupHead		= pFixup;
  pFixup->ib		= _v (hdrOld.ibHdrSection);
  pFixup->cb		= _v (hdrOld.cEntrySection)*_v (hdrOld.cbEntrySection);
  pFixup->cbNew		= _v (hdr.cEntrySection)*_v (hdr.cbEntrySection);
  pFixup->pv		= rgSection;
  pFixup->szDescription = "entry section";

  if (_v (hdrOld.cEntryProgram)) {
    pFixup		= new FIXUP32;
    memset (pFixup, 0, sizeof (*pFixup));
    pFixup->pNext       = pFixupHead;
    pFixupHead		= pFixup;
    pFixup->ib		= _v (hdrOld.ibHdrProgram);
    pFixup->cb		= _v (hdrOld.cEntryProgram)*_v (hdrOld.cbEntryProgram);
    pFixup->cbNew	= _v (hdr.cEntryProgram)*_v (hdr.cbEntryProgram);
    pFixup->pv		= rgProgram;
    pFixup->szDescription = "entry program";
  }

  pFixup		= new FIXUP32;
  memset (pFixup, 0, sizeof (*pFixup));
  pFixup->pNext	        = pFixupHead;
  pFixupHead		= pFixup;
  pFixup->ib		= 0;
  pFixup->cb		= sizeof (hdr);
  pFixup->cbNew		= sizeof (hdr);
  pFixup->pv		= &hdr;
  pFixup->szDescription = "header";

				// Clobber old signature
  if (fixupOldSignature.cb) {
    pFixup		= new FIXUP32;
    memcpy (pFixup, &fixupOldSignature, sizeof (FIXUP32));
    pFixup->szDescription = "old signature";
    pFixup->pNext	        = pFixupHead;
    pFixupHead		= pFixup;
  }
				// Make room for new signature
  pFixup		= new FIXUP32;
  memset (pFixup, 0, sizeof (*pFixup));
  pFixup->pNext	        = pFixupHead;
  pFixupHead		= pFixup;
  pFixup->ib		= cb;
  pFixup->cb		= 0;
  pFixup->cbNew		= fixupSignature.cb;
  pFixup->pv		= malloc (pFixup->cbNew);
  pFixup->szDescription = "signature";
  memset (pFixup->pv, 0, pFixup->cbNew);
  
  //  printf ("@perform fixups\n"); report_fixups (pFixupHead);

				// Perform fixups
  unsigned32 cbNew = fixup_fixups32 (pFixupHead) + cb;
  fixup_elf32_header (hdr, hdrOld, pFixupHead);
  if (_v (hdr.cEntryProgram))
    fixup_elf32_programs (hdrOld, rgProgram, rgProgramOld, pFixupHead);
  fixup_elf32_sections (hdrOld, rgSection, rgSectionOld, pFixupHead);

  //  report_fixups (pFixupHead);
  sectionSig.ib = pFixupHead->ibNew;
  sectionSig.cb = pFixupHead->cbNew;

  //  printf ("@rewrite\n"); report_fixups (pFixupHead);

				// Rewrite
  size_t cbWritten = 0;
  unsigned32 ib = 0;
  unsigned32 ibFile = 0;
  while (ib < cbNew) {
    FIXUP32* pFixupNext = NULL;
    for (pFixup = pFixupHead; pFixup; pFixup = pFixup->pNext)
      if (pFixup->ibNew >= ib 
	  && (pFixupNext == NULL || pFixup->ibNew < pFixupNext->ibNew))
	pFixupNext = pFixup;

    unsigned32 cbWrite;
    if (pFixupNext && pFixupNext->ibNew == ib) {
      //      report_fixup (pFixupNext);
      cbWrite = write (fh, pFixupNext->pv, pFixupNext->cbNew);
      if (cbWrite != pFixupNext->cbNew) {
	cbWritten = (size_t) -1;
	break;
      }
      ibFile += pFixupNext->cb;
    }
    else {
      size_t cb = (pFixupNext ? pFixupNext->ibNew : cbNew) - ib;
      //      printf ("%8ld l%8d->%8ld from file\n", ibFile, cb, ib + cb);
      cbWrite = write (fh, pb + ibFile, cb);
      if (cbWrite != cb) {
	cbWritten = (size_t) -1;
	break;
      }
      ibFile += cb;
    }
    cbWritten += cbWrite;
    ib += cbWrite;
  }

				// Cleanup
  free (rgbSectionNames);
  // *** FIXME: release fixups and sections and program

  fixupSignature = *pFixupHead;
  fixupSignature.pv = 0;
  assert (pFixupHead && pFixupHead->pv);
  free (pFixupHead->pv);

  while (pFixupHead) {
    pFixup = pFixupHead;
    pFixupHead = pFixup->pNext;
    delete pFixup;
  }

  return cbWritten;
}
