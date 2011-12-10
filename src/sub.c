/***************************************************************************/
/*                                                                         */
/* Fast Webpage Exchanger - an FTP client for updating webpages            */
/* Copyright (C) 1999-2000 Yuuki NINOMIYA <gm@debian.or.jp>                */
/*                                                                         */
/* This program is free software; you can redistribute it and/or modify    */
/* it under the terms of the GNU General Public License as published by    */
/* the Free Software Foundation; either version 2, or (at your option)     */
/* any later version.                                                      */
/*                                                                         */
/* This program is distributed in the hope that it will be useful,         */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           */
/* GNU General Public License for more details.                            */
/*                                                                         */
/* You should have received a copy of the GNU General Public License       */
/* along with this program; if not, write to the                           */
/* Free Software Foundation, Inc., 59 Temple Place - Suite 330,            */
/* Boston, MA 02111-1307, USA.                                             */
/*                                                                         */
/***************************************************************************/

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


/* --------------------------------------------------
 NAME       n_atol
 FUNCTION   convert string to long(the maximum bytes is n)
 INPUT      str ... converted string
            n ... the maximum length of string
 OUTPUT     result value
-------------------------------------------------- */
long n_atol(char *str,int n)
{
	char *temp;
	long result;

	temp=str_malloc(n+1);
	strncpy(temp,str,n);
	temp[n]='\0';

	result=atol(temp);

	free(temp);
	return(result);
}


/* --------------------------------------------------
 NAME       cmp_file_with_wildcard
 FUNCTION   compare raw file name with file name with wild card
 INPUT      read_name ... raw file name
            wild_name ... file name with wild card
 OUTPUT     return 0 if read_name is equal to wild_name otherwise return -1
-------------------------------------------------- */
int cmp_file_with_wildcard(char *real_name,char *wild_name)
{
	char *temp;
	regex_t re;
	int res;

	temp=cnvregexp(wild_name);

	if(regcomp(&re,temp,REG_EXTENDED)!=0){
		fprintf(stderr,_("Invalid wildcard `%s'.\n"),wild_name);
		exit(1);
	}
	res=regexec(&re,real_name,0,NULL,0);
	regfree(&re);
	free(temp);
	if(res==0){
		return(0);
	}else{
		return(-1);
	}
}


/* --------------------------------------------------
 NAME       cnvregexp
 FUNCTION   convert shell wildcards to regular expressions
 INPUT      str ... shell wildcards
 OUTPUT     pointer to converted to regular expressions
-------------------------------------------------- */
char *cnvregexp(char *str)
{
	char *cp, *pattern;
	int i;
	int flag = 0;

	pattern = str_malloc(1 + strlen(str) * 2 + 3);
	i = 0;
	pattern[i++] = '^';
	for (cp = str; *cp; cp++) {
		if (flag) {
			if (*cp == ']') flag = 0;
			pattern[i++] = *cp;
			continue;
		}
		switch (*cp) {
			case '\\':
				if (!*(cp + 1)) break;
				pattern[i++] = *(cp++);
				pattern[i++] = *cp;
				break;
			case '?':
				pattern[i++] = '.';
				break;
			case '*':
				pattern[i++] = '.';
				pattern[i++] = '*';
				break;
			case '[':
				flag = 1;
				pattern[i++] = *cp;
				break;
			case '^':
			case '$':
			case '.':
				pattern[i++] = '\\';
			default:
				pattern[i++] = *cp;
				break;
		}
	}
	pattern[i++] = '$';
	pattern[i++] = '\0';
	pattern = str_realloc(pattern, i);

	return(pattern);
}



