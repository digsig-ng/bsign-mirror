/* main.cxx
     $Id: main.cxx,v 1.43 2002/01/29 23:29:03 elf Exp $

   written by Oscar Levi
   1 December 1998
   
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

*/

  /* ----- Includes */

#include "standard.h"

#define DECLARE_VERSION
#include "version.h"

#include <sys/mman.h>
#include <sys/stat.h>
//#include <limits.h>
#include <errno.h>

#include "bsign.h"
#include "options.h"
#include "files.h"
#include "exitstatus.h"
#include "filewalk.h"

extern "C" void sha_memory(void*, unsigned32, unsigned32*);


  /* ----- Class Globals/Statics */

int g_fPalette;
int g_fSymbols;
int g_fSections;
int g_fDetail;

int g_fSign;			// !0: hash & sign files 
int g_fHash;			// !0: hash files instead, 0: check signature
bool g_fExpectSignature;	// !0: check for signature as well as hash
bool g_fProcessedFile;		// Set when we do something
bool g_fInverseResult;	        // Negate result for benefit of find program
int g_fNoSymLinks;	        // Treat symlinks as invalid file format
int g_fQuiet;			// Inhibit messages, return code only
int g_fDebug;			// Show debug messages
int g_fVerbose;			// Verbose reports 
int g_fIgnore;			// Ignore directories and non-ELF in error msgs
int g_fHideGoodSigs;		// Hide files with good signatures from output
int g_fSummary;			// Show processing summary at end
int g_fForceResign;		// Resign files that are already signed
const char* g_szOutput;		// Name of output file
const char* g_szFileList;	// Name of file containing file list
const char* g_szPGOptions;	// Options to pass to privacy guard program
FileWalk g_filewalk;		// Global file walking object
char* g_szPathTempHash;		// Pointer to temporary filename for hashing

int g_cFilesProcessed;		// Total files processed
int g_cFilesSkipped;		// Number of files skipped because irrelevent
int g_cFilesInaccessible;	// Number of files that could not be read
int g_cFilesGood;		// Number of files with good hash/signature
int g_cFilesBad;		// Number of files with bad hash/signature

const char* g_szApplication;	// Name of application

void setup_signals (void);

int do_filename (OPTION* pOption, const char* pch);
int do_files (OPTION* pOption, const char* pch);
int do_unrecognized (OPTION*, const char*);
int do_usage (OPTION*, const char*);
int do_version (OPTION*, const char*);
int do_hash (OPTION*, const char*);
int do_sign (OPTION*, const char*);
int do_check (OPTION*, const char*);
int do_unsigned (OPTION*, const char*);
int do_verify (OPTION*, const char*);
int do_include (OPTION*, const char*);
int do_exclude (OPTION*, const char*);

void process_files (void);		// the execution routine

