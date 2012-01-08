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

/* $Id: filedata.c,v 1.1.1.1 2002/08/30 19:24:52 gmkun Exp $ */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#include "intl.h"
#include "strlib.h"
#include "variable.h"
#include "proto.h"


static Order *src_order = NULL;
static Order *dest_order = NULL;
static int max_src_order = 0;
static int max_dest_order = 0;


static int comp_file_name(const void *file_state1, const void *file_state2);
static int comp_isdir(const void *file_state1, const void *file_state2);
static int comp_path_name(const void *path_state1, const void *path_state2);
static int get_num_dirs(FileState *file_state, int num_files);

static void generate_order(FileData *src_file_data, FileData *dest_file_data, const char *src_dir, const char *dest_dir, Direction direction, int host);

static void compare_src_recursively(FileData *src_file_data, FileData *dest_file_data, int src_path_num, int dest_path_num, const char *src_dir, const char *dest_dir, Direction direction, int host);
static void compare_src_file(FileData *src_file_data, FileData *dest_file_data, int src_path_num, int dest_path_num, int src_file_num, const char *src_dir, const char *dest_dir, Direction direction, int host);
static void compare_src_dir(FileData *src_file_data, FileData *dest_file_data, int src_path_num, int dest_path_num, int src_file_num, const char *src_dir, const char *dest_dir, Direction direction, int host);
static void search_child_src_dir(FileData *src_file_data, FileData *dest_file_data, int src_path_num, int dest_path_num, int src_file_num, const char *src_dir, const char *dest_dir, Direction direction, int host);

static void compare_dest_recursively(FileData *src_file_data, FileData *dest_file_data, int src_path_num, int dest_path_num, const char *src_dir, const char *dest_dir, Direction direction, int host);
static void compare_dest_file(FileData *src_file_data, FileData *dest_file_data, int src_path_num, int dest_path_num, int dest_file_num, const char *src_dir, const char *dest_dir, Direction direction, int host);
static void compare_dest_dir(FileData *src_file_data, FileData *dest_file_data, int src_path_num, int dest_path_num, int dest_file_num, const char *src_dir, const char *dest_dir, Direction direction, int host);
static void search_child_dest_dir(FileData *src_file_data, FileData *dest_file_data, int src_path_num, int dest_path_num, int dest_file_num, const char *src_dir, const char *dest_dir, Direction direction, int host);

static void add_src_action(const FileState *file_state, const char *src_dir, const char *dest_dir, Action action);
static void add_dest_action(const FileState *file_state, const char *src_dir, const char *dest_dir, Action action);

static Order *optimize_order(void);

static bool is_keep_dir(const char *dir, const char *file, Direction direction, int host);
static bool is_keep_file(const char *dir, const char *file, Direction direction, int host);


/* --- PUBLIC FUNCTIONS --- */

void sort_file_state(FileState *file_state, int num_files)
{
	int num_dirs;

	num_dirs = get_num_dirs(file_state, num_files);

	/* files move to the front */
	qsort(file_state, num_files, sizeof(FileState), comp_isdir);

	/* sort files by alphabetical order */
	qsort(file_state, num_files - num_dirs, sizeof(FileState), comp_file_name);

	/* sort directories by alphabetical order */
	qsort(file_state + num_files - num_dirs, num_dirs, sizeof(FileState), comp_file_name);
}


void sort_path_state(PathState *path_state, int num_paths)
{
	/* sort by alphabetical order */
	qsort(path_state, num_paths, sizeof(PathState), comp_path_name);
}


void output_file_data(const FileData *file_data, int nest_level)
{
	int i, j;

	for (i = 0; i < file_data->num_paths; i++) {
		printf("%3d %s\n", file_data->path_state[i].num_files, file_data->path_state[i].path);
		for (j = 0; j < file_data->path_state[i].num_files; j++) {
			printf("%ld %8ld %o %d %d %s\n",
			       file_data->path_state[i].file_state[j].time,
			       file_data->path_state[i].file_state[j].size,
			       file_data->path_state[i].file_state[j].mode,
			       file_data->path_state[i].file_state[j].isdir,
			       file_data->path_state[i].file_state[j].isremoved,
			       file_data->path_state[i].file_state[j].name);
		}
	}
}


