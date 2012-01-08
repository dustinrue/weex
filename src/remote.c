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

/* $Id: remote.c,v 1.1.1.1 2002/08/30 19:24:53 gmkun Exp $ */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <fnmatch.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#include "intl.h"
#include "strlib.h"
#include "ftplib.h"
#include "variable.h"
#include "proto.h"


static char **temp_file_name = NULL;
static int max_temp_file = 0;
static bool isconnected = FALSE;


static void scan_remote_file(FileData *file_data, const char *path_dir, int nest, int host);
static void parse_file_list(FileData *file_data, int num_paths, const char *lsfile, const char *path_dir, AccessMethod method, int host);
static char *make_temp_file_name(int host);
static void remove_temp_files(int host);


/* --- PUBLIC FUNCTIONS --- */

int connect_to_remote_host(int host)
{
	if (isconnected) {
		return (0);
	}

	message(CONNECT, config.host_name[host], 0, host);

	if (config.host_name[host] == NULL) {
		fprintf(stderr, _("The host name is not configured.\n"));
		return (-1);
	}

	if (config.access_method[host] == FTP) {
		if (ftp_connect(host) == -1 || ftp_login(host) == -1) {
			return (-1);
		}
	}
	isconnected = TRUE;
	return (0);
}


void disconnect_from_remote_host(int host)
{
	if (!isconnected) {
		return;
	}

	if (config.access_method[host] == FTP) {
		ftp_disconnect(host);
	}
	remove_temp_files(host);
	isconnected = FALSE;

	message(DISCONNECT, config.host_name[host], 0, host);
}


void get_remote_file_data(FileData *file_data, int host)
{
	file_data->num_paths = 0;
	file_data->path_state = NULL;

	scan_remote_file(file_data, config.remote_top_dir[host], 0, host);

	sort_path_state(file_data->path_state, file_data->num_paths);
#if 0
	output_file_data(file_data, 0);
#endif
}


/* --- PRIVATE FUNCTIONS --- */

static void scan_remote_file(FileData *file_data, const char *path_dir, int nest, int host)
{
	int num_paths;
	char *outfile;
	char *path_file;
	char *temp;
	int i;

	temp = str_concat(path_dir, "/", NULL);
	message(ENTER, temp, nest, host);

	num_paths = file_data->num_paths;
	file_data->num_paths++;
	
	file_data->path_state = str_realloc(file_data->path_state, sizeof(PathState) * (num_paths + 1));
	file_data->path_state[num_paths].path = str_dup(path_dir);
	file_data->path_state[num_paths].isremoved = FALSE;
	file_data->path_state[num_paths].file_state = NULL;

	outfile = make_temp_file_name(host);

	if (config.access_method[host] == FTP) {
		ftp_get_file_list(outfile, path_dir, host);
	} else if (config.access_method[host] == SCP || config.access_method[host] == RSYNC) {
		ssh_get_file_list(outfile, path_dir, host);
	} else {
		internal_error(__FILE__, __LINE__);
	}

	parse_file_list(file_data, num_paths, outfile, path_dir, config.access_method[host], host);

	for (i = 0; i < file_data->path_state[num_paths].num_files; i++) {
		if (file_data->path_state[num_paths].file_state[i].isdir) {
			path_file = str_concat(path_dir, "/", file_data->path_state[num_paths].file_state[i].name, NULL);
			nest++;
			scan_remote_file(file_data, path_file, nest, host);
			nest--;
			free(path_file);
		}
	}

	message(LEAVE, temp, nest, host);
	free(temp);
}


