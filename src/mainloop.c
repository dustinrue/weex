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

/* $Id: mainloop.c,v 1.1.1.1 2002/08/30 19:24:53 gmkun Exp $ */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <string.h>
#include <stdlib.h>

#include "intl.h"
#include "ftplib.h"
#include "strlib.h"
#include "variable.h"
#include "proto.h"


static int *get_targeted_host(int argc, char *argv[], int max_hosts);
static int argv_to_host_num(const char *arg);
static Direction get_direction(int host);
static void rebuild_cache(int host);
static void catch_up(const FileData *local_file_data, int host);


/* --- PUBLIC FUNCTIONS --- */

void main_loop(int argc, char *argv[], int max_hosts)
{
	int i;
	int host;
	int *targeted_host;
	FileData local_file_data;
	FileData remote_file_data;
	Order *order;
	Direction direction;
	bool use_cache;
	char *top_dir;

	FtpInit();

	targeted_host = get_targeted_host(argc, argv, max_hosts);

	for (i = 0;; i++) {
		host = targeted_host[i];
		if (host == -1) {
			break;
		}

		message(PROCESS, cfgSectionNumberToName(host), 0, host);

		direction = get_direction(host);

		get_local_file_data(&local_file_data, host);

		if (command_line_option.rebuild_cache) { /* -r | --rebuild-cache */
			rebuild_cache(host);
			free_file_data(&local_file_data);
			continue;
		}
		if (command_line_option.catch_up) { /* -R | --catch-up */
			catch_up(&local_file_data, host);
			free_file_data(&local_file_data);
			continue;
		}

		if (direction == DOWNLOAD || !does_cache_exist(host)) {
			if (direction == DOWNLOAD) {
				printf(_("Rebuilding cache for downloading...\n"));
			} else {
				printf(_("Cache file is not found.\nCreating a new one...\n"));
			}
			if (connect_to_remote_host(host) != 0) {
				free_file_data(&local_file_data);
				continue;
			}
			get_remote_file_data(&remote_file_data, host);
			use_cache = FALSE;
		} else {
			load_cache(&remote_file_data, host);
			use_cache = TRUE;
		}

		order = compare_both_hosts_and_generate_order(&local_file_data, &remote_file_data, config.local_top_dir[host], config.remote_top_dir[host], direction, host);

		if (!does_need_update(order)) {
			disconnect_from_remote_host(host);
			if (direction == UPLOAD) {
				printf(_("The remote host doesn't need updating.\n"));
			} else {
				printf(_("The local host doesn't need updating.\n"));
			}
			if (!use_cache) {
				save_cache(&remote_file_data, host);
			}
			free_file_data(&remote_file_data);
			free_file_data(&local_file_data);
			continue;
		}

		if(command_line_option.list) {
			disconnect_from_remote_host(host);
			put_listing_of_updated_file(order, direction);
			free_file_data(&remote_file_data);
			free_file_data(&local_file_data);
			continue;
		}

		if(command_line_option.nlist) {
			disconnect_from_remote_host(host);
			put_the_number_of_updated_file(order, direction);
			free_file_data(&remote_file_data);
			free_file_data(&local_file_data);
			continue;
		}

		if (!use_cache) {
			printf("\n");
		}
		put_num_updated_file(order, direction);

		if (connect_to_remote_host(host) != 0) {
			free_file_data(&remote_file_data);
			free_file_data(&local_file_data);
			continue;
		}

		if (direction == UPLOAD) {
			top_dir = str_concat(config.remote_top_dir[host], "/", NULL);
		} else {
			top_dir = str_concat(config.local_top_dir[host], "/", NULL);
		}

		message(ENTER, top_dir, 0, host);
		execute_order(order, &remote_file_data, direction, host);
		message(LEAVE, top_dir, 0, host);

		free(top_dir);
		free_order(order);

		disconnect_from_remote_host(host);

		save_cache(&remote_file_data, host);

		free_file_data(&remote_file_data);
		free_file_data(&local_file_data);
	}
	free(targeted_host);
}




/* --- PRIVATE FUNCTIONS --- */

