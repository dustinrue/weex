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

/* $Id: proto.h,v 1.1.1.1 2002/08/30 19:24:53 gmkun Exp $ */

#ifndef PROTO_H_INCLUDED
#define PROTO_H_INCLUDED

#include <time.h>


/* weex.c */
void internal_error(const char *file, int line);
void minor_error(int host);
void fatal_error(int host);

/* config.c */
int parse_config_file(void);
void display_host_definition(int argc, char *argv[], int max_hosts);
bool is_match_dir(const char *dir, cfgList *list, int host);
bool is_match_file(const char *file, const char *dir, cfgList *list, int host);

/* mainloop.c */
void main_loop(int argc, char *argv[], int max_hosts);

/* cache.c */
bool does_cache_exist(int host);
void load_cache(FileData *file_data, int host);
void save_cache(FileData *file_data, int host);
void add_file_to_cache(Order *order, FileData *remote_file_data, int host);
void mkdir_cache(Order *order, FileData *remote_file_data, int host);
void remove_cache(Order *order, FileData *remote_file_data, int host);
void rmdir_cache(Order *order, FileData *remote_file_data, int host);

/* local.c */
void get_local_file_data(FileData *file_data, int host);
void local_change_mode_if_necessary(const char *dest_dir, const char *dest_file, mode_t mode, int nest, int host);

/* filedata.c */
void sort_file_state(FileState *file_state, int num_files);
void sort_path_state(PathState *path_state, int num_paths);
void output_file_data(const FileData *file_data, int nest_level);
void free_file_data(FileData *file_data);
Order *compare_both_hosts_and_generate_order(FileData *local_file_data, FileData *remote_file_data, const char *local_top_dir, const char *remote_top_dir, Direction direction, int host);
void free_order(Order *order);
bool does_need_update(Order *order);
void put_listing_of_updated_file(Order *order, Direction direction);
void put_the_number_of_updated_file(Order *order, Direction direction);
void put_num_updated_file(Order *order, Direction direction);
int search_path_num(PathState *path_state, int num_paths, const char *path);
int search_file_num(PathState *path_state, int path_num, const char *name);

/* remote.c */
int connect_to_remote_host(int host);
void disconnect_from_remote_host(int host);
void get_remote_file_data(FileData *file_data, int host);

/* ftp.c */
int ftp_connect(int host);
int ftp_login(int host);
void ftp_disconnect(int host);
void ftp_get_file_list(const char *outfile, const char *path, int host);
void ftp_extract_file_status(char *text, char **name, off_t *size, mode_t *mode, int *type, int host);
time_t ftp_get_last_modification_time(const char *path_file, int host);
int ftp_chdir(const char *path);
int ftp_send_file(const char *src, const char *dest, int trans_mode, int host);
int ftp_receive_file(const char *src, const char *dest, int trans_mode, int host);
int ftp_remove_file(const char *dest, int host);
int ftp_make_directory(const char *dest, int host);
int ftp_remove_directory(const char *dest, int host);
void ftp_change_mode_if_necessary(const char *dest_dir, const char *dest_file, mode_t mode, int nest, int host);

/* scprsync.c */
void ssh_get_file_list(const char *outfile, const char *path, int host);
void ssh_extract_file_status(char *text, char **name, time_t *time, off_t *size, mode_t *mode, int *type, int host);
int scp_send_file(const char *src, const char *dest, int host);
int scp_receive_file(const char *src, const char *dest, int host);
int rsync_send_file(const char *src, const char *dest, int host);
int rsync_receive_file(const char *src, const char *dest, int host);
void ssh_change_mode_if_necessary(const char *dest_dir, const char *dest_file, mode_t mode, int nest, int host);

/* sub.c */
long n_atol(const char *str, int n);
int is_match_regexp(const char *regexp, const char *string, int host);

/* execute.c */
void execute_order(Order *order, FileData *remote_file_data, Direction direction, int host);
void message(Action action, const char *text, int nest, int host);


#endif /* PROTO_H_INCLUDED */
