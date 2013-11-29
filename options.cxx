/* options.cxx
     $Id: options.cxx,v 1.3 1998/12/12 06:53:02 elf Exp $

   written by Oscar Levi
   1 December 1996
   
   This file is part of the project BSIGN.  See the file README for
   more information.

   Copyright (C) 1996,1998 The Buici Company.

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

   ---------------------
   ABBREVIATED CHANGELOG
   ---------------------

   Option parsing and value storage.

   Version 0.3 (14 November 1996)
     Permit option arguments to be prefixed by '='.
   Version 0.2 (14 May 1996)
     Long option names prefixed by a period are interpreted without a
     dash.  This allows command parsing as well as option parsing.
   Version 0.1
     Short and long options.  Default and error option codes.  Long
     option prefix matching.

*/

  /* ----- Includes */

#include "standard.h"
#include "options.h"


#define USE_PARTIALS		// Permit partial matches for long options


  /* ----- Class Globals/Statics */

typedef enum {
  STATE_0			= 0,
  STATE_FOUND_DASH		= 1,
  STATE_FOUND_DASHDASH		= 2,
} STATE_PARSE;

typedef enum {
  FETCH_DEFAULT			= 0,
  FETCH_SHORT			= 1,
  FETCH_LONG			= 2,
  FETCH_COMMAND			= 3,
} FETCH_MODE;


static int eval_option (char* pch, int cch, char* pchArgument, 
			OPTION* pOptions);
static OPTION* fetch_option (char* pch, FETCH_MODE mode, OPTION* rgOptions);


  /* ----- Methods */

char* parse_application (char* pch)
{
  char* pchSep    = rindex (pch, '\\');
  char* pchSepAlt = rindex (pch, '/');
  char* pchColon  = rindex (pch, ':');
  char* pchDot    = rindex (pch, '.');

  if (pchSepAlt > pchSep)
    pchSep = pchSepAlt;
  if (pchColon > pchSep)
    pchSep = pchColon;
  pch = pchSep ? pchSep + 1 : pch;

  if (pchDot && strcasecmp (pch, ".exe"))
    *pchDot = 0;
  return pch;
}  /* parse_applications */


/* parse_options

   accepts the argument vector for the application and an option
   description array and parses the command line arguments.  The
   return value is zero if the parse succeeds or non-zero if there
   was an error.

*/

int parse_options (int argc, char** argv, OPTION* rgOptions, int* pargc_used)
{
  int argc_used;
  if (!pargc_used)
    pargc_used = &argc_used;

  int result = 0;

  for (; argc; --argc, ++argv, ++*pargc_used) {
    char* pch;
    int state = STATE_0;
    for (pch = *argv; *pch; ++pch) {
      OPTION* pOption;
      char* pchArgument = NULL;
      int cch;			// General purpose length storage

      switch (state) {

	case STATE_0:
	  if (*pch == '-') {
	    state = STATE_FOUND_DASH;
	    continue;
	  }  /* if */

	  if ((pOption = fetch_option (pch, FETCH_COMMAND, rgOptions))) {
	    if ((result = eval_option (pch, strlen (pch), NULL, pOption)))
	      return result;
	  }  /* if */
	  else if ((pOption = fetch_option (NULL, FETCH_DEFAULT, rgOptions))) {
	    if ((result = eval_option (NULL, 0, pch, pOption)))
	      return result;
	  }  /* else-if */
	  else
	    return OPTION_ERR_OK;
	  pch += strlen (pch) - 1;
	  break;

	case STATE_FOUND_DASH:
	  if (*pch == '-') {
	    state = STATE_FOUND_DASHDASH;
	    continue;
	  }  /* if */

	  pOption = fetch_option (pch, FETCH_SHORT, rgOptions);
	  if (!pOption)
	    return OPTION_ERR_UNRECOGNIZED;

	  if (pOption->flags & OPTION_F_ARGUMENT) {
	    if (*(pch + 1)) {
	      pchArgument = pch + 1;
	      if (*pchArgument == '=')
		++pchArgument;
	    }
	    else if (argc > 1) {
	      pchArgument = argv[1];
	      --argc;
	      ++argv;
	      ++*pargc_used;
	    }  /* else */
	    else
	      return OPTION_ERR_NOARGUMENT;
	  }  /* if */

	  if ((result = eval_option (pch, 1, pchArgument, pOption)))
	    return result;

	  if (pchArgument) {
	    pch += strlen (pch) - 1;
	    state = STATE_0;
	  }  /* if */
	  break;

	case STATE_FOUND_DASHDASH:
	  pOption = fetch_option (pch, FETCH_LONG, rgOptions);
	  if (!pOption)
	    return OPTION_ERR_UNRECOGNIZED;

	  cch = strlen (pch);
	  if (pOption->flags & OPTION_F_ARGUMENT) {
	    int cchOption = strcspn (pch, "=");
	    if (pch[cchOption] == '=') {
	      pchArgument = pch + cchOption + 1;
	      cch = cchOption;
	    }
	    else if (argc > 1) {
	      pchArgument = argv[1];
	      --argc;
	      ++argv;
	      ++*pargc_used;
	    }  /* else */
	    else
	      return OPTION_ERR_NOARGUMENT;
	  }  /* if */

	  if ((result = eval_option (pch, cch, pchArgument, pOption)))
	    return result;

	  pch += strlen (pch) - 1;

	  state = STATE_0;
	  break;
      }  /* switch */
    }  /* for */
  }  /* for */
  return OPTION_ERR_OK;
}  /* parse_options */


