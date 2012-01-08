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

/* $Id: cache.c,v 1.1.1.1 2002/08/30 19:24:52 gmkun Exp $ */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "intl.h"
#include "strlib.h"
#include "variable.h"
#include "proto.h"

static void write_cache(FileData *file_data, FILE *fp, int host);
static int read_cache(FileData *file_data, FILE *fp, int host);
static int get_num_removed_paths(FileData *file_data);
static int get_num_removed_files(PathState *path_state);


/* --- PUBLIC FUNCTIONS --- */

bool does_cache_exist(int host)
{
	if (access(config.cache_file[host], R_OK | W_OK) == 0) {
		return (TRUE);
	}
	return (FALSE);
}


void load_cache(FileData *file_data, int host)
{
	FILE *fp;
	int result;

	fp = fopen(config.cache_file[host], "r");
	if (fp == NULL) {
		perror("fopen");
		fprintf(stderr, _("Cannot open file `%s' for reading.\n"), config.cache_file[host]);
		exit(1);
	}

	result = read_cache(file_data, fp, host);
	if (result > 0) {
		fprintf(stderr, _("Cache file `%s' is broken at line %d.\n"), config.cache_file[host], result);
		exit(1);
	}

	if (fclose(fp) != 0) {
		perror("fclose");
		fprintf(stderr, _("Cannot close file `%s'.\n"), config.cache_file[host]);
		exit(1);
	}
}


void save_cache(FileData *file_data, int host)
{
	FILE *fp;

	fp = fopen(config.cache_file[host], "w");
	if (fp == NULL) {
		perror("fopen");
		fprintf(stderr, _("Cannot open file `%s' for writing.\n"), config.cache_file[host]);
		exit(1);
	}

	write_cache(file_data, fp, host);

	if (fclose(fp) != 0) {
		perror("fclose");
		fprintf(stderr, _("Cannot close file `%s'.\n"), config.cache_file[host]);
		exit(1);
	}
}


void add_file_to_cache(Order *order, FileData *remote_file_data, int host)
{
	int path_num;
	int file_num;

	path_num = search_path_num(remote_file_data->path_state, remote_file_data->num_paths, order->dest_dir);
	if (path_num < 0) {
		internal_error(__FILE__, __LINE__);
	}
	file_num = search_file_num(remote_file_data->path_state, path_num, order->file);
	if (file_num < 0) {	/* new file */
		remote_file_data->path_state[path_num].file_state = str_realloc(remote_file_data->path_state[path_num].file_state, (remote_file_data->path_state[path_num].num_files + 1) * sizeof(FileState));
		file_num = remote_file_data->path_state[path_num].num_files;
		remote_file_data->path_state[path_num].num_files++;
	}
	remote_file_data->path_state[path_num].file_state[file_num].name = str_dup(order->file);
	remote_file_data->path_state[path_num].file_state[file_num].time = order->time;
	remote_file_data->path_state[path_num].file_state[file_num].size = order->size;
	remote_file_data->path_state[path_num].file_state[file_num].mode = order->mode;
	remote_file_data->path_state[path_num].file_state[file_num].isdir = order->isdir;
	remote_file_data->path_state[path_num].file_state[file_num].isremoved = FALSE;
}


void mkdir_cache(Order *order, FileData *remote_file_data, int host)
{
	int path_num;

	remote_file_data->path_state = str_realloc(remote_file_data->path_state, (remote_file_data->num_paths + 1) * sizeof(PathState));
	path_num = remote_file_data->num_paths;
	remote_file_data->num_paths++;

	remote_file_data->path_state[path_num].path = str_concat(order->dest_dir, "/", order->file, NULL);
	remote_file_data->path_state[path_num].isremoved = FALSE;
	remote_file_data->path_state[path_num].num_files = 0;
	remote_file_data->path_state[path_num].file_state = NULL;

	add_file_to_cache(order, remote_file_data, host);
}


void remove_cache(Order *order, FileData *remote_file_data, int host)
{
	int path_num;
	int file_num;

	path_num = search_path_num(remote_file_data->path_state, remote_file_data->num_paths, order->dest_dir);
	if (path_num < 0) {
		internal_error(__FILE__, __LINE__);
	}
	file_num = search_file_num(remote_file_data->path_state, path_num, order->file);
	if (file_num < 0) {
		internal_error(__FILE__, __LINE__);
	}

	remote_file_data->path_state[path_num].file_state[file_num].isremoved = TRUE;
}