static void parse_file_list(FileData *file_data, int num_paths, const char *lsfile, const char *path_dir, AccessMethod method, int host)
{
	FileState *file_state = NULL;
	FILE *fp;
	char *path_file;
	char *temp;
	char *text;

	char *name;
	time_t time;
	off_t size;
	mode_t mode;
	int type;

	char *ssh_command;
	int num_files = 0;

	fp = fopen(lsfile, "r");
	if (fp == NULL) {
		perror("fopen");
		fprintf(stderr, _("Cannot open file `%s'.\n"), lsfile);
		fatal_error(host);
	}

	while ((text = str_fgets(fp)) != NULL) {
		if (strncmp(text, "total", 5) == 0) { /* skip */
			free(text);
			continue;
		}
		if (strncmp(text, "/bin/ls:", 8) == 0) {
			fprintf(stderr, _("Listing remote directory contents failed: %s\nThe directory doesn't exist or permission denied.\n"), path_dir);
			fatal_error(host);
		}

		switch (method) {
		case FTP:
			ftp_extract_file_status(text, &name, &size, &mode, &type, host);
			break;
		case SCP:
		case RSYNC:
			ssh_extract_file_status(text, &name, &time, &size, &mode, &type, host);
			break;
		default:
			internal_error(__FILE__, __LINE__);
		} 
		free(text);

		if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
			free(name);
			continue;
		}

		path_file = str_concat(path_dir, "/", name, NULL);

		if (type == 'l') { /* Check whether the symbolic link is directory or not. */
			switch (method) {
			case FTP:
				type = ftp_chdir(path_file) ? 'd' : '-';
				break;
			case SCP:
			case RSYNC:
				ssh_command = str_dup_printf("ssh %s@%s cd %s", config.login_name[host], config.host_name[host], path_file);
				type = (system(ssh_command) == 0) ? 'd' : '-';
				free(ssh_command);
				break;
			default:
				internal_error(__FILE__, __LINE__);
			}
		}

		if ((type == 'd' && is_match_dir(path_file, config.ignore_remote_dir[host], host)) ||
		    (type != 'd' && (is_match_file(path_file, path_dir, config.ignore_remote_file[host], host) ||
				     is_match_file(path_file, path_dir, config.ignore_file[host], host)))) {
			free(path_file);
			free(name);
			continue;
		}

		if (config.conv_to_lower[host]) {
			temp = str_tolower(name);
			free(name);
			name = temp;

			free(path_file);
			path_file = str_concat(path_dir, "/", name, NULL);
		}

		if (method == FTP) {
			if (type != 'd') {
				time = ftp_get_last_modification_time(path_file, host);
			} else {
				time = 0;
			}
		}

                file_state = str_realloc(file_state, (num_files + 1) * sizeof(FileState));
		file_state[num_files].name = name;
		file_state[num_files].time = time;
		file_state[num_files].size = size;
		file_state[num_files].mode = mode;
		file_state[num_files].isdir = (type == 'd');
		file_state[num_files].isremoved = FALSE;

		num_files++;

		free(path_file);
	}

	if (fclose(fp) != 0) {
		perror("fclose");
		fprintf(stderr, _("Cannot close file `%s'.\n"), lsfile);
		fatal_error(host);
        }

	sort_file_state(file_state, num_files);

	file_data->path_state[num_paths].num_files = num_files;
	file_data->path_state[num_paths].file_state = file_state;
}


static char *make_temp_file_name(int host)
{
	char *template;
	int fd;

	if (getenv("TMPDIR") != NULL) {
		template = str_dup_printf("%s/weex.%d.XXXXXX", getenv("TMPDIR"), getpid());
	} else {
		template = str_dup_printf("/tmp/weex.%d.XXXXXX", getpid());
	}

	fd = mkstemp(template);
	if (fd == -1) {
		perror("mkstemp");
		fprintf(stderr, _("Cannot create a unique file name.\n"));
		fatal_error(host);
	}
	if (close(fd) == -1) {
		perror("close");
		fprintf(stderr, _("Cannot close temporary file `%s'.\n"), template);
		fatal_error(host);
	}

	temp_file_name = str_realloc(temp_file_name, sizeof(*temp_file_name) * (max_temp_file + 1));
	temp_file_name[max_temp_file] = template;
	max_temp_file++;

	return (template);
}


static void remove_temp_files(int host)
{
	int i;

	for (i = 0; i < max_temp_file; i++) {
		if (unlink(temp_file_name[i]) == -1){
			perror("unlink");
			fprintf(stderr, _("Cannot remove temporary file `%s'.\n"), temp_file_name[i]);
			exit(1);
		}
		free(temp_file_name[i]);
	}
	free(temp_file_name);

	temp_file_name = NULL;
	max_temp_file = 0;
}
