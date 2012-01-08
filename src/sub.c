/**************************************************************************/
/*                                                                        */
/*  weex - a non-interactive mirroring utility for updating web pages     */
/*  Copyright (C) 1999-2000 Yuuki NINOMIYA <gm@debian.or.jp>              */
/*                                                                        */
/*  This program is free software; you can redistribute it and/or modify  */
/*  it under the terms of the GNU General Public License as published by  */
/*  the Free Software Foundation; either version 2, or (at your option)   */
/*  any later version.                                                    */
/*                                                                        */
/*  This program is distributed in the hope that it will be useful,       */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of        */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         */
/*  GNU General Public License for more details.                          */
/*                                                                        */
/*  You should have received a copy of the GNU General Public License     */
/*  along with this program; if not, write to the                         */
/*  Free Software Foundation, Inc., 59 Temple Place - Suite 330,          */
/*  Boston, MA 02111-1307, USA.                                           */
/*                                                                        */
/**************************************************************************/

/* $Id: sub.c,v 1.1.1.1 2002/08/30 19:24:53 gmkun Exp $ */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#ifdef HAVE_GNUREGEX_H
#  include <gnuregex.h>
#else
#  include <regex.h>
#endif

#include "intl.h"
#include "strlib.h"
#include "variable.h"
#include "proto.h"


/* --- PUBLIC FUNCTIONS --- */

long n_atol(const char *str, int n)
{
	char *temp;
	long result;

	temp = str_malloc(n + 1);
	strncpy(temp, str, n);
	temp[n] = '\0';

	result = atol(temp);

	free(temp);
	return (result);
}


int is_match_regexp(const char *regexp, const char *string, int host)
{
	regex_t re;
	int result;
	int errcode;
	char error_message[256];
	char *temp;

	temp = str_concat("^", regexp, "$", NULL);
	errcode = regcomp(&re, temp, REG_EXTENDED);
	if (errcode != 0) {
		regerror(errcode, &re, error_message, sizeof(error_message));
		fprintf(stderr, "regcomp: %s\n", error_message);
		fprintf(stderr, _("`%s' is invalid regular expression.\n"), temp);
		fatal_error(host);
	}

	result = regexec(&re, string, 0, NULL, 0);

	regfree(&re);
	free(temp);

	if (result == 0) {
		return (TRUE);
	} else {
		return (FALSE);
	}
}
