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

/* $Id: execute.c,v 1.1.1.1 2002/08/30 19:24:52 gmkun Exp $ */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "intl.h"
#include "ftplib.h"
#include "strlib.h"
#include "variable.h"
#include "proto.h"


static int get_transfer_mode(const char *dir, const char *file, Direction direction, int host);
static void send_file(Order *order, FileData *remote_file_data, Direction direction, int trans_mode, int nest, int host);
static void remove_file(Order *order, FileData *remote_file_data, Direction direction, int host);
static void make_directory(Order *order, FileData *remote_file_data, Direction direction, int host);
static void remove_directory(Order *order, FileData *remote_file_data, Direction direction, int host);


/* --- PUBLIC FUNCTIONS --- */

void execute_order(Order *order, FileData *remote_file_data, Direction direction, int host)
{
	int i;
	int nest = 1;
	char *temp;
	int trans_mode;

	for (i = 0; order[i].action != END; i++) {
		switch (order[i].action) {
		case ENTER:
			temp = str_dup_printf("%s/%s/", order[i].dest_dir, order[i].file);
			message(ENTER, temp, nest, host);
			free(temp);
			nest++;
			break;

		case LEAVE:
			nest--;
			temp = str_dup_printf("%s/%s/", order[i].dest_dir, order[i].file);
			message(LEAVE, temp, nest, host);
			free(temp);
			break;

		case NEW:
			trans_mode = get_transfer_mode(order[i].src_dir, order[i].file, direction, host);
			if (trans_mode == FTPLIB_IMAGE) {
				if (direction == UPLOAD) {
					message(SEND_NEW_IMAGE, order[i].file, nest, host);
				} else {
					message(RECEIVE_NEW_IMAGE, order[i].file, nest, host);
				}
			} else {
				if (direction == UPLOAD) {
					message(SEND_NEW_ASCII, order[i].file, nest, host);
				} else {
					message(RECEIVE_NEW_ASCII, order[i].file, nest, host);
				}
			}
			send_file(order + i, remote_file_data, direction, trans_mode, nest, host);
			break;

		case UPDATE:
			trans_mode = get_transfer_mode(order[i].src_dir, order[i].file, direction, host);
			if (trans_mode == FTPLIB_IMAGE) {
				if (direction == UPLOAD) {
					message(SEND_UPDATE_IMAGE, order[i].file, nest, host);
				} else {
					message(RECEIVE_UPDATE_IMAGE, order[i].file, nest, host);
				}
			} else {
				if (direction == UPLOAD) {
					message(SEND_UPDATE_ASCII, order[i].file, nest, host);
				} else {
					message(RECEIVE_UPDATE_ASCII, order[i].file, nest, host);
				}
			}
			if (!config.overwrite_ok[host]) {
				remove_file(order + i, remote_file_data, direction, host);
			}
			send_file(order + i, remote_file_data, direction, trans_mode, nest, host);
			break;

		case REMOVE:
			message(REMOVE, order[i].file, nest, host);
			remove_file(order + i, remote_file_data, direction, host);
			break;

		case MKDIR:
			temp = str_dup_printf("%s/%s/", order[i].dest_dir, order[i].file);
			message(MKDIR, temp, nest, host);
			free(temp);
			make_directory(order + i, remote_file_data, direction, host);
			break;

		case RMDIR:
			temp = str_dup_printf("%s/%s/", order[i].dest_dir, order[i].file);
			message(RMDIR, temp, nest, host);
			free(temp);
			remove_directory(order + i, remote_file_data, direction, host);
			break;

		default:
			internal_error(__FILE__, __LINE__);
		}
	}
}


void message(Action action, const char *text, int nest, int host)
{
	int i, j, k;
	struct {
		Action action;
		char *color;
		char *message;
	} message_table[] = {
		{ CONNECT,              config.color.connect[host],              _("Connect    : ") },
		{ DISCONNECT,           config.color.disconnect[host],           _("Disconnect : ") },
		{ ENTER,                config.color.enter[host],                _("Entering   : ") },
		{ LEAVE,                config.color.leave[host],                _("Leaving    : ") },
		{ SEND_NEW_IMAGE,       config.color.send_new_image[host],       _("Sending    : ") },
		{ SEND_NEW_ASCII,       config.color.send_new_ascii[host],       _("Sending    : ") },
		{ SEND_UPDATE_IMAGE,    config.color.send_update_image[host],    _("Sending    : ") },
		{ SEND_UPDATE_ASCII,    config.color.send_update_ascii[host],    _("Sending    : ") },
		{ RECEIVE_NEW_IMAGE,    config.color.receive_new_image[host],    _("Receiving  : ") },
		{ RECEIVE_NEW_ASCII,    config.color.receive_new_ascii[host],    _("Receiving  : ") },
		{ RECEIVE_UPDATE_IMAGE, config.color.receive_update_image[host], _("Receiving  : ") },
		{ RECEIVE_UPDATE_ASCII, config.color.receive_update_ascii[host], _("Receiving  : ") },
		{ CHMOD,                config.color.chmod[host],                _("Chmod      : ") },
		{ REMOVE,               config.color.remove[host],               _("Removing   : ") },
		{ MKDIR,                config.color.mkdir[host],                _("Making     : ") },
		{ RMDIR,                config.color.rmdir[host],                _("Removing   : ") },
		{ PROCESS,              config.color.process[host],              _("Processing : ") },
	};

	if (command_line_option.silent || command_line_option.quiet ||
	    config.silent[host] || config.suppress_non_error_messages[host]) {
		return;
	}

	for (i = 0; i < sizeof(message_table) / sizeof(message_table[0]); i++) {
		if (action == message_table[i].action) {
			printf("%s", message_table[i].message);
			for (j = 0; j < nest ; j++) {
				for (k = 0; k < config.nest_spaces[host]; k++) {
					printf(" ");
				}
			}
			if (config.monochrome[host] || command_line_option.monochrome) {
				printf("%s\n", text);
			} else {
				printf("\x1b[%sm%s\x1b[%sm\n", message_table[i].color, text, config.color.normal[host]);
			}
			if (action == DISCONNECT) {
				printf("\n");
			}
			break;
                }
        }
}