void rmdir_cache(Order *order, FileData *remote_file_data, int host)
{
	int path_num;
	int own_path_num;
	int file_num;
	char *path_file;

	path_num = search_path_num(remote_file_data->path_state, remote_file_data->num_paths, order->dest_dir);

	path_file = str_concat(order->dest_dir, "/", order->file, NULL);
	own_path_num = search_path_num(remote_file_data->path_state, remote_file_data->num_paths, path_file);
	free(path_file);

	if (path_num < 0 || own_path_num < 0) {
		internal_error(__FILE__, __LINE__);
	}
	file_num = search_file_num(remote_file_data->path_state, path_num, order->file);
	if (file_num < 0) {
		internal_error(__FILE__, __LINE__);
	}

	remote_file_data->path_state[path_num].file_state[file_num].isremoved = TRUE;
	remote_file_data->path_state[own_path_num].isremoved = TRUE;
}


/* --- PRIVATE FUNCTIONS --- */

static void write_cache(FileData *file_data, FILE *fp, int host)
{
	int i, j;

	fprintf(fp, "%d\n", file_data->num_paths - get_num_removed_paths(file_data));

	sort_path_state(file_data->path_state, file_data->num_paths);
	for (i = 0; i < file_data->num_paths; i++) {
		if (file_data->path_state[i].isremoved) {
			continue;
		}
		fprintf(fp, "PATH %10d %s\n", file_data->path_state[i].num_files - get_num_removed_files(file_data->path_state + i), file_data->path_state[i].path);
		sort_file_state(file_data->path_state[i].file_state, file_data->path_state[i].num_files);
		for (j = 0; j < file_data->path_state[i].num_files; j++) {
			if (file_data->path_state[i].file_state[j].isremoved) {
				continue;
			}
			fprintf(fp, "FILE %10ld %10ld %3o %d %s\n",
			       file_data->path_state[i].file_state[j].time,
			       file_data->path_state[i].file_state[j].size,
			       file_data->path_state[i].file_state[j].mode,
			       file_data->path_state[i].file_state[j].isdir,
			       file_data->path_state[i].file_state[j].name);
		}
	}
}


static int read_cache(FileData *file_data, FILE *fp, int host)
{
	char *buf;
	int line;
	int path_num, file_num;

	buf = str_fgets(fp);
	if (buf == NULL) {
		return (1);
	}

	file_data->num_paths = atoi(buf);
	free(buf);
	if (file_data->num_paths <= 0) {
		file_data->num_paths = 0;
		file_data->path_state = NULL;
		return (0);
	}

	file_data->path_state = str_malloc(file_data->num_paths * sizeof(PathState));
	path_num = -1;

	for (line = 2;; line++) {
		buf = str_fgets(fp);
		if (buf == NULL) {
			return (0);
		}
		if (strncmp(buf, "PATH", 4) == 0) {
			path_num++;
			file_data->path_state[path_num].num_files = atoi(buf + 5);
			file_data->path_state[path_num].path = str_dup(buf + 16);
			file_data->path_state[path_num].isremoved = FALSE;
			file_data->path_state[path_num].file_state = str_malloc(file_data->path_state[path_num].num_files * sizeof(FileState));
			file_num = -1;
		} else if (strncmp(buf, "FILE", 4) == 0) {
			file_num++;
			if (sscanf(buf + 5, "%10ld %10ld %3o %d\n",
			       &file_data->path_state[path_num].file_state[file_num].time,
			       &file_data->path_state[path_num].file_state[file_num].size,
			       &file_data->path_state[path_num].file_state[file_num].mode,
		               (int *)(&file_data->path_state[path_num].file_state[file_num].isdir)) < 4) {
				return (line);
			}
			file_data->path_state[path_num].file_state[file_num].isremoved = FALSE;
			file_data->path_state[path_num].file_state[file_num].name = str_dup(buf + 33);
		} else {
			return (line);
		}
	}
}


static int get_num_removed_paths(FileData *file_data)
{
	int i;
	int n = 0;

	for (i = 0; i < file_data->num_paths; i++) {
		if (file_data->path_state[i].isremoved) {
			n++;
		}
	}
	return (n);
}


static int get_num_removed_files(PathState *path_state)
{
	int i;
	int n = 0;

	for (i = 0; i < path_state->num_files; i++) {
		if (path_state->file_state[i].isremoved) {
			n++;
		}
	}
	return (n);
}
