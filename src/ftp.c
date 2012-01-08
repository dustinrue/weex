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

/* $Id: ftp.c,v 1.1.1.1 2002/08/30 19:24:53 gmkun Exp $ */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "intl.h"
#include "ftplib.h"
#include "strlib.h"
#include "variable.h"
#include "proto.h"


static netbuf *ftp_buf;


/* --- PUBLIC FUNCTIONS --- */

int ftp_connect(int host)
{
	if (!FtpConnect(config.host_name[host], &ftp_buf)) {
		fprintf(stderr, _("Cannot connect host `%s'.\n"), config.host_name[host]);
		return (-1);
	}
	FtpOptions(FTPLIB_CONNMODE, (config.ftp_passive[host] ? FTPLIB_PASSIVE : FTPLIB_PORT), ftp_buf);
	return (0);
}


int ftp_login(int host)
{
	if (config.login_name[host] == NULL) {
		ftp_disconnect(host);
		fprintf(stderr, _("The login name is not configured.\n"));
		return (-1);
	}
	if (config.password[host] == NULL) {
		fprintf(stderr, _("No password specified in configuration file for `%s'. Asking ...\n"), config.host_name[host]);
		config.password[host] = str_dup(getpass(_("Password: ")));
	}

	if (!FtpLogin(config.login_name[host], config.password[host], ftp_buf)) {
		ftp_disconnect(host);
		fprintf(stderr, _("Login failed. Check your ID and password.\n"));
		return (-1);
	}
	return (0);
}


void ftp_disconnect(int host)
{
	FtpQuit(ftp_buf);
}


void ftp_get_file_list(const char *outfile, const char *path, int host)
{
	char *s;

	s = str_dup_printf("%s%s", (config.show_hidden_files[host] ? "-a " : ""), (path[0] == '\0') ? "/" : path);

	if (FtpDir(outfile, s, host, ftp_buf) != 1) {
		fprintf(stderr, _("Cannot get a listing of the contents of remote directory `%s'.\n"), path);
		fatal_error(host);
	}

	free(s);
}


void ftp_extract_file_status(char *text, char **name, off_t *size, mode_t *mode, int *type, int host)
{
	char *ptr;
	char *link_temp;
	char *text_bak;
	int i;
	off_t size_buf = 0;

	text_bak = str_dup(text);

	ptr = strtok(text, " ");
	if (ptr == NULL) {
		fprintf(stderr, _("Cannot parse a listing of the contents of a remote directory:\n%s\n"), text_bak);
		fatal_error(host);
	}
	if (isdigit(ptr[0]) && isdigit(ptr[1]) && ptr[2] == '-') { /* Remote system type is Windows_NT */
		for (i = 0; i < 2; i++) {
			if ((ptr = strtok(NULL, " ")) == NULL) {
				fprintf(stderr, _("Cannot parse a listing of the contents of a remote directory:\n%s\n"), text_bak);
				fatal_error(host);
			}
		}
		if (strcmp(ptr, "<DIR>") == 0) {
			*type = 'd';
			*size = 0;
			*mode = 0755;
		} else {
			*type = '-';
			*size = atol(ptr);
			*mode = 0644;
		}
	} else {		/* Remote system type is UNIX or MACOS */
		*type = text[0];
		*mode =  (text[1] == 'r') * 0400;
		*mode += (text[2] == 'w') * 0200;
		*mode += ((text[3] == 'x') || (text[3] == 's') || (text[3] == 't')) * 0100;
		*mode += (text[4] == 'r') * 0040;
		*mode += (text[5] == 'w') * 0020;
		*mode += ((text[6] == 'x') || (text[6] == 's') || (text[6] == 't')) * 0010;
		*mode += (text[7] == 'r') * 0004;
		*mode += (text[8] == 'w') * 0002;
		*mode += ((text[9] == 'x') || (text[9] == 's') || (text[9] == 't')) * 0001;

		for (i = 0; i < 7; i++) {
			if ((ptr = strtok(NULL, " ")) == NULL) {
				fprintf(stderr, _("Cannot parse a listing of the contents of a remote directory:\n%s\n"), text_bak);
				fatal_error(host);
			}
			switch (i) {
			case 0:
				size_buf = atol(ptr);
				if (strcmp(ptr, "folder") == 0) { /* MACOS (directory) */
					*size = 0;
					i++;
				}
				break;
			case 1:
				if (strcmp(ptr, "0") == 0) { /* MACOS (file) */
					*size = size_buf;
				} else if (strcmp(ptr, "0000/0000") == 0) { /* geocities */
					i++;
				}
				break;
			case 2:
				size_buf = atol(ptr);
				break;
			case 3:
				if (isupper(ptr[0]) && islower(ptr[1]) && islower(ptr[2]) && ptr[3] == '\0') { /* FTP server used by www.enjoy.ne.jp */
					*size = size_buf;
					i++;
				} else {
					*size = atol(ptr); /* generic FTP server on UNIX */
				}
				break;
			default:
				break; /* ignore */
			}
		}
	}
	ptr = ptr + strlen(ptr) + 1;

	if ((link_temp = strstr(ptr, " -> ")) != NULL) { /* This file is a symbolic link. */
		*link_temp = '\0';
	}

	/* Strip leading spaces from the filename which can occur
	   with an FTP server based on Windows NT */
	while (*ptr == ' ') {
		ptr++;
	}

	*name = str_dup(ptr);
	free(text_bak);
}


