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

#include <stdio.h>
#include <stdarg.h>

#include "intl.h"
#include "variable.h"
#include "proto.h"

static char *mes_color[]={
	"\x1b[33m",	/* CONNECT     : yellow  */
	"\x1b[33m",	/* DISCONNECT  : yellow  */
	"\x1b[36m",	/* ENTER       : cyan    */
	"\x1b[36m",	/* LEAVE       : cyan    */
	"\x1b[32m",	/* ASCII_SEND  : green   */
	"\x1b[35m",	/* BINARY_SEND : magenta */
	"\x1b[33m",	/* CHMOD       : yellow  */
	"\x1b[31m",	/* REMOVE      : red     */
	"\x1b[34m",	/* MKDIR       : blue    */
	"\x1b[32m",	/* ASCII_LOWER_SEND  : green   */
	"\x1b[35m",	/* BINARY_LOWER_SEND : magenta */
	"\x1b[0m",	/* PROCESS     : default */
};

static char *process_mes[]={
	N_("Connect    : "),	/* CONNECT     */
	N_("Disconnect : "),	/* DISCONNECT  */
	N_("Entering   : "),	/* ENTER       */
	N_("Leaving    : "),	/* LEAVE       */
	N_("Sending    : "),	/* ASCII_SEND  */
	N_("Sending    : "),	/* BINARY_SEND */
	N_("Changing   : "),	/* CHMOD       */
	N_("Removing   : "),	/* REMOVE      */
	N_("Making Dir : "),	/* MKDIR       */
	N_("Sending    : "),	/* ASCII_LOWER_SEND  */
	N_("Sending    : "),	/* BINARY_LOWER_SEND */
	N_("Processing : "),	/* PROCESS     */
};


/* --------------------------------------------------
 NAME       put_mes
 FUNCTION   output processing message
 INPUT      mes ... type of the message
            nest ... nest level of directory
            `...' ... output strings
 OUTPUT     none
-------------------------------------------------- */
void put_mes(Message mes,int nest, ...)
{
	int mono_flag;
	va_list ap;
	int i,j;
	char *temp;

	if(cfg_silent[host_number] || opt_silent){
		return;
	}

	mono_flag=(cfg_monochrome[host_number] || opt_monochrome);

	printf("%s%s",_(process_mes[mes]),mono_flag ? "" : mes_color[mes]);
	for(i=0;i<nest;i++){
		for(j=0;j<nest_spaces[host_number];j++){
			printf(" ");
		}
	}
	va_start(ap,nest);
	if(mes==ASCII_LOWER_SEND || mes==BINARY_LOWER_SEND){
		printf(va_arg(ap,char *));
		printf("%s",mono_flag ? "" : "\x1b[0m");
		printf(" -> ");
		printf("%s",mono_flag ? "" : mes_color[mes]);
		printf(va_arg(ap,char *));
	}else{
		while((temp=va_arg(ap,char *))!=NULL){
			printf("%s",temp);
		}
	}
	va_end(ap);
	printf("%s\n",mono_flag ? "" : "\x1b[0m");
}