OPTION rgOptions[] =
{
  { "sign",		0, NULL, do_sign  },
  { "s",		0, NULL, do_sign  },

  { "hash",		0, NULL, do_hash  },
  { "H",		0, NULL, do_hash  },

  { "unsigned",		0, NULL, do_unsigned },

  { "quiet",		OPTION_F_SET_INT,			&g_fQuiet },
  { "q",		OPTION_F_SET_INT,			&g_fQuiet },

  { "debug",		OPTION_F_SET_INT,			&g_fDebug },
  { "d",		OPTION_F_SET_INT,			&g_fDebug },

  { "checkhash",	0, NULL, do_check  },
  { "c",		0, NULL, do_check  },

  { "nosymlinks",	OPTION_F_SET_INT,		     &g_fNoSymLinks },

  { "verbose",		OPTION_F_SET_INT,			&g_fVerbose },
  { "v",		OPTION_F_SET_INT,			&g_fVerbose },

  { "verify",		0, NULL, do_verify  },
  { "V",		0, NULL, do_verify  },

  { "output",		OPTION_F_SET_STRING | OPTION_F_ARGUMENT, &g_szOutput },
  { "o",		OPTION_F_SET_STRING | OPTION_F_ARGUMENT, &g_szOutput },

  { "pgoptions",     OPTION_F_SET_STRING | OPTION_F_ARGUMENT, &g_szPGOptions },
  { "P",	     OPTION_F_SET_STRING | OPTION_F_ARGUMENT, &g_szPGOptions },

  { "files",		OPTION_F_ARGUMENT, NULL, do_files },
  { "f",		OPTION_F_ARGUMENT, NULL, do_files },

  { "ignore-unsupported", OPTION_F_SET_INT,		     &g_fIgnore },
  { "I",		OPTION_F_SET_INT,		     &g_fIgnore },

  { "include",		OPTION_F_ARGUMENT, NULL, do_include },
  { "i",		OPTION_F_ARGUMENT, NULL, do_include },

  { "exclude",		OPTION_F_ARGUMENT, NULL, do_exclude },
  { "e",		OPTION_F_ARGUMENT, NULL, do_exclude },

  { "hide-good-sigs",   OPTION_F_SET_INT,	       &g_fHideGoodSigs },
  { "G",		OPTION_F_SET_INT,	       &g_fHideGoodSigs },

  { "summary",		OPTION_F_SET_INT,	       &g_fSummary },
  { "S",		OPTION_F_SET_INT,	       &g_fSummary },

  { "force-resign",	OPTION_F_SET_INT,	       &g_fForceResign },

  { "help",		0, NULL, do_usage				},
  { "h",		0, NULL, do_usage				},

  { "version",		0, NULL, do_version				},
  { "V",		0, NULL, do_version				},

  { "",			OPTION_F_NONOPTION, NULL, do_filename		},
  { "",			OPTION_F_DEFAULT, NULL,	do_unrecognized		},
  { NULL								},
};

  /* ----- Methods */


/* cleanup_temp_hash

   function to remove the temporary used during verification.  Until
   we can verify without writing the data to disk, this is the best we
   can do.  This function is so declared because it is called from the
   signal handlers.

*/

void cleanup_temp_hash (bool fSignal)
{
  if (g_szPathTempHash) {
    int result = unlink (g_szPathTempHash); 
    //    if (result == 0 && fSignal)
    //      fprintf (stderr, "unlinked temp %s\n", g_szPathTempHash);
  }
  if (!fSignal) {
    char* sz = g_szPathTempHash;
    g_szPathTempHash = NULL;	// Prevent signal handler conflict
    delete sz;
  }
}


int do_check (OPTION* /* pOption */, const char* /* pch */)
{
  g_fSign = g_fHash = false;
  g_fExpectSignature = false;
  g_fInverseResult = false;
  return 0;
}

/* do_unsigned

   exit 0 for signed and !1 for unsigned binaries.

*/

int do_unsigned (OPTION*, const char*)
{
  g_fSign = g_fHash = false;
  g_fExpectSignature = true;
  g_fInverseResult = true;
  return 0;
}

int do_verify (OPTION* /* pOption */, const char* /* pch */)
{
  g_fSign = g_fHash = false;
  g_fExpectSignature = true;
  g_fInverseResult = false;
  return 0;
}

int do_sign (OPTION* /* pOption */, const char* /* pch */)
{
  g_fSign = true;
  g_fHash = true;
  g_fInverseResult = false;
  return 0;
}

int do_hash (OPTION* /* pOption */, const char* /* pch */)
{
  g_fSign = false;
  g_fHash = true;
  g_fInverseResult = false;
  return 0;
}

int do_files (OPTION* pOption, const char* pch)
{
  g_filewalk.source (pch);
  return 0;
}


int do_filename (OPTION* pOption, const char* pch)
{
  g_filewalk.insert (pch);	// Add to list of files to process
  return 0;
}


int do_include (OPTION*, const char* pch)
{
  g_filewalk.include (pch);
  return 0;
}


int do_exclude (OPTION*, const char* pch)
{
  g_filewalk.exclude (pch);
  return 0;
}

int do_unrecognized (OPTION* /* pOption */, const char* pch)
{
  fprintf (stderr, "%s: unrecognized option '%s'\n",
	  g_szApplication, pch);
  fprintf (stderr, "Try '%s --help' for usage information.\n",
	  g_szApplication);
  g_exitStatus = invalidargument;
  return 1;
}  /* do_unrecognized */