int ftp_chdir(const char *path)
{
	return (FtpChdir(path, ftp_buf));
}


time_t ftp_get_last_modification_time(const char *path_file, int host)
{
	char mdtm_temp[20];
	struct tm time;
	char year[5], month[3], mday[3], hour[3], min[3], sec[3];
	time_t modtime;

	if (FtpModDate(path_file, mdtm_temp, 20, ftp_buf) == 1){
		if (sscanf(mdtm_temp, "%4s%2s%2s%2s%2s%2s", year, month, mday, hour, min, sec) < 6) {
			fprintf(stderr, _("Cannot parse last modification time: %s.\n"), mdtm_temp);
			minor_error(host);
			return (0);
		}

		time.tm_year = atoi(year) - 1900;
		time.tm_mon = atoi(month) - 1;
		time.tm_mday = atoi(mday);
		
		time.tm_hour = atoi(hour);
		time.tm_min = atoi(min);
		time.tm_sec = atoi(sec);

		time.tm_isdst = -1;
		modtime = mktime(&time);

		if (modtime == -1) {
			fprintf(stderr, _("`%s' is wrong time.\n"), mdtm_temp);
			minor_error(host);
			return (0);
		}
		return (modtime);
	} else {
		fprintf(stderr, _("Cannot get last modification time of `%s'.\n"), path_file);
		minor_error(host);
		return (0);
	}
	
}


/* prototype */

int ftp_send_file(const char *src, const char *dest, int trans_mode, int host)
{
	if (FtpPut(src, dest, trans_mode, host, ftp_buf) == 0) {
		return (-1);
	}
	return (0);
}


int ftp_receive_file(const char *src, const char *dest, int trans_mode, int host)
{
	if (FtpGet(dest, src, trans_mode, host, ftp_buf) == 0) {
		return (-1);
	}
	return (0);
}


int ftp_remove_file(const char *dest, int host)
{
	if(FtpDelete(dest,ftp_buf)==0){
		return (-1);
	}
	return (0);
}


int ftp_make_directory(const char *dest, int host)
{
	if(FtpMkdir(dest,ftp_buf)==0){
		return (-1);
	}
	return (0);
}


int ftp_remove_directory(const char *dest, int host)
{
	if(FtpRmdir(dest,ftp_buf)==0){
		return (-1);
	}
	return (0);
}


void ftp_change_mode_if_necessary(const char *dest_dir, const char *dest_file, mode_t mode, int nest, int host)
{
	char *full_path_name;
	char mode_temp[20];

	full_path_name = str_concat(dest_dir, "/", dest_file, NULL);
	sprintf(mode_temp, "%3o", mode);

	if (is_match_dir(dest_dir, config.preserve_permission_local_dir[host], host) ||
	    is_match_file(full_path_name, dest_dir, config.preserve_permission_local_file[host], host) ||
	    is_match_file(full_path_name, dest_dir, config.preserve_permission_file[host], host)) {
		message(CHMOD, dest_file, nest, host);
		FtpChmod(full_path_name, mode_temp, ftp_buf); /* TODO: error handling */
	}
}


/* --- PRIVATE FUNCTIONS --- */
