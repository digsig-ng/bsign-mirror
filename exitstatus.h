/* exitstatus.h
   $Id: exitstatus.h,v 1.8 2002/01/29 23:29:03 elf Exp $
   
   written by Oscar Levi
   12 Dec 1998

*/

#if !defined (__EXITSTATUS_H__)
#    define   __EXITSTATUS_H__

/* ----- Includes */

/* ----- Globals */

		// Application result codes.  Note that I will use
		// errno values for the codes that coincide with that
		// set, but I cannot reserve all of them since the
		// result code for a program is only one byte and I'd
		// like to keep these numbers in the low-half of the range.
typedef enum {
  noerror		= 0,	// success; expected result

  permissiondenied	= 1,	// insufficient priviledge for operation
  filenotfound		= 2,	// specified file not found
  nomemory		= 12,	// memory allocation failed (may never be used)
  isdirectory		= 21,	// argument is a directory and must not be
  invalidargument	= 22,	// invalid command line argument
  toomanyopenfiles	= 24,	// pipe call failed
  filebusy		= 26,	// unable to rewrite file as it is in use
  nospace		= 28,	// output device full, no space for new file
  nametoolong		= 36,	// pathname too long

  nohash		= 64,	// hash missing and was expected
  nosignature		= 65,	// signature missing and was expected
  badhash		= 66,	// hash failed verification
  badsignature		= 67,	// signature failed verification
  unsupportedfiletype	= 68,	// type of file not (yet) supported
  badpassphrase	        = 69,	// passphrase given to gpg was incorrect
  rewritefailed		= 70,	// error rewriting 
  quit			= 71,   // premature application termination
  programnotfound	= 72,	// exec failed because program wasn't found
} eExitStatus;

extern eExitStatus g_exitStatus;
extern char g_szExitStatus[256]; /* Exit status error message */

inline bool is_exitstatus (void) {
  return g_exitStatus || g_szExitStatus[0] != 0; }
inline bool is_exitstatus_reported (void) {
  return g_szExitStatus[0] == 0; }
void set_exitstatus (eExitStatus, const char* sz = NULL, ...);
void set_exitstatus_reported (void);

#endif  /* __EXITSTATUS_H__ */