/* --- PRIVATE FUNCTIONS --- */

static int get_transfer_mode(const char *dir, const char *file, Direction direction, int host)
{
	char *full_path_name;
	int result = FTPLIB_IMAGE;

	full_path_name = str_concat(dir, "/", file, NULL);

	if (direction == UPLOAD) {
		if (is_match_dir(dir, config.ascii_local_dir[host], host) ||
		    is_match_file(full_path_name, dir, config.ascii_local_file[host], host) ||
		    is_match_file(full_path_name, dir, config.ascii_file[host], host)) {
			result = FTPLIB_ASCII;
		}
	} else if (direction == DOWNLOAD) {
		if (is_match_dir(dir, config.ascii_remote_dir[host], host) ||
		    is_match_file(full_path_name, dir, config.ascii_remote_file[host], host) ||
		    is_match_file(full_path_name, dir, config.ascii_file[host], host)) {
			result = FTPLIB_ASCII;
		}
	} else {
		internal_error(__FILE__, __LINE__);
	}

	free(full_path_name);
	return (result);
}


static void send_file(Order *order, FileData *remote_file_data, Direction direction, int trans_mode, int nest, int host)
{
	char *src_path;
	char *dest_path;

	src_path = str_concat(order->src_dir, "/", order->file, NULL);
	dest_path = str_concat(order->dest_dir, "/", order->file, NULL);

	if (direction == UPLOAD) {
		switch (config.access_method[host]) {
		case FTP:
			if (ftp_send_file(src_path, dest_path, trans_mode, host) == 0) {
				add_file_to_cache(order, remote_file_data, host);
				ftp_change_mode_if_necessary(order->dest_dir, order->file, order->mode, nest, host);
			}
			break;
		case SCP:
			if (scp_send_file(src_path, order->dest_dir, host) == 0) {
				add_file_to_cache(order, remote_file_data, host);
				ssh_change_mode_if_necessary(order->dest_dir, order->file, order->mode, nest, host);
			}
			break;
		case RSYNC:
			if (rsync_send_file(src_path, order->dest_dir, host) == 0) {
				add_file_to_cache(order, remote_file_data, host);
				ssh_change_mode_if_necessary(order->dest_dir, order->file, order->mode, nest, host);
			}
			break;
		default:
			internal_error(__FILE__, __LINE__);
		}
		
	} else if (direction == DOWNLOAD) {
		switch (config.access_method[host]) {
		case FTP:
			if (ftp_receive_file(src_path, dest_path, trans_mode, host) == 0) {
				local_change_mode_if_necessary(order->dest_dir, order->file, order->mode, nest, host);
			}
			break;
		case SCP:
			if (scp_receive_file(src_path, order->dest_dir, host) == 0) {
				local_change_mode_if_necessary(order->dest_dir, order->file, order->mode, nest, host);
			}
			break;
		case RSYNC:
			if (rsync_receive_file(src_path, order->dest_dir, host) == 0) {
				local_change_mode_if_necessary(order->dest_dir, order->file, order->mode, nest, host);
			}
			break;
		default:
			internal_error(__FILE__, __LINE__);
		}
	} else {
		internal_error(__FILE__, __LINE__);
	}

	free(src_path);
	free(dest_path);
}


static void remove_file(Order *order, FileData *remote_file_data, Direction direction, int host)
{
	char *dest_path;

	dest_path = str_concat(order->dest_dir, "/", order->file, NULL);

	if (config.access_method[host] == FTP) {
		if (direction == UPLOAD) {
			ftp_remove_file(dest_path, host);
		} else if (direction == DOWNLOAD) {
			unlink(dest_path);
		} else {
			internal_error(__FILE__, __LINE__);
		}
	} else {
		internal_error(__FILE__, __LINE__);
	}

	if (direction == UPLOAD) {
		remove_cache(order, remote_file_data, host);
	}

	free(dest_path);
}


static void make_directory(Order *order, FileData *remote_file_data, Direction direction, int host)
{
	char *dest_path;

	dest_path = str_concat(order->dest_dir, "/", order->file, NULL);

	if (config.access_method[host] == FTP) {
		if (direction == UPLOAD) {
			ftp_make_directory(dest_path, host);
		} else if (direction == DOWNLOAD) {
			mkdir(dest_path, 0755);
		} else {
			internal_error(__FILE__, __LINE__);
		}
	} else {
		internal_error(__FILE__, __LINE__);
	}

	if (direction == UPLOAD) {
		mkdir_cache(order, remote_file_data, host);
	}

	free(dest_path);
}


static void remove_directory(Order *order, FileData *remote_file_data, Direction direction, int host)
{
	char *dest_path;

	dest_path = str_concat(order->dest_dir, "/", order->file, NULL);

	if (config.access_method[host] == FTP) {
		if (direction == UPLOAD) {
			ftp_remove_directory(dest_path, host);
		} else if (direction == DOWNLOAD) {
			rmdir(dest_path);
		} else {
			internal_error(__FILE__, __LINE__);
		}
	} else {
		internal_error(__FILE__, __LINE__);
	}

	if (direction == UPLOAD) {
		rmdir_cache(order, remote_file_data, host);
	}

	free(dest_path);
}