void free_file_data(FileData *file_data)
{
	int i, j;

	for (i = 0; i < file_data->num_paths; i++) {
		for (j = 0; j < file_data->path_state[i].num_files; j++) {
			free(file_data->path_state[i].file_state[j].name);
		}
		free(file_data->path_state[i].path);
		free(file_data->path_state[i].file_state);
	}
	free(file_data->path_state);
}


Order *compare_both_hosts_and_generate_order(FileData *local_file_data, FileData *remote_file_data, const char *local_top_dir, const char *remote_top_dir, Direction direction, int host)
{
	if (direction == UPLOAD) {
		generate_order(local_file_data, remote_file_data, local_top_dir, remote_top_dir, direction, host);
	} else {
		generate_order(remote_file_data, local_file_data, remote_top_dir, local_top_dir, direction, host);
	}

	add_src_action(NULL, NULL, NULL, END);
	add_dest_action(NULL, NULL, NULL, END);

	return (optimize_order());
}


void free_order(Order *order)
{
	int i;

	for (i = 0; order[i].action != END; i++) {
		free(order[i].file);
		free(order[i].src_dir);
		free(order[i].dest_dir);
	}
	free(order);
}


bool does_need_update(Order *order)
{
	int i;

	for (i = 0; order[i].action != END; i++) {
		if (order[i].action != ENTER && order[i].action != LEAVE) {
			return (TRUE);
		}
	}
	return (FALSE);
}


void put_listing_of_updated_file(Order *order, Direction direction)
{
	int i;

	if (direction == UPLOAD) {
		printf(_("The following new files need to be sent.\n"));
	} else {
		printf(_("The following new files need to be received.\n"));
	}
	for (i = 0; order[i].action != END; i++) {
		if (order[i].action == NEW) {
			printf("%s/%s\n", order[i].src_dir, order[i].file);
		}
	}

	if (direction == UPLOAD) {
		printf(_("\nThe following updated files need to be sent.\n"));
	} else {
		printf(_("\nThe following updated files need to be received.\n"));
	}
	for (i = 0; order[i].action != END; i++) {
		if (order[i].action == UPDATE) {
			printf("%s/%s\n", order[i].src_dir, order[i].file);
		}
	}

	printf(_("\nThe following files need to be removed.\n"));
	for (i = 0; order[i].action != END; i++) {
		if (order[i].action == REMOVE) {
			printf("%s/%s\n", order[i].dest_dir, order[i].file);
		}
	}

	printf(_("\nThe following directories need to be made.\n"));
	for (i = 0; order[i].action != END; i++) {
		if (order[i].action == MKDIR) {
			printf("%s/%s\n", order[i].dest_dir, order[i].file);
		}
	}

	printf(_("\nThe following directories need to be removed.\n"));
	for (i = 0; order[i].action != END; i++) {
		if (order[i].action == RMDIR) {
			printf("%s/%s\n", order[i].dest_dir, order[i].file);
		}
	}
}