static int *get_targeted_host(int argc, char *argv[], int max_hosts)
{
	int i, j;
	int *targeted_host;

	if (command_line_option.all_hosts) { /* All defined host except `default' are targeted. */
		targeted_host = str_malloc(max_hosts * sizeof(int));
		for (i = 0, j = 0; i < max_hosts; i++) {
			if (strcasecmp("default", cfgSectionNumberToName(i)) != 0) { /* not `default' */
				targeted_host[j] = i;
				j++;
			}
		}
		targeted_host[j] = -1;	/* sentinel */
	} else {
		targeted_host = str_malloc(argc * sizeof(int));
		for (i = 0; i < argc - 1; i++) {
			targeted_host[i] = argv_to_host_num(argv[i + 1]);
		}
		targeted_host[i] = -1;	/* sentinel */
	}

	return (targeted_host);
}


static int argv_to_host_num(const char *arg)
{
	int host_num;

	host_num = atoi(arg) - 1;
	if (host_num == -1) {	/* specified as name */
		host_num = cfgSectionNameToNumber(arg);
		if (host_num == -1) {
			fprintf(stderr, _("Specified host (section) name `%s' is undefined.\n"), arg);
			exit(1);
		}
	} else {		/* specified as number */
		if (cfgSectionNumberToName(host_num) == NULL) {
			fprintf(stderr, _("Specified host (section) number `%s' is undefined.\n"), arg);
			exit(1);
		}
	}
	if (strcasecmp("default", cfgSectionNumberToName(host_num)) == 0) {
		fprintf(stderr, _("Cannot specify section `default'.\n"));
		exit(1);
	}

	return (host_num);
}


static Direction get_direction(int host)
{
	Direction direction;

	direction = config.default_transfer_direction[host];
	if (command_line_option.upload) {
		direction = UPLOAD;
		printf(_("Local --> Remote (upload)\n"));
	} else if (command_line_option.download) {
		direction = DOWNLOAD;
		printf(_("Local <-- Remote (download)\n"));
	}
	return (direction);
}


static void rebuild_cache(int host)
{
	FileData remote_file_data;

	printf(_("Rebuilding cache...\n"));
	if (connect_to_remote_host(host) != 0) {
		return;
	}
	get_remote_file_data(&remote_file_data, host);
	disconnect_from_remote_host(host);
	save_cache(&remote_file_data, host);
	free_file_data(&remote_file_data);
}


static void catch_up(const FileData *local_file_data, int host)
{
	int i, j;
	FileData remote_file_data;

	printf(_("Rebuilding cache as all files are updated...\n"));

	remote_file_data.path_state = str_malloc(sizeof(PathState) * (local_file_data->num_paths));
	remote_file_data.num_paths = local_file_data->num_paths;

	for (i = 0; i < local_file_data->num_paths; i++) {
		remote_file_data.path_state[i].path = str_concat(config.remote_top_dir[host], local_file_data->path_state[i].path + strlen(config.local_top_dir[host]), NULL);
		remote_file_data.path_state[i].isremoved = local_file_data->path_state[i].isremoved;
		remote_file_data.path_state[i].num_files = local_file_data->path_state[i].num_files;
		remote_file_data.path_state[i].file_state = str_malloc(sizeof(FileState) * (local_file_data->path_state[i].num_files));
		for (j = 0; j < local_file_data->path_state[i].num_files; j++) {
			remote_file_data.path_state[i].file_state[j].time = local_file_data->path_state[i].file_state[j].time;
			remote_file_data.path_state[i].file_state[j].size = local_file_data->path_state[i].file_state[j].size;
			remote_file_data.path_state[i].file_state[j].mode = local_file_data->path_state[i].file_state[j].mode;
			remote_file_data.path_state[i].file_state[j].isdir = local_file_data->path_state[i].file_state[j].isdir;
			remote_file_data.path_state[i].file_state[j].isremoved = local_file_data->path_state[i].file_state[j].isremoved;
			remote_file_data.path_state[i].file_state[j].name = str_dup(local_file_data->path_state[i].file_state[j].name);
		}
	}

	save_cache(&remote_file_data, host);
	free_file_data(&remote_file_data);
}
