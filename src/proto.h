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

#ifndef PROTO_H_INCLUDED
#define PROTO_H_INCLUDED


#include <sys/types.h>

/* weex.c */

void put_usage(void);
void put_version(void);
void put_config(int max_hosts);


/* config.c */

void make_weex_directory(void);
char *config_location(void);
int read_config(void);
void check_permission(char *file);
void set_default(int max_hosts);
void set_str_default(int host,int default_num,char **str_pointer);
void set_list_default(int host,int default_num,cfgList **list_pointer);
void set_bool_default(int host,int default_num,int bool_default,int *bool_pointer);
void set_int_default(int host,int default_num,int default_val,int *int_pointer);
void str_attach_slash_to_trailing(int host,char **str_pointer);
void list_attach_slash_to_trailing(int host,cfgList **list_pointer);
void absolutize_path(int host,char *base_dir,cfgList **list_absolute_path);
void absolutize_file(int host,char *base_dir,cfgList **list_absolute_file);


/* ftp_main.c */

void ftp_main(int argc,char *argv[],int max_hosts);
int ftp_connect_init(void);
int argv_to_host_num(char *arg);
int ftp_connect(void);
int ftp_login(void);
void change_dir(char *path,LocalOrRemote side);
void up_dir(LocalOrRemote side);
void change_dir_actually(LocalOrRemote side);
void ftp_disconnect(void);
void ftp_recursive_search(void);
void remove_files_on_remote_only(int max_local_file,int max_remote_file,FileData *local_data,FileData *remote_data,int dir_nest,int *dir_removed_file);
int get_equivalent_remote_number(char *local_name,int max_remote_file,FileData *remote_data);
int get_equivalent_local_number(char *remote_name,int max_local_file,FileData *local_data);
void ftp_enter(char *dir_name,int remote_isdir,int dir_nest,int remote_num,int *dir_removed_file);
void ftp_remove(char *file_name,int dir_nest,int *dir_removed_file);
void remove_remote_dir(char *dir_name,int dir_nest,int *dir_removed_file);
void ftp_mkdir(char *dir_name);


/* cache.c */

void load_cache(void);
int get_cache(char *file_name,long *date,long *time);
void update_cache(char *file_name,long date,long time);
void del_cache(char *file_name);
void del_cache_dir(char *dir_name);
void save_cache(void);
void update_cache_directory(char *dir);
int find_cache_dir(char *name);
int get_remote_file_data_from_cache(FileData **remote_data);


/* message.c */

void put_mes(Message mes,int nest, ...);


/* filedata.c */

int get_local_file_data(FileData **local_data);
int get_remote_file_data(FileData **remote_data);
void free_file_data(FileData *file_data,int max_file_data);
int parse_file_and_filetype(char *str,char **file);
int is_ignore_file(char *file_name,LocalOrRemote side);
int is_ignore_dir(char *dir_name,LocalOrRemote side);
int is_ascii_file(char *file_name);
int is_change_permission_dir(void);
int is_preserve_permission_dir(void);
int is_keep_remote_dir(void);


/* sub.c */

long n_atol(char *str,int n);
int cmp_file_with_wildcard(char *real_name,char *wild_name);
char *cnvregexp(char *str);


/* log.c */

void log_init(void);
FILE *log_open(void);
void log_close(FILE *fp);
void log_write(char *str);
void log_flush(void);


#endif /* PROTO_H_INCLUDED */