void put_the_number_of_updated_file(Order *order, Direction direction)
{
	int i;
	int new_num = 0;
	int update_num = 0;
	int remove_num = 0;
	int mkdir_num = 0;
	int rmdir_num = 0;
	off_t new_size = 0;
	off_t update_size = 0;
	off_t remove_size = 0;

	for (i = 0; order[i].action != END; i++) {
		switch (order[i].action) {
		case NEW:
			new_num++;
			new_size += order[i].size;
			break;
		case UPDATE:
			update_num++;
			update_size += order[i].size;
			break;
		case REMOVE:
			remove_num++;
			remove_size += order[i].size;
			break;
		case MKDIR:
			mkdir_num++;
			break;
		case RMDIR:
			rmdir_num++;
			break;
		case ENTER:
		case LEAVE:
			break;
		default:
			internal_error(__FILE__, __LINE__);
		}
	}

	if (new_num == 0) {
		printf((direction == UPLOAD) ? _("No new files need to be sent.\n") : _("No new files need to be received.\n"));
	} else {
		printf((direction == UPLOAD) ?
		       ((new_num > 1) ? _("%d new files (%ld bytes) need to be sent.\n") :     _("%d new file (%ld bytes) needs to be sent.\n")) :
		       ((new_num > 1) ? _("%d new files (%ld bytes) need to be received.\n") : _("%d new file (%ld bytes) needs to be received.\n")), new_num, new_size);
	}

	if (update_num == 0) {
		printf((direction == UPLOAD) ? _("No updated files need to be sent.\n") : _("No updated files need to be received.\n"));
	} else {
		printf((direction == UPLOAD) ?
		       ((update_num > 1) ? _("%d updated files (%ld bytes) need to be sent.\n")     : _("%d updated file (%ld bytes) needs to be sent.\n")) :
		       ((update_num > 1) ? _("%d updated files (%ld bytes) need to be received.\n") : _("%d updated file (%ld bytes) needs to be received.\n")), update_num, update_size);
	}

	if (remove_num == 0) {
		printf(_("No files need to be removed.\n"));
	} else {
		printf((remove_num > 1) ? _("%d files (%ld bytes) need to be removed.\n") : _("%d file (%ld bytes) needs to be removed.\n"), remove_num, remove_size);
	}

	if (mkdir_num == 0) {
		printf(_("No directories need to be made.\n"));
	} else {
		printf((mkdir_num > 1) ? _("%d directories need to be made.\n") : _("%d directories needs to be made.\n"), mkdir_num);
	}

	if (rmdir_num == 0) {
		printf(_("No directories need to be removed.\n"));
	} else {
		printf((rmdir_num > 1) ? _("%d directories need to be removed.\n") : _("%d directories needs to be removed.\n"), rmdir_num);
	}
}


void put_num_updated_file(Order *order, Direction direction)
{
	int i;
	int num = 0;
	off_t size = 0;

	for (i = 0; order[i].action != END; i++) {
		if (order[i].action == NEW || order[i].action == UPDATE) {
			num++;
			size += order[i].size;
		}
	}
	if (num == 0) {
		return;
	}

	printf((direction == UPLOAD) ?
	       ((num > 1) ? _("%d files (%ld bytes) need to be sent.\n") : _("%d file (%ld bytes) needs to be sent.\n")) :
	       ((num > 1) ? _("%d files (%ld bytes) need to be received.\n") : _("%d file (%ld bytes) needs to be received.\n")), num, size);
}


/* TODO: using binary search */
int search_path_num(PathState *path_state, int num_paths, const char *path)
{
	int i;

	for (i = 0; i < num_paths; i++) {
		if (strcmp(path_state[i].path, path) == 0) {
			return (i);
		}
	}
	return (-1);
}


int search_file_num(PathState *path_state, int path_num, const char *name)
{
	int i;

	for (i = 0; i < path_state[path_num].num_files; i++) {
		if (strcmp(path_state[path_num].file_state[i].name, name) == 0) {
			return (i);
		}
	}
	return (-1);
}


/* --- PRIVATE FUNCTIONS --- */

static int comp_file_name(const void *file_state1, const void *file_state2)
{
	return (strcmp(((FileState *) file_state1)->name, ((FileState *) file_state2)->name));
}


static int comp_isdir(const void *file_state1, const void *file_state2)
{
	if (!((FileState *) file_state1)->isdir && ((FileState *) file_state2)->isdir) {
		return (-1);
	}
	if (((FileState *) file_state1)->isdir && !((FileState *) file_state2)->isdir) {
		return (1);
	}
	return (0);
}


static int comp_path_name(const void *path_state1, const void *path_state2)
{
	return (strcmp(((PathState *) path_state1)->path, ((PathState *) path_state2)->path));
}