int do_usage (OPTION*, const char*)
{
  fprintf (stderr, 
"%s version %s\n"
"usage: %s [options] <file> [ [options] <file> ... ]\n"
"  -c, --checkhash      Check hash (*)\n"
"  -d, --debug          Show debug messages\n"
"  -f, --files FILE     Process filenames in FILE, one per line. "
                        "- for stdin\n" 
"  -G, --hide-good-sigs Do not report when good signatures are found\n"
"  -H, --hash           Rewrite files with hash only\n"
"  -I, --ignore-unsupported\n"
"                       Ignore directories and non-ELF files in error "
                        "messages\n"
"      --nosymlinks     Treat symlinks as an unsupported file type\n"
"  -i, --include PATH   add PATH to the list of paths to enumerate\n" 
"  -e, --exclude PATH   add PATH to the list of paths to exclude\n" 
"  -o, --output FILE    Save new version to FILE\n"
"  -P, --pgoptions OPTS Pass OPTS string to the privacy guard program\n" 
"  -q, --quiet          Inhibit messages, call must rely on exit status\n"
"      --force-resign   Force new signatures for already signed files\n"
"  -s, --sign           Rewrite files with hash and signature\n"
"  -S, --summary        Print processing summary after last input file\n"
"  -v, --verbose        Be verbose in reporting program state\n"
"  -V, --verify         Verifies the signature\n"
"      --version        Display version and copyright\n"
"  -h, --help           Usage message\n"
"                   (*) Default option\n"
"\n"
"There are three ways to specify files to process.  Filenames may appear\n"
"on the command line.  The --files switch accepts the name of a file\n"
"containing a list of files to process.  The --include and --exclude\n"  
"options define paths to enumerate and paths exclude.  Only one of these\n"
"methods may be used in a single invocation.\n\n"
"The simplest invocation that will sign all files and exclude the /proc\n"
"directory is this.\n\n"
"  bsign -s -I -i / -e /proc\n\n"
	,
	  g_szApplication, g_szVersion,
	  g_szApplication);

  return 1;
}  /* do_usage */


int do_version (OPTION*, const char*)
{
  fprintf (stdout, "%s\n", g_szVersion);
  return 1;
}  /* do_version */


