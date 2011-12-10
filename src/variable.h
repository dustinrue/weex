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

#ifndef VARIABLE_H_INCLUDED
#define VARIABLE_H_INCLUDED

#ifdef GLOBAL_VALUE_DEFINE
#  define GLOBAL
#  define GLOBAL_VAL(v) =(v)
#else
#  define GLOBAL extern
#  define GLOBAL_VAL(v)  /* */
#endif


#include "parsecfg.h"
#include "ftplib.h"


#ifdef DEBUG
#  define DebugPrint(str) fprintf str
#else
#  define DebugPrint(str)
#endif

typedef enum{
	LOCAL,REMOTE
} LocalOrRemote;

typedef enum{
	CONNECT,DISCONNECT,ENTER,LEAVE,ASCII_SEND,BINARY_SEND,CHMOD,REMOVE,MKDIR,ASCII_LOWER_SEND,BINARY_LOWER_SEND,PROCESS
} Message;

typedef struct{
	char *name;
	long date;
	long time;
} Cache;

typedef struct{
	char *name;
	int max_file;
	Cache *ptr;
} Cache_dir;

typedef struct{
	char *name;
	long date;
	long time;
	int isdir;
} FileData;


GLOBAL netbuf *ftp_buf;

GLOBAL int host_number;
GLOBAL char *current_dir[2];
GLOBAL int is_cache_existent;

/* command line options */
GLOBAL int opt_silent;
GLOBAL int opt_force;
GLOBAL int opt_monochrome;
GLOBAL int opt_test;
GLOBAL int rebuild_cache;

/* configuration file options */
GLOBAL char **host_name;
GLOBAL char **login_name;
GLOBAL char **authorization_name;
GLOBAL char **password;
GLOBAL char **src_dir;
GLOBAL char **dest_dir;
GLOBAL cfgList **ignore_local_dir;
GLOBAL cfgList **ignore_local_file;
GLOBAL cfgList **ignore_remote_dir;
GLOBAL cfgList **ignore_remote_file;
GLOBAL cfgList **keep_remote_dir;
GLOBAL cfgList **ascii_file;
GLOBAL char **change_permission;
GLOBAL cfgList **change_permission_dir;
GLOBAL cfgList **preserve_permission_dir;
GLOBAL int *conv_to_lower;
GLOBAL int *overwrite_ok;
GLOBAL int *rename_ok;
GLOBAL int *record_log;
GLOBAL int *cfg_silent;
GLOBAL int *cfg_force;
GLOBAL int *cfg_monochrome;
GLOBAL int *ftp_passive;
GLOBAL int *nest_spaces;
GLOBAL int *log_detail_level;
GLOBAL int *max_retry_to_send;
GLOBAL int *show_hidden_file;
GLOBAL int *follow_symlinks;
GLOBAL int *chdir_at_connection;


#undef	GLOBAL
#undef	GLOBAL_VAL
#endif /* VARIABLE_H_INCLUDED */