static int get_num_dirs(FileState *file_state, int num_files)
{
	int i;
	int num_dirs = 0;

	for (i = 0; i < num_files; i++) {
		if (file_state[i].isdir) {
			num_dirs++;
		}
	}
	return (num_dirs);
}


static void generate_order(FileData *src_file_data, FileData *dest_file_data, const char *src_dir, const char *dest_dir, Direction direction, int host)
{
	int src_path_num;
	int dest_path_num;

	src_path_num = search_path_num(src_file_data->path_state, src_file_data->num_paths, src_dir);
	dest_path_num = search_path_num(dest_file_data->path_state, dest_file_data->num_paths, dest_dir);

	if (src_path_num >= 0) {
		compare_src_recursively(src_file_data, dest_file_data, src_path_num, dest_path_num, src_dir, dest_dir, direction, host);
	}
	if (dest_path_num >= 0) {
		compare_dest_recursively(src_file_data, dest_file_data, src_path_num, dest_path_num, src_dir, dest_dir, direction, host);
	}
}


static void compare_src_recursively(FileData *src_file_data, FileData *dest_file_data, int src_path_num, int dest_path_num, const char *src_dir, const char *dest_dir, Direction direction, int host)
{
	int i;

	for (i = 0; i < src_file_data->path_state[src_path_num].num_files; i++) {
		if (src_file_data->path_state[src_path_num].file_state[i].isdir) {
			compare_src_dir(src_file_data, dest_file_data, src_path_num, dest_path_num, i, src_dir, dest_dir, direction, host);
		} else {
			compare_src_file(src_file_data, dest_file_data, src_path_num, dest_path_num, i, src_dir, dest_dir, direction, host);
		}
	}
}


static void compare_src_file(FileData *src_file_data, FileData *dest_file_data, int src_path_num, int dest_path_num, int src_file_num, const char *src_dir, const char *dest_dir, Direction direction, int host)
{
	int dest_file_num;

	if (dest_path_num >= 0) {
		dest_file_num = search_file_num(dest_file_data->path_state, dest_path_num, src_file_data->path_state[src_path_num].file_state[src_file_num].name);
		if (dest_file_num >= 0) { /* same name already exists */
			if (dest_file_data->path_state[dest_path_num].file_state[dest_file_num].isdir) { /* src is file, but dest is dir, so dest dir is removed */
				search_child_dest_dir(src_file_data, dest_file_data, src_path_num, dest_path_num, dest_file_num, src_dir, dest_dir, direction, host);
				add_dest_action(dest_file_data->path_state[dest_path_num].file_state + dest_file_num, src_dir, dest_dir, RMDIR);
				add_dest_action(src_file_data->path_state[src_path_num].file_state + src_file_num, src_dir, dest_dir, NEW);
			}else if (config.force[host] || command_line_option.force ||
				  src_file_data->path_state[src_path_num].file_state[src_file_num].time > dest_file_data->path_state[dest_path_num].file_state[dest_file_num].time) { /* updated file */
				add_src_action(src_file_data->path_state[src_path_num].file_state + src_file_num, src_dir, dest_dir, UPDATE);
			}
			return;
		}
	}

	/* new file || in new directory, so all files are sent */
	add_src_action(src_file_data->path_state[src_path_num].file_state + src_file_num, src_dir, dest_dir, NEW);
}