void process_files (void)
{
  const char* pch;
  while (pch = g_filewalk.next ()) {

    int fh = -1;
    //    char* szFileNew = NULL;
    int fhNew = -1;
    char* pbMap = (char*) -1;
    long cb = 0;
    struct stat& stat = *g_filewalk.stat ();
    char rgb[size_elf_header ()];
    bool fElf = false;

    if (g_filewalk.stat () == NULL) {
      set_exitstatus (filenotfound, 
		      "unable to stat file '%s'", pch);
      goto do_filename_done;
    }

    set_exitstatus (noerror);
    g_fProcessedFile = true;
    ++g_cFilesProcessed;

    if (!S_ISREG (stat.st_mode)) {
      set_exitstatus (unsupportedfiletype, 
		      "unable to process non-regular file '%s'", pch);
      goto do_filename_done;
    }

    fh = open (pch, O_RDONLY);

    //  printf ("do_filename: '%s'\n", pch);

    if (fh == -1) {
      switch (errno) {
      case EACCES:
	set_exitstatus (permissiondenied,
			"insufficient priviledge to open file '%s'", pch);
	++g_cFilesInaccessible;
	break;
      default:
	set_exitstatus (filenotfound, "file not found '%s' %d", pch, errno);
	break;
      }
      goto do_filename_done;
    }
   
    //  fprintf (stderr, "fh %d\n", fh);
    if ((g_fSign | g_fHash)
	&& (g_fNoSymLinks && S_ISLNK (stat.st_mode))) {
      set_exitstatus (unsupportedfiletype, 
		      "ignoring symbolic link '%s'", pch);
      ++g_cFilesSkipped;
      goto do_filename_done;
    }

    if (!(cb = stat.st_size)) {
      set_exitstatus (unsupportedfiletype, "empty file '%s'", pch);
      ++g_cFilesSkipped;
      goto do_filename_done;
    }
    
    if (cb >= size_elf_header ()) {
      fElf = is_elf_header (rgb, read (fh, rgb, size_elf_header ()));
      lseek (fh, SEEK_SET, 0);
    }			    

    // *** FIXME: at the moment, we only work with ELF format files.
    // This test is here in order to avoid mmap'ing the file
    // unnecessarily.

    if (!fElf) {
      set_exitstatus (unsupportedfiletype, "file '%s' is not ELF", pch);
      ++g_cFilesSkipped;
      goto do_filename_done;
    }

      // Open temporary file
    if (g_fSign | g_fHash) {
      char* szPath = path_of (g_szOutput ? g_szOutput : pch);
      //    printf ("path_of '%s'\n", szPath);
      char* szFileNew = new char [strlen (szPath) + 32];
      strcpy (szFileNew, szPath);
      strcat (szFileNew, "/bsign.XXXXXX");
      g_szPathTempHash = szFileNew; // Permit signal cleanup
      fhNew = mkstemp (szFileNew);
      delete szPath;
      //    printf ("fhNew %d\n", fhNew);
      if (fhNew == -1) {
	szPath = path_of (g_szOutput ? g_szOutput : pch);
	set_exitstatus (permissiondenied, 
			"unable to create file in '%s' for '%s'",
			szPath, pch);
	delete szPath;
	++g_cFilesInaccessible;
	goto do_filename_done;
      }
      //     printf ("opened '%s'\n", szFileNew);
    }

    
    if ((pbMap = (char*) mmap (NULL, cb, PROT_READ,
			       MAP_FILE | MAP_PRIVATE, fh, 0))
	== (caddr_t) -1) {
      set_exitstatus (permissiondenied, "unable to mmap '%s'", pch);
      goto do_filename_done;
    }

    if (g_fSign && !g_fForceResign && is_elf_signed (pbMap, cb, NULL, NULL)) {
      set_exitstatus (noerror, "Skipping already signed file");
      ++g_cFilesSkipped;
      goto do_filename_done;
    }

    if (g_fSign || g_fHash) {
      if (is_elf (pbMap, cb)) {
	if (g_fVerbose)
	  fprintf (stdout, "%s\n", pch);
	switch (g_exitStatus = hash_elf (pbMap, cb, fhNew, g_fSign)) {
	case noerror:
	  break;
	case badpassphrase:
	  set_exitstatus (g_exitStatus, "incorrect passphrase"
			  " or gpg not installed");
	  goto do_filename_done;
	default:
	  set_exitstatus (g_exitStatus, "error %d while hashing '%s'",
			  g_exitStatus, pch);
	  goto do_filename_done;
	}
      }      
      else {
	set_exitstatus (unsupportedfiletype, "file '%s' is not ELF", pch);
	++g_cFilesSkipped;
	goto do_filename_done;
      }
    }
    else {			// check only
      if (!is_elf (pbMap, cb)) {
	set_exitstatus (unsupportedfiletype, "file '%s' is not ELF", pch);
	++g_cFilesSkipped;
      goto do_filename_done;
      }

      if (g_fVerbose)
	fprintf (stdout, "%s\n", pch);

      switch (g_exitStatus = check_elf (pbMap, cb, g_fExpectSignature)) {
      case noerror:
	set_exitstatus (g_exitStatus, "good %s found in '%s'.", 
			g_fExpectSignature ? "signature" : "hash", pch);
	++g_cFilesGood;
	break;
      case badhash:
	set_exitstatus (g_exitStatus, "invalid hash in '%s'.", pch);
	++g_cFilesBad;
	break;
      case badsignature:
	set_exitstatus (g_exitStatus, 
			"failed to verify signature in '%s'.", pch);
	++g_cFilesBad;
	break;
      case nohash:
	set_exitstatus (g_exitStatus, "no hash found in '%s'.", pch);
	++g_cFilesBad;
	break;
      case nosignature:
	set_exitstatus (g_exitStatus, "no signature found in '%s'.", pch);
	++g_cFilesBad;
	break;
      case programnotfound:
	set_exitstatus (g_exitStatus, "unable to verify signature, "
			"probably because gpg is not installed");
	break;
      default:
	set_exitstatus (g_exitStatus, "error %d while verifying '%s'.", pch);
	++g_cFilesBad;
	break;
      }
    }

    if (fhNew != -1) {
      close (fhNew);
      fhNew = -1;
      if (!dup_status (g_szPathTempHash, pch)) {
	set_exitstatus (permissiondenied, 
			"insufficient priviledge to duplicate '%s'"
			" ownership", pch);
	++g_cFilesInaccessible;
	goto do_filename_done;
      }
      if (!replace_file (g_szOutput ? g_szOutput : pch, g_szPathTempHash)) {
	// I'd be suprised if I ever get here.
	// I think that by this time, I have
	// already written the file in the
	// same directory and performed chown
	// to set the permissions.
	set_exitstatus (permissiondenied, "unable to create output file '%s'", 
			g_szOutput ? g_szOutput : pch);
	++g_cFilesInaccessible;
	goto do_filename_done;
      }
    }

  do_filename_done:
    if (pbMap != (caddr_t) -1)
      munmap (pbMap, cb);
    if (fhNew != -1)
      close (fhNew);
    cleanup_temp_hash (false);
    close (fh);

    if (!g_fQuiet) {
      if (g_fIgnore 
	  && (   g_exitStatus == isdirectory
	      || g_exitStatus == unsupportedfiletype))
	set_exitstatus_reported ();
      if (g_fHideGoodSigs && g_exitStatus == noerror)
	set_exitstatus_reported ();
      if (!is_exitstatus_reported ()) {
	fprintf (stdout, "%s: %s\n", g_szApplication, g_szExitStatus);
	set_exitstatus_reported ();
      }
    }
    //  if (!g_exitStatus && g_fVerbose && pOption) 
    //    fprintf (stderr, "%s\n", pch);

    //  return 0;			// Always continue working
  }
}


