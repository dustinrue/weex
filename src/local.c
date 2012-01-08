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

/* $Id: local.c,v 1.1.1.1 2002/08/30 19:24:53 gmkun Exp $ */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>

#include "intl.h"
#include "strlib.h"
#include "variable.h"
#include "proto.h"


static void scan_local_file(FileData *file_data, const char *path, int host);
static int expand_symlink(struct stat *file_stat, char *path_file);
static void check_conflict_with_conv_to_lower(const FileState *file_state, int num_files);


/* --- PUBLIC FUNCTIONS --- */

void get_local_file_data(FileData *file_data, int host)
{
	file_data->num_paths = 0;
	file_data->path_state = NULL;

	scan_local_file(file_data, config.local_top_dir[host], host);

#if 0
	output_file_data(file_data, 0);
#endif
}


void local_change_mode_if_necessary(const char *dest_dir, const char *dest_file, mode_t mode, int nest, int host)
{
}


/* --- PRIVATE FUNCTIONS --- */

static void scan_local_file(FileData *file_data, const char *path_dir, int host)
{
	FileState *file_state = NULL;
	DIR *dir;
	struct dirent *ent;
	struct stat status;
	struct tm *time;
	bool isdir;
	int num_paths;
	int num_files;
	char *path_file;

	dir = opendir(path_dir);
	if (dir == NULL) {
		perror("opendir");
		fprintf(stderr, _("Cannot open directory `%s'.\n"), path_dir);
		exit(1);
	}

	num_paths = file_data->num_paths;
	file_data->num_paths++;

	file_data->path_state = str_realloc(file_data->path_state, sizeof(PathState) * (num_paths + 1));
	file_data->path_state[num_paths].path = str_dup(path_dir);
	file_data->path_state[num_paths].isremoved = FALSE;
	file_data->path_state[num_paths].file_state = NULL;
	num_files = 0;

	while ((ent = readdir(dir)) != NULL) {
		if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
			continue;
		}

		path_file = str_concat(path_dir, "/", ent->d_name, NULL);

		if (lstat(path_file, &status) == -1) {
			perror("lstat");
			fprintf(stderr, _("Cannot get file status: %s\n"), path_file);
			exit(1);
		}
		if (config.follow_symlinks[host]) {
			if (expand_symlink(&status, path_file) == -1) {
				fprintf(stderr, _("Cannot expand symbolic link `%s'.\nThis file is ignored.\n"), path_file);
				free(path_file);
				continue;
			}
		}

		isdir = ((status.st_mode & S_IFDIR) == S_IFDIR);

		if ((isdir && is_match_dir(path_file, config.ignore_local_dir[host], host)) ||
		    (!isdir && (is_match_file(path_file, path_dir, config.ignore_local_file[host], host) ||
				is_match_file(path_file, path_dir, config.ignore_file[host], host)))) {
			free(path_file);
			continue;
		}

		file_state = str_realloc(file_state, (num_files + 1) * sizeof(FileState));
		file_state[num_files].name = str_dup(ent->d_name);
		time = gmtime(&status.st_mtime);
		file_state[num_files].time = mktime(time);
		file_state[num_files].size = status.st_size;
		file_state[num_files].mode = status.st_mode & 0777;
		file_state[num_files].isdir = isdir;
		file_state[num_files].isremoved = FALSE;

		if (isdir) {
			scan_local_file(file_data, path_file, host);
		}

		num_files++;

		free(path_file);
	}

	if (closedir(dir) == -1){
		perror("closedir");
		fprintf(stderr, _("Cannot close directory `%s'.\n"), path_dir);
		exit(1);
	}

	sort_file_state(file_state, num_files);

	if (config.conv_to_lower[host]) {
		check_conflict_with_conv_to_lower(file_state, num_files);
	}

	file_data->path_state[num_paths].num_files = num_files;
	file_data->path_state[num_paths].file_state = file_state;
}


static int expand_symlink(struct stat *status, char *path_file)
{
	long path_max;

#ifdef PATH_MAX
	path_max = PATH_MAX;
#else
	path_max = pathconf(*path_dir, _PC_PATH_MAX);
	if (path_max <= 0) {
		path_max = 4096;
	}
#endif

	while ((status->st_mode & S_IFLNK) == S_IFLNK) {
		char realfname[path_max];

		if (realpath(path_file, realfname) == NULL) {
			return (-1);
		}
		if (lstat(realfname, status) == -1) {
			perror("lstat");
			fprintf(stderr, _("Cannot get file status: %s\n"), path_file);
			exit(1);
		}
	}
	return (0);
}


static void check_conflict_with_conv_to_lower(const FileState *file_state, int num_files)
{
	int i, j;

	for (i = 0; i < num_files; i++) {
		for (j = i; j < num_files; j++) {
			if (strcasecmp(file_state[i].name, file_state[j].name) == 0) {
				fprintf(stderr, _("ConvToLower causes a conflict between `%s' and `%s'. Proceed anyway.\n"), file_state[i].name, file_state[j].name);
			}
		}
	}
}