static void compare_src_dir(FileData *src_file_data, FileData *dest_file_data, int src_path_num, int dest_path_num, int src_file_num, const char *src_dir, const char *dest_dir, Direction direction, int host)
{
	int dest_file_num;

	if (dest_path_num >= 0) {
		dest_file_num = search_file_num(dest_file_data->path_state, dest_path_num, src_file_data->path_state[src_path_num].file_state[src_file_num].name);
		if (dest_file_num >= 0) { /* same name already exists */
			if (dest_file_data->path_state[dest_path_num].file_state[dest_file_num].isdir) {
				search_child_src_dir(src_file_data, dest_file_data, src_path_num, dest_path_num, src_file_num, src_dir, dest_dir, direction, host);
			} else { /* same name exists, but it's a file. so remove it and create new directory */
				add_src_action(dest_file_data->path_state[dest_path_num].file_state + dest_file_num, src_dir, dest_dir, REMOVE);
				add_src_action(src_file_data->path_state[src_path_num].file_state + src_file_num, src_dir, dest_dir, MKDIR);
				search_child_src_dir(src_file_data, dest_file_data, src_path_num, dest_path_num, src_file_num, src_dir, dest_dir, direction, host);
			}
			return;
		}
	}

	/* new directory || in new directory, so all directories are made */
	add_src_action(src_file_data->path_state[src_path_num].file_state + src_file_num, src_dir, dest_dir, MKDIR);
	search_child_src_dir(src_file_data, dest_file_data, src_path_num, dest_path_num, src_file_num, src_dir, dest_dir, direction, host);
}


static void search_child_src_dir(FileData *src_file_data, FileData *dest_file_data, int src_path_num, int dest_path_num, int src_file_num, const char *src_dir, const char *dest_dir, Direction direction, int host)
{
	char *new_src_dir;
	char *new_dest_dir;
	int new_src_path_num;
	int new_dest_path_num;

	new_src_dir = str_concat(src_dir, "/", src_file_data->path_state[src_path_num].file_state[src_file_num].name, NULL);
	new_dest_dir = str_concat(dest_dir, "/", src_file_data->path_state[src_path_num].file_state[src_file_num].name, NULL);

	new_src_path_num = search_path_num(src_file_data->path_state, src_file_data->num_paths, new_src_dir);
	new_dest_path_num = search_path_num(dest_file_data->path_state, dest_file_data->num_paths, new_dest_dir);

	add_src_action(src_file_data->path_state[src_path_num].file_state + src_file_num, src_dir, dest_dir, ENTER);
	compare_src_recursively(src_file_data, dest_file_data, new_src_path_num, new_dest_path_num, new_src_dir, new_dest_dir, direction, host);
	add_src_action(src_file_data->path_state[src_path_num].file_state + src_file_num, src_dir, dest_dir, LEAVE);

	free(new_dest_dir);
	free(new_src_dir);
}


static void compare_dest_recursively(FileData *src_file_data, FileData *dest_file_data, int src_path_num, int dest_path_num, const char *src_dir, const char *dest_dir, Direction direction, int host)
{
	int i;

	for (i = 0; i < dest_file_data->path_state[dest_path_num].num_files; i++) {
		if (dest_file_data->path_state[dest_path_num].file_state[i].isdir) {
			compare_dest_dir(src_file_data, dest_file_data, src_path_num, dest_path_num, i, src_dir, dest_dir, direction, host);
		} else {
			compare_dest_file(src_file_data, dest_file_data, src_path_num, dest_path_num, i, src_dir, dest_dir, direction, host);
		}
	}
}


static void compare_dest_file(FileData *src_file_data, FileData *dest_file_data, int src_path_num, int dest_path_num, int dest_file_num, const char *src_dir, const char *dest_dir, Direction direction, int host)
{
	int src_file_num;

	if (src_path_num >= 0) {
		src_file_num = search_file_num(src_file_data->path_state, src_path_num, dest_file_data->path_state[dest_path_num].file_state[dest_file_num].name);
		if (src_file_num >= 0) { /* file exist (not remove) */
			return;
		}
	}

	if (is_keep_file(dest_dir, dest_file_data->path_state[dest_path_num].file_state[dest_file_num].name, direction, host)) {
		return;
	}

	/* obsolete file || in obsolete directory, so all files are removed */
	add_dest_action(dest_file_data->path_state[dest_path_num].file_state + dest_file_num, src_dir, dest_dir, REMOVE);
}