/* main

   entry point.  Note that this main is simplified to an idiom because
   of the options code we use.  Files are processed when non-option
   command line arguments are found.  Refer to OPTION_F_NOPTION.

*/

int main (int argc, char** argv)
{
  setup_signals ();

  g_szApplication = parse_application (*argv);

  int argc_used;
  int result = parse_options (argc - 1, argv + 1, rgOptions, &argc_used);

  if (result) {
    switch (result) {
      case 1:
	break;
      default:
	fprintf (stderr, "parse error %d at word %d\n", result, argc_used + 1);
	break;
    }  /* switch */
    g_exitStatus = invalidargument;
    return 1;
  }  /* if */
  
  if (!g_filewalk.is_ready ())
    set_exitstatus (invalidargument, 
		    "use only one method for specifying input files."); 

  if (g_szOutput && !g_filewalk.is_only_one ())
    set_exitstatus (invalidargument, 
		    "--output option requires one"
		    " and only one input filename");

  if (!is_exitstatus ())
    process_files ();

  //  printf ("done with args (%d)\n", result);


  if (!is_exitstatus () && !g_fProcessedFile) {
    fprintf (stdout, "No input files specified.\n");
    do_usage (NULL, NULL);
  }

     // Invert OK codes when asked to do so
  if (g_fInverseResult) {
    switch (g_exitStatus) {
    case noerror:
    case unsupportedfiletype:
    default:
      g_exitStatus = filenotfound; // Anything to say 'no need to sign it'
      break;
    case nosignature:
    case nohash:
    case badhash:
    case badsignature:
      g_exitStatus = noerror;
      break;
    }
  }

  if (g_fSummary)
    fprintf (stdout, 
	     "Summary processed:%d skipped:%d inaccessible:%d\n"
	     "Results succeeded:%d failed:%d\n",
	     g_cFilesProcessed, g_cFilesSkipped, g_cFilesInaccessible,
	     g_cFilesGood, g_cFilesBad);

  if (is_exitstatus () && !is_exitstatus_reported () && !g_fQuiet)
    fprintf (stdout, "%s: %s\n", g_szApplication, g_szExitStatus);

  //  fprintf (stderr, "exit (%d)\n", g_exitStatus);
  return g_exitStatus;
}  /* main */
