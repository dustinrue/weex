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

/* $Id: scprsync.c,v 1.1.1.1 2002/08/30 19:24:53 gmkun Exp $ */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "intl.h"
#include "strlib.h"
#include "variable.h"
#include "proto.h"


static void exec_external_command(const char *ext_command, int host);


/* --- PUBLIC FUNCTIONS --- */

void ssh_get_file_list(const char *outfile, const char *path, int host)
{
	char *command_line;

	command_line = str_dup_printf("ssh %s@%s ls -l%s --full-time %s > %s", config.login_name[host], config.host_name[host], (config.show_hidden_files[host] ? "a" : ""), path, outfile);
	exec_external_command(command_line, host);
	free(command_line);
}


void ssh_extract_file_status(char *text, char **name, time_t *modtime, off_t *size, mode_t *mode, int *type, int host)
{
	char *ptr;
	char *link_temp;
	int i, j;
	char *month[]={ "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	struct tm time;

	ptr = strtok(text, " ");
	if (ptr == NULL) {
		fprintf(stderr, _("Cannot parse a listing of the contents of a remote directory:\n%s\n"), text);
		fatal_error(host);
	}

	*type = text[0];

	for (i = 0; i < 9; i++) {
		if ((ptr = strtok(NULL, " ")) == NULL) {
			fprintf(stderr, _("Cannot parse a listing of the contents of a remote directory:\n%s\n"), text);
			fatal_error(host);
		}
		switch (i) {
		case 3:
			*size = atol(ptr);
			break;
		case 5:
			for (j = 0; j < 12; j++) {
				if (strcmp(ptr, month[j]) == 0) {
					time.tm_mon = j;
					break;
				}
			}
			if (j == 12) {
				fprintf(stderr, _("Unknown month name: %s"), ptr);
				minor_error(host);
				time.tm_mon = 0;
			}
			break;
		case 6:
			time.tm_mday = atoi(ptr);
			break;
		case 7:
			time.tm_hour = atoi(ptr);
			time.tm_min = atoi(ptr + 3);
			time.tm_sec = atoi(ptr + 6);
			break;
		case 8:
			time.tm_year = atoi(ptr) - 1900;
			break;
		default:
			break;	/* ignore */
		}
	}
	ptr = ptr + strlen(ptr) + 1;

	if ((link_temp = strstr(ptr, " -> ")) != NULL) { /* This file is a symbolic link. */
		*link_temp = '\0';
	}

	*name = str_dup(ptr);
	*modtime = mktime(&time);
}


int scp_send_file(const char *src, const char *dest, int host)
{
	return (0);
}


int scp_receive_file(const char *src, const char *dest, int host)
{
	return (0);
}


int rsync_send_file(const char *src, const char *dest, int host)
{
	return (0);
}


int rsync_receive_file(const char *src, const char *dest, int host)
{
	return (0);
}


void ssh_change_mode_if_necessary(const char *dest_dir, const char *dest_file, mode_t mode, int nest, int host)
{
}


/* --- PRIVATE FUNCTIONS --- */

static void exec_external_command(const char *ext_command, int host)
{
	if (system(ext_command) != 0) {
		fprintf(stderr, "Executing external command failed: %s\n", ext_command);
		fatal_error(host);
	}
}