static void compare_dest_dir(FileData *src_file_data, FileData *dest_file_data, int src_path_num, int dest_path_num, int dest_file_num, const char *src_dir, const char *dest_dir, Direction direction, int host)
{
	int src_file_num;

	if (src_path_num >= 0) {
		src_file_num = search_file_num(src_file_data->path_state, src_path_num, dest_file_data->path_state[dest_path_num].file_state[dest_file_num].name);
		if (src_file_num >= 0) { /* same name already exists */
			if (src_file_data->path_state[src_path_num].file_state[src_file_num].isdir) {
				search_child_dest_dir(src_file_data, dest_file_data, src_path_num, dest_path_num, dest_file_num, src_dir, dest_dir, direction, host);
			}
			return;
		}
	}

	if (is_keep_dir(dest_dir, dest_file_data->path_state[dest_path_num].file_state[dest_file_num].name, direction, host)) {
		return;
	}

	/* obsolete directory || in obsolete directory, so all directories are removed */
	search_child_dest_dir(src_file_data, dest_file_data, src_path_num, dest_path_num, dest_file_num, src_dir, dest_dir, direction, host);
	add_dest_action(dest_file_data->path_state[dest_path_num].file_state + dest_file_num, src_dir, dest_dir, RMDIR);
}

static void search_child_dest_dir(FileData *src_file_data, FileData *dest_file_data, int src_path_num, int dest_path_num, int dest_file_num, const char *src_dir, const char *dest_dir, Direction direction, int host)
{
	char *new_src_dir;
	char *new_dest_dir;
	int new_src_path_num;
	int new_dest_path_num;

	new_src_dir = str_concat(src_dir, "/", dest_file_data->path_state[dest_path_num].file_state[dest_file_num].name, NULL);
	new_dest_dir = str_concat(dest_dir, "/", dest_file_data->path_state[dest_path_num].file_state[dest_file_num].name, NULL);

	new_src_path_num = search_path_num(src_file_data->path_state, src_file_data->num_paths, new_src_dir);
	new_dest_path_num = search_path_num(dest_file_data->path_state, dest_file_data->num_paths, new_dest_dir);

	add_dest_action(dest_file_data->path_state[dest_path_num].file_state + dest_file_num, src_dir, dest_dir, ENTER);
	compare_dest_recursively(src_file_data, dest_file_data, new_src_path_num, new_dest_path_num, new_src_dir, new_dest_dir, direction, host);
	add_dest_action(dest_file_data->path_state[dest_path_num].file_state + dest_file_num, src_dir, dest_dir, LEAVE);

	free(new_dest_dir);
	free(new_src_dir);
}


static void add_src_action(const FileState *file_state, const char *src_dir, const char *dest_dir, Action action)
{
	src_order = str_realloc(src_order, sizeof(*src_order) * (max_src_order + 1));
	src_order[max_src_order].action = action;

	if (action == END) {
		src_order[max_src_order].file = NULL;
		src_order[max_src_order].src_dir = NULL;
		src_order[max_src_order].dest_dir = NULL;
		src_order[max_src_order].time = 0;
		src_order[max_src_order].size = 0;
		src_order[max_src_order].mode = 0;
		src_order[max_src_order].isdir = FALSE;
	} else {
		src_order[max_src_order].file = str_dup(file_state->name);
		src_order[max_src_order].src_dir = str_dup(src_dir);
		src_order[max_src_order].dest_dir = str_dup(dest_dir);
		src_order[max_src_order].time = file_state->time;
		src_order[max_src_order].size = file_state->size;
		src_order[max_src_order].mode = file_state->mode;
		src_order[max_src_order].isdir = file_state->isdir;
	}
	max_src_order++;
}