/* fetch_option

   accepts a pointer into a word within the command line, usually
   after either a single or double dash, and a mode specifier that
   determines which types of options it can match.  It then tries to
   match that option string in the option data.  Long options and
   command options will match partials if the USE_PARTIALS macro is
   defined. 

 */

OPTION* fetch_option (char* pch, FETCH_MODE mode, OPTION* rgOptions)
{
  int cch = ((mode == FETCH_SHORT) ? 1 : (pch ? strlen (pch) : 0));
  if (cch) {
    int cchOption = strcspn (pch, "=");
    if (cchOption < cch)
      cch = cchOption;
  }

  OPTION* pOptionDefault = NULL;
  OPTION* pOptionPartial = NULL;

  for (OPTION* pOption = rgOptions; pOption && pOption->sz; ++pOption) {
    if (   mode == FETCH_DEFAULT
	&& (pOption->flags & OPTION_F_DEFAULT)
	&& !pOptionDefault)
      pOptionDefault = pOption;

    if (!pch) {
      if (pOption->flags & OPTION_F_NONOPTION)
	return pOption;
      continue;
    }  /* if */

    if (*pOption->sz != *pch)
      continue;

    if (   (mode == FETCH_COMMAND && !(pOption->flags & OPTION_F_COMMAND))
	|| (mode != FETCH_COMMAND &&  (pOption->flags & OPTION_F_COMMAND)))
      continue;

    if (cch == 1 && mode != FETCH_COMMAND) {
      if (!pOption->sz[1])
	return pOption;
      continue;
    }  /* if */
    if (strncmp (pch, pOption->sz, cch))
      continue;
    if (!pOption->sz[cch])
      return pOption;
#if defined USE_PARTIALS
    pOptionPartial = pOptionPartial ? (OPTION*) -1 : pOption;
#endif
  }  /* for */

  return (pOptionPartial && pOptionPartial != (OPTION*) -1)
      ? pOptionPartial
      : pOptionDefault;
}  /* fetch_option */


/* eval_option

   accepts a pointer to the user's option string, a pointer to the
   option argument, and the fetched option pointer.

 */

int eval_option (char* pch, int cch, char* pchArgument, OPTION* pOption)
{
  if (pOption->pfn) {
    char sz[2];
    if (pOption->flags & OPTION_F_DEFAULT) {
      if (cch == 1) {
	sz[0] = *pch;
	sz[1] = 0;
	pchArgument = sz;
      }  /* if */
      else
	pchArgument = pch;
    }  /* if */
    return pOption->pfn (pOption, pchArgument)
      ? OPTION_ERR_FAIL
      : OPTION_ERR_OK;
  }  /* if */

  if (pOption->flags & OPTION_F_DEFAULT) {
    printf ("Unrecognized option '%.*s'\n", cch, pch);
    return OPTION_ERR_UNRECOGNIZED;
  }

  if (pOption->flags & OPTION_F_NONOPTION)
    return OPTION_ERR_EXIT;

  if ((pOption->flags & OPTION_F_SET_MASK) && pOption->pv) {
    long value = 1;
    if (pchArgument && !(pOption->flags & OPTION_F_SET_STRING))
      value = strtol (pchArgument, NULL, 0);
    else if (pOption->flags & OPTION_F_CLEAR)
      value = 0;

    switch (pOption->flags & OPTION_F_SET_TYPE) {
      default:
	return OPTION_ERR_BADOPTION;
      case OPTION_F_SET_INT:
	*(int*)   pOption->pv = int   (value);
	break;
      case OPTION_F_SET_SHORT:
	*(short*) pOption->pv = short (value);
	break;
      case OPTION_F_SET_LONG:
	*(long*)  pOption->pv = long  (value);
	break;
      case OPTION_F_SET_STRING:
	*(char**) pOption->pv = pchArgument;
	break;
    }  /* switch */

    return OPTION_ERR_OK;
  }  /* if */
  else
    return OPTION_ERR_BADOPTION;
}  /* eval_option */
