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

/* $Id: variable.h,v 1.1.1.1 2002/08/30 19:24:53 gmkun Exp $ */

#ifndef VARIABLE_H_INCLUDED
#define VARIABLE_H_INCLUDED

#ifdef GLOBAL_VALUE_DEFINE
#  define GLOBAL
#  define GLOBAL_VAL(v) =(v)
#else
#  define GLOBAL extern
#  define GLOBAL_VAL(v) /* */
#endif

#include <sys/types.h>

#include "parsecfg.h"


#define DEFAULT_CONFIG_DIR ".weex"
#define DEFAULT_CONFIG_FILE "weexrc"


typedef enum {
	FALSE,
	TRUE
} bool;

typedef enum {
	UPLOAD,
	DOWNLOAD
} Direction;

typedef enum {
	FTP,
	SCP,
	RSYNC
} AccessMethod;

typedef enum {
	TIME,
	SIZE,
	TIMESIZE
} DetectingModificationMethod;

typedef enum {
	CONNECT,
	DISCONNECT,
	ENTER,
	LEAVE,
	NEW,
	UPDATE,
	SEND_NEW_IMAGE,
	SEND_NEW_ASCII,
	SEND_UPDATE_IMAGE,
	SEND_UPDATE_ASCII,
	RECEIVE_NEW_IMAGE,
	RECEIVE_NEW_ASCII,
	RECEIVE_UPDATE_IMAGE,
	RECEIVE_UPDATE_ASCII,
	CHMOD,
	REMOVE,
	MKDIR,
	RMDIR,
	PROCESS,
	END
} Action;


typedef struct {
	char *config_file;
	bool all_hosts;
	bool silent;
	bool quiet;
	bool suppress_progress;
	bool force;
	bool monochrome;
	bool rebuild_cache;
	bool catch_up;
	bool not_mkdir_weex;
	bool confirmation;
	bool simulate;
	bool list;
	bool nlist;
	bool download;
	bool upload;
	bool debug;
	bool view;
} CommandLineOption;

typedef struct {
	char **connect;
	char **disconnect;
	char **enter;
	char **leave;
	char **send_new_image;
	char **receive_new_image;
	char **send_new_ascii;
	char **receive_new_ascii;
	char **send_update_image;
	char **receive_update_image;
	char **send_update_ascii;
	char **receive_update_ascii;
	char **chmod;
	char **remove;
	char **mkdir;
	char **rmdir;
	char **process;
	char **normal;
} Color;

typedef struct {
	char **host_name;
	char **login_name;
	char **password;
	char **local_top_dir;
	char **remote_top_dir;
	cfgList **ignore_local_dir;
	cfgList **ignore_remote_dir;
	cfgList **ignore_local_file;
	cfgList **ignore_remote_file;
	cfgList **ignore_file;
	cfgList **keep_local_dir;
	cfgList **keep_remote_dir;
	cfgList **keep_local_file;
	cfgList **keep_remote_file;
	cfgList **keep_file;
	cfgList **ascii_local_dir;
	cfgList **ascii_remote_dir;
	cfgList **ascii_local_file;
	cfgList **ascii_remote_file;
	cfgList **ascii_file;
	cfgList **preserve_permission_local_dir;
	cfgList **preserve_permission_remote_dir;
	cfgList **preserve_permission_local_file;
	cfgList **preserve_permission_remote_file;
	cfgList **preserve_permission_file;
	cfgList **change_permission_local_dir;
	cfgList **change_permission_remote_dir;
	cfgList **change_permission_local_file;
	cfgList **change_permission_remote_file;
	cfgList **change_permission_file;
	char **cache_file;
	char **log_file;
	int *log_detail_level;
	int *nest_spaces;
	int *max_retry_to_send;
	bool *record_log;
	bool *conv_to_lower;
	bool *overwrite_ok;
	bool *silent;
	bool *suppress_progress;
	bool *suppress_non_error_messages;
	bool *exit_on_minor_error;
	bool *monochrome;
	bool *force;
	bool *ftp_passive;
	bool *show_hidden_files;
	bool *follow_symlinks;
	bool *regexp;
	bool *cache_facility;
	char **default_transfer_direction_string;
	char **detecting_modification_method_string;
	char **access_method_string;
	Direction *default_transfer_direction;
	DetectingModificationMethod *detecting_modification_method;
	AccessMethod *access_method;
	Color color;
} ConfigurationFileOption;


typedef struct {
	char *name;
	time_t time;
	off_t size;
	mode_t mode;
	bool isdir;
	bool isremoved;
} FileState;

typedef struct {
	char *path;
	bool isremoved;
	int num_files;
	FileState *file_state;
} PathState;

typedef struct {
	int num_paths;
	PathState *path_state;
} FileData;


typedef struct {
	Action action;
	char *file;
	char *src_dir;
	char *dest_dir;
	time_t time;
	off_t size;
	mode_t mode;
	bool isdir;
} Order;


/* definition/declaration of global variables */

GLOBAL CommandLineOption command_line_option;
GLOBAL ConfigurationFileOption config;


#undef GLOBAL
#undef GLOBAL_VAL
#endif /* VARIABLE_H_INCLUDED */