static void add_dest_action(const FileState *file_state, const char *src_dir, const char *dest_dir, Action action)
{
	dest_order = str_realloc(dest_order, sizeof(*dest_order) * (max_dest_order + 1));
	dest_order[max_dest_order].action = action;

	if (action == END) {
		dest_order[max_dest_order].file = NULL;
		dest_order[max_dest_order].src_dir = NULL;
		dest_order[max_dest_order].dest_dir = NULL;
		dest_order[max_dest_order].time = 0;
		dest_order[max_dest_order].size = 0;
		dest_order[max_dest_order].mode = 0;
		dest_order[max_dest_order].isdir = FALSE;
	} else {
		dest_order[max_dest_order].file = str_dup(file_state->name);
		dest_order[max_dest_order].src_dir = str_dup(src_dir);
		dest_order[max_dest_order].dest_dir = str_dup(dest_dir);
		dest_order[max_dest_order].time = file_state->time;
		dest_order[max_dest_order].size = file_state->size;
		dest_order[max_dest_order].mode = file_state->mode;
		dest_order[max_dest_order].isdir = file_state->isdir;
	}
	max_dest_order++;
}


static Order *optimize_order(void)
{
	Order *order;
	int max = 0;
	int dest = 0;
	int i, j;

	order = str_malloc(sizeof(*order) * (max_src_order + max_dest_order + 1));

	for (i = 0; src_order[i].action != END; i++) {
		if (src_order[i].action == MKDIR){
			for (j = i; !(src_order[j].action == LEAVE && strcmp(src_order[i].dest_dir, src_order[j].dest_dir) == 0 && strcmp(src_order[i].file, src_order[j].file) == 0); j++) {
				order[max] = src_order[j];
				max++;
			}
			order[max] = src_order[j];
			max++;
			i = j;
			continue;
		}
		if (src_order[i].action == ENTER){
			while (!(dest_order[dest].action == ENTER && strcmp(src_order[i].dest_dir, dest_order[dest].dest_dir) == 0 && strcmp(src_order[i].file, dest_order[dest].file) == 0)) {
				order[max] = dest_order[dest];
				max++;
				dest++;
			}
			free(dest_order[dest].file);
			free(dest_order[dest].src_dir);
			free(dest_order[dest].dest_dir);
			dest++;
		} else if (src_order[i].action == LEAVE) {
			while (!(dest_order[dest].action == LEAVE && strcmp(src_order[i].dest_dir, dest_order[dest].dest_dir) == 0 && strcmp(src_order[i].file, dest_order[dest].file) == 0)) {
				order[max] = dest_order[dest];
				max++;
				dest++;
			}
			free(dest_order[dest].file);
			free(dest_order[dest].src_dir);
			free(dest_order[dest].dest_dir);
			dest++;
		}
		order[max] = src_order[i];
		max++;
	}
	for (i = dest; dest_order[i].action != END; i++) {
		order[max] = dest_order[i];
		max++;
	}

	free(src_order);
	free(dest_order);
	src_order = NULL;
	dest_order = NULL;
	max_src_order = 0;
	max_dest_order = 0;

	order[max].action = END;
	order[max].file = NULL;
	order[max].src_dir = NULL;
	order[max].dest_dir = NULL;

	return (order);
}


static bool is_keep_dir(const char *dir, const char *file, Direction direction, int host)
{
	char *temp;
	bool result;

	temp = str_concat(dir, "/", file, NULL);

	if (direction == UPLOAD) {
		result = is_match_dir(temp, config.keep_remote_dir[host], host);
	} else if (direction == DOWNLOAD) {
		result = is_match_dir(temp, config.keep_local_dir[host], host);
	} else {
		internal_error(__FILE__, __LINE__);
	}

	free(temp);
	return (result);
}


static bool is_keep_file(const char *dir, const char *file, Direction direction, int host)
{
	char *temp;
	bool result;

	temp = str_concat(dir, "/", file, NULL);

	if (direction == UPLOAD) {
		result = (is_match_dir(dir, config.keep_remote_dir[host], host) ||
			  is_match_file(temp, dir, config.keep_remote_file[host], host) ||
			  is_match_file(temp, dir, config.keep_file[host], host));
	} else if (direction == DOWNLOAD) {
		result = (is_match_dir(dir, config.keep_local_dir[host], host) ||
			  is_match_file(temp, dir, config.keep_local_file[host], host) ||
			  is_match_file(temp, dir, config.keep_file[host], host));
	} else {
		internal_error(__FILE__, __LINE__);
	}

	free(temp);
	return (result);
}
