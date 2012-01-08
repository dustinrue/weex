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

/* $Id: config.c,v 1.1.1.1 2002/08/30 19:24:52 gmkun Exp $ */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <fnmatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_LIBREADLINE
#  include <readline/readline.h>
#  include <readline/history.h>
#endif

#include "intl.h"
#include "parsecfg.h"
#include "strlib.h"
#include "variable.h"
#include "proto.h"


#define NO_DEFAULT 1
#define DEFAULT_EXIST 2


static cfgStruct config_table[] = {
        /* parameter                      type             address of variable */
	{ "HostName",                       CFG_STRING,      &config.host_name                            },
	{ "LoginName",                      CFG_STRING,      &config.login_name                           },
	{ "Password",                       CFG_STRING,      &config.password                             },
	{ "LocalTopDir",                    CFG_STRING,      &config.local_top_dir                        },
	{ "RemoteTopDir",                   CFG_STRING,      &config.remote_top_dir                       },
	{ "IgnoreLocalDir",                 CFG_STRING_LIST, &config.ignore_local_dir                     },
	{ "IgnoreRemoteDir",                CFG_STRING_LIST, &config.ignore_remote_dir                    },
	{ "IgnoreLocalFile",                CFG_STRING_LIST, &config.ignore_local_file                    },
	{ "IgnoreRemoteFile",               CFG_STRING_LIST, &config.ignore_remote_file                   },
	{ "IgnoreFile",                     CFG_STRING_LIST, &config.ignore_file                          },
	{ "KeepLocalDir",                   CFG_STRING_LIST, &config.keep_local_dir                       },
	{ "KeepRemoteDir",                  CFG_STRING_LIST, &config.keep_remote_dir                      },
	{ "KeepLocalFile",                  CFG_STRING_LIST, &config.keep_local_file                      },
	{ "KeepRemoteFile",                 CFG_STRING_LIST, &config.keep_remote_file                     },
	{ "KeepFile",                       CFG_STRING_LIST, &config.keep_file                            },
	{ "AsciiLocalDir",                  CFG_STRING_LIST, &config.ascii_local_dir                      },
	{ "AsciiRemoteDir",                 CFG_STRING_LIST, &config.ascii_remote_dir                     },
	{ "AsciiLocalFile",                 CFG_STRING_LIST, &config.ascii_local_file                     },
	{ "AsciiRemoteFile",                CFG_STRING_LIST, &config.ascii_remote_file                    },
	{ "AsciiFile",                      CFG_STRING_LIST, &config.ascii_file                           },
	{ "PreservePermissionLocalDir",     CFG_STRING_LIST, &config.preserve_permission_local_dir        },
	{ "PreservePermissionRemoteDir",    CFG_STRING_LIST, &config.preserve_permission_remote_dir       },
	{ "PreservePermissionLocalFile",    CFG_STRING_LIST, &config.preserve_permission_local_file       },
	{ "PreservePermissionRemoteFile",   CFG_STRING_LIST, &config.preserve_permission_remote_file      },
	{ "PreservePermissionFile",         CFG_STRING_LIST, &config.preserve_permission_file             },
	{ "ChangePermissionLocalDir",       CFG_STRING_LIST, &config.change_permission_local_dir          },
	{ "ChangePermissionRemoteDir",      CFG_STRING_LIST, &config.change_permission_remote_dir         },
	{ "ChangePermissionLocalFile",      CFG_STRING_LIST, &config.change_permission_local_file         },
	{ "ChangePermissionRemoteFile",     CFG_STRING_LIST, &config.change_permission_remote_file        },
	{ "ChangePermissionFile",           CFG_STRING_LIST, &config.change_permission_file               },
	{ "CacheFile",                      CFG_STRING,      &config.cache_file                           },
	{ "RecordLog",                      CFG_BOOL,        &config.record_log                           },
	{ "LogFile",                        CFG_STRING,      &config.log_file                             },
	{ "LogDetailLevel",                 CFG_INT,         &config.log_detail_level                     },
	{ "NestSpaces",                     CFG_INT,         &config.nest_spaces                          },
	{ "MaxRetryToSend",                 CFG_INT,         &config.max_retry_to_send                    },
	{ "ConvToLower",                    CFG_BOOL,        &config.conv_to_lower                        },
	{ "OverwriteOK",                    CFG_BOOL,        &config.overwrite_ok                         },
	{ "Silent",                         CFG_BOOL,        &config.silent                               },
	{ "SuppressProgress",               CFG_BOOL,        &config.suppress_progress                    },
	{ "SuppressNonErrorMessages",       CFG_BOOL,        &config.suppress_non_error_messages          },
	{ "ExitOnMinorError",               CFG_BOOL,        &config.exit_on_minor_error                  },
	{ "Monochrome",                     CFG_BOOL,        &config.monochrome                           },
	{ "Force",                          CFG_BOOL,        &config.force                                },
	{ "FtpPassive",                     CFG_BOOL,        &config.ftp_passive                          },
	{ "ShowHiddenFiles",                CFG_BOOL,        &config.show_hidden_files                    },
	{ "FollowSymlinks",                 CFG_BOOL,        &config.follow_symlinks                      },
	{ "RegularExpression",              CFG_BOOL,        &config.regexp                               },
	{ "DefaultTransferDirection",       CFG_STRING,      &config.default_transfer_direction_string    },
	{ "DetectingModificationMethod",    CFG_STRING,      &config.detecting_modification_method_string },
	{ "AccessMethod",                   CFG_STRING,      &config.access_method_string                 },
	{ "DefaultTransferDirectionNum",    CFG_INT,         &config.default_transfer_direction           },
	{ "DetectingModificationMethodNum", CFG_INT,         &config.detecting_modification_method        },
	{ "AccessMethodNum",                CFG_INT,         &config.access_method                        },
	{ "CacheFacility",                  CFG_BOOL,        &config.cache_facility                       },
	{ "ColorConnect",                   CFG_STRING,      &config.color.connect                        },
	{ "ColorDisconnect",                CFG_STRING,      &config.color.disconnect                     },
	{ "ColorEnter",                     CFG_STRING,      &config.color.enter                          },
	{ "ColorLeave",                     CFG_STRING,      &config.color.leave                          },
	{ "ColorSendNewImage",              CFG_STRING,      &config.color.send_new_image                 },
	{ "ColorSendNewAscii",              CFG_STRING,      &config.color.send_new_ascii                 },
	{ "ColorSendUpdateImage",           CFG_STRING,      &config.color.send_update_image              },
	{ "ColorSendUpdateAscii",           CFG_STRING,      &config.color.send_update_ascii              },
	{ "ColorReceiveNewImage",           CFG_STRING,      &config.color.receive_new_image              },
	{ "ColorReceiveNewAscii",           CFG_STRING,      &config.color.receive_new_ascii              },
	{ "ColorReceiveUpdateImage",        CFG_STRING,      &config.color.receive_update_image           },
	{ "ColorReceiveUpdateAscii",        CFG_STRING,      &config.color.receive_update_ascii           },
	{ "ColorChmod",                     CFG_STRING,      &config.color.chmod                          },
	{ "ColorRemove",                    CFG_STRING,      &config.color.remove                         },
	{ "ColorMkdir",                     CFG_STRING,      &config.color.mkdir                          },
	{ "ColorRmdir",                     CFG_STRING,      &config.color.rmdir                          },
	{ "ColorProcess",                   CFG_STRING,      &config.color.process                        },
	{ "ColorDefault",                   CFG_STRING,      &config.color.normal                         },
        { NULL, CFG_END, NULL },
};

/* Internal default value of parameters are defined in get_*_internal_default(). */


static void display_specified_host_definition(int host);
static void set_default(int max_hosts);
static char *get_string_internal_default(const char *parameter, int host);
static int get_integer_internal_default(const char *parameter);
static bool get_boolean_internal_default(const char *parameter);
static void check_and_convert_string_value(int max_hosts);
static void setup_wizard(void);
static int configure_new_host(int max_hosts);
static void reconfigure_existing_host(int max_hosts);
static void print_current_configuration(int max_hosts);
static void configure_host(int host_num, int hasdefault);
static char *input_string(const char *message);
static void check_permission(char *file);
static void remove_trailing_slash(int config_num, int host);
static void absolutize_path(int config_num, int host);
static cfgList *copy_cfg_list(const cfgList *src);


/* --- PUBLIC FUNCTION --- */

int parse_config_file(void)
{
	int max_hosts;

	if (access(command_line_option.config_file, R_OK) != 0) {
		fprintf(stderr, _("Configuration file `%s' is not found.\n"), command_line_option.config_file);
		fprintf(stderr, _("Running weex setup wizard...\n\n"));
		setup_wizard();
		exit(0);
	}

	max_hosts = cfgParse(command_line_option.config_file, config_table, CFG_INI);
	if (max_hosts == 0) {
		fprintf(stderr, _("There is no section in configuration file `%s'.\n"), command_line_option.config_file);
		exit(1);
	}
	if (max_hosts == -1) {
		fprintf(stderr, _("\nAn error has occured while parsing configuration file `%s'.\n"), command_line_option.config_file);
		exit(1);
	}

	set_default(max_hosts);
	check_and_convert_string_value(max_hosts);
	check_permission(command_line_option.config_file);

	return (max_hosts);
}


void display_host_definition(int argc, char *argv[], int max_hosts)
{
	int i;
	int host_num;

	if (argc > 1) {
		for (i = 0; i < argc - 1; i++) {
			host_num = atoi(argv[i + 1]) - 1;
			if (host_num == -1) { /* specified as name */
				host_num = cfgSectionNameToNumber(argv[i + 1]);
				if (host_num == -1) {
					fprintf(stderr, _("Specified host (section) name `%s' is undefined.\n"), argv[i + 1]);
					fprintf(stderr, "\n");
					continue;
				}
			} else { /* specified as number */
				if (cfgSectionNumberToName(host_num) == NULL) {
					fprintf(stderr, _("Specified host (section) number `%s' is undefined.\n"), argv[i + 1]);
					fprintf(stderr, "\n");
					continue;
				}
			}
			display_specified_host_definition(host_num);
		}
	} else {
		for (i = 0; i < max_hosts; i++) {
			display_specified_host_definition(i);
		}
	}
}


bool is_match_dir(const char *dir, cfgList *list, int host)
{
	cfgList *l;

	for (l = list; l != NULL; l = l->next) {
		if (config.regexp[host]) {
			if (is_match_regexp(l->str, dir, host)) {
				return (TRUE);
			}
		} else {
			if (fnmatch(l->str, dir, 0) == 0) {
				return (TRUE);
			}
		}
	}
	return (FALSE);
}


bool is_match_file(const char *file, const char *dir, cfgList *list, int host)
{
	cfgList *l;
	char *s;

	for (l = list; l != NULL; l = l->next) {
		if (strchr(l->str, '/') == NULL) {
			s = str_concat(dir, "/", l->str, NULL);
		} else {
			s = str_dup(l->str);
		}

		if (config.regexp[host]) {
			if (is_match_regexp(s, file, host)) {
				free(s);
				return (TRUE);
			}
		} else {
			if (fnmatch(s, file, 0) == 0) {
				free(s);
				return (TRUE);
			}
		}

		free(s);
	}
	return (FALSE);
}


/* --- PRIVATE FUNCTION --- */

static void display_specified_host_definition(int host)
{
	int j;
	int flag;
	char *string;
	cfgList *string_list;
	bool boolean;
	int integer;

	printf("%d [%s]\n", host + 1, cfgSectionNumberToName(host));
	for (j = 0; config_table[j].type != CFG_END; j++) {
		switch (config_table[j].type) {
		case CFG_STRING:
			string = *(*(char ***) (config_table[j].value) + host);
			if (string == NULL) {
				printf(_("%-30s : not configured\n"), config_table[j].parameterName);
			} else {
				if (strcmp(config_table[j].parameterName, "Password") == 0) {
					printf("%-30s = ********\n", config_table[j].parameterName);
				} else if (string[0] == '\0' && strcmp(config_table[j].parameterName + strlen(config_table[j].parameterName) - 3, "Dir") == 0) {
					printf("%-30s = /\n", config_table[j].parameterName);
				} else {
					printf("%-30s = %s\n", config_table[j].parameterName, string);
				}
			}
			break;
		case CFG_STRING_LIST:
			string_list = *(*(cfgList ***) (config_table[j].value) + host);
			if (string_list == NULL) {
				printf(_("%-30s : not configured\n"), config_table[j].parameterName);
			} else {
				for (flag = 1; string_list != NULL; string_list = string_list->next, flag = 0) {
					if (string_list->str[0] == '\0' && strcmp(config_table[j].parameterName + strlen(config_table[j].parameterName) - 3, "Dir") == 0) {
						printf("%-30s = /\n", (flag ? config_table[j].parameterName : ""));
					} else {
						printf("%-30s = %s\n", (flag ? config_table[j].parameterName : ""), string_list->str);
					}
				}
			}
			break;
		case CFG_BOOL:
			boolean = *(*(bool **) (config_table[j].value) + host);
			printf("%-30s = %s\n", config_table[j].parameterName, (boolean ? "true" : "false"));
			break;
		case CFG_INT:
			integer = *(*(int **) (config_table[j].value) + host);
			printf("%-30s = %d\n", config_table[j].parameterName, integer);
			break;
		default:
			internal_error(__FILE__, __LINE__);
		}
	}
	printf("\n");
}


static void set_default(max_hosts)
{
	int i, j;
	int default_section;
	char *string_default;
	int integer_default;
	bool boolean_default;

	default_section = cfgSectionNameToNumber("default");
	if (default_section == -1) {
		fprintf(stderr, _("Section `default' is required.\n"));
		exit(1);
	}
	for (i = 0; i < max_hosts; i++) {
		for (j = 0; config_table[j].type != CFG_END; j++) {
			switch (config_table[j].type) {
			case CFG_STRING:
				if (*(*(char ***) (config_table[j].value) + i) == NULL) {	/* The section doesn't have the parameter. */
					if (*(*(char ***) (config_table[j].value) + default_section) == NULL) {	/* Section `default' doesn't have the parameter. */
						string_default = get_string_internal_default(config_table[j].parameterName, i);
						if (string_default != NULL) {	/* If weex has internal default value, use it. */
							*(*(char ***) (config_table[j].value) + i) = string_default;
						}
					} else {	/* Section `default' has the parameter. */
						if (strcmp(config_table[j].parameterName, "CacheFile") == 0) { /* Add section name to the value, if parameter is `CacheFile'. */
							*(*(char ***) (config_table[j].value) + i) = str_concat(*(*(char ***) (config_table[j].value) + default_section), ".", cfgSectionNumberToName(i), NULL);
						} else {
							*(*(char ***) (config_table[j].value) + i) = str_dup(*(*(char ***) (config_table[j].value) + default_section));
						}
					}
				}
				break;
			case CFG_INT:
				if (*(*(int **) (config_table[j].value) + i) == -1) {	/* The section doesn't have the parameter. */
					if (*(*(int **) (config_table[j].value) + default_section) == -1) {	/* Section `default' doesn't have the parameter. */
						integer_default = get_integer_internal_default(config_table[j].parameterName);
						if (integer_default != -1) {	/* If weex has internal default value, use it. */
							*(*(int **) (config_table[j].value) + i) = integer_default;
						}
					} else {	/* Section `default' has the parameter. */
						*(*(int **) (config_table[j].value) + i) = *(*(int **) (config_table[j].value) + default_section);
					}
				}
				break;
			case CFG_BOOL:
				if (*(*(bool **) (config_table[j].value) + i) == -1) {	/* The section doesn't have the parameter. */
					if (*(*(bool **) (config_table[j].value) + default_section) == -1) {	/* Section `default' doesn't have the parameter. */
						boolean_default = get_boolean_internal_default(config_table[j].parameterName);
						if (boolean_default != -1) {	/* If weex has internal default value, use it. */
							*(*(bool **) (config_table[j].value) + i) = boolean_default;
						}
					} else {	/* Section `default' has the parameter. */
						*(*(bool **) (config_table[j].value) + i) = *(*(bool **) (config_table[j].value) + default_section);
					}
				}
				break;
			case CFG_STRING_LIST:
				if (*(*(cfgList ***) (config_table[j].value) + i) == NULL) {	/* The section doesn't have the parameter. */
					if (*(*(cfgList ***) (config_table[j].value) + default_section) != NULL) {	/* Section `default' have the parameter. */
						*(*(cfgList ***) (config_table[j].value) + i) = copy_cfg_list(*(*(cfgList ***) (config_table[j].value) + default_section));
					}
				}
				break;
			default:
				internal_error(__FILE__, __LINE__);
			}
			remove_trailing_slash(j, i);
			absolutize_path(j, i);
		}
	}
}


static char *get_string_internal_default(const char *parameter, int host)
{
	int i;
	struct {
		char *parameter;
		char *value;
	} string_default[] = {
		{ "CacheFile",                   str_concat(getenv("HOME"), "/.weex/weex.cache.", cfgSectionNumberToName(host), NULL) },
		{ "DefaultTransferDirection",    "upload"   },
		{ "DetectingModificationMethod", "timesize" },
		{ "AccessMethod",                "ftp"      },
		{ "ColorConnect",                "33"       },
		{ "ColorDisconnect",             "33"       },
		{ "ColorEnter",                  "36"       },
		{ "ColorLeave",                  "36"       },
		{ "ColorSendNewImage",           "35;1"     },
		{ "ColorReceiveNewImage",        "35;1"     },
		{ "ColorSendNewAscii",           "32;1"     },
		{ "ColorReceiveNewAscii",        "32;1"     },
		{ "ColorSendUpdateImage",        "35"       },
		{ "ColorReceiveUpdateImage",     "35"       },
		{ "ColorSendUpdateAscii",        "32"       },
		{ "ColorReceiveUpdateAscii",     "32"       },
		{ "ColorChmod",                  "33"       },
		{ "ColorRemove",                 "31"       },
		{ "ColorMkdir",                  "34"       },
		{ "ColorRmdir",                  "31"       },
		{ "ColorProcess",                "0"        },
		{ "ColorDefault",                "0"        },
	};
	char *s;

	for (i = 0; i < sizeof(string_default) / sizeof(string_default[0]); i++) {
		if (strcmp(string_default[i].parameter, parameter) == 0) {
			s = str_dup(string_default[i].value);
			free(string_default[0].value);
			return (s);
		}
	}
	free(string_default[0].value);
	return (NULL);
}


static int get_integer_internal_default(const char *parameter)
{
	int i;
	struct {
		char *parameter;
		int value;
	} integer_default[] = {
		{ "LogDetailLevel", 1 },
		{ "NestSpaces",     4 },
		{ "MaxRetryToSend", 8 }
	};

	for (i = 0; i < sizeof(integer_default) / sizeof(integer_default[0]); i++) {
		if (strcmp(integer_default[i].parameter, parameter) == 0) {
			return (integer_default[i].value);
		}
	}
	return (-1);
}


static bool get_boolean_internal_default(const char *parameter)
{
	int i;
	struct {
		char *parameter;
		bool value;
	} boolean_default[] = {
		{ "RecordLog",                TRUE  },
		{ "ConvToLower",              FALSE },
		{ "OverwriteOK",              TRUE  },
		{ "Silent",                   FALSE },
		{ "SuppressProgress",         FALSE },
		{ "SuppressNonErrorMessages", FALSE },
		{ "SuppressAllMessages",      FALSE },
		{ "ExitOnMinorError",         FALSE },
		{ "Monochrome",               FALSE },
		{ "Force",                    FALSE },
		{ "FtpPassive",               FALSE },
		{ "ShowHiddenFiles",          FALSE },
		{ "FollowSymlinks",           FALSE },
		{ "RegularExpression",        FALSE },
		{ "CacheFacility",            TRUE  }
	};
	for (i = 0; i < sizeof(boolean_default) / sizeof(boolean_default[0]); i++) {
		if (strcmp(boolean_default[i].parameter, parameter) == 0) {
			return (boolean_default[i].value);
		}
	}
	return (-1);
}


static void remove_trailing_slash(int config_num, int host)
{
	int i;
	char *dir_parameter[] = {
		"LocalTopDir",
		"RemoteTopDir",
		"IgnoreLocalDir",
		"IgnoreRemoteDir",
		"KeepLocalDir",
		"KeepRemoteDir",
		"AsciiLocalDir",
		"AsciiRemoteDir",
		"PreservePermissionLocalDir",
		"PreservePermissionRemoteDir",
		"ChangePermissionLocalDir",
		"ChangePermissionRemoteDir",
	};
	char *p;
	cfgList *l;

	for (i = 0; i < sizeof(dir_parameter) / sizeof(dir_parameter[0]); i++) {
		if (strcmp(dir_parameter[i], config_table[config_num].parameterName) == 0) {
			switch (config_table[config_num].type) {
			case CFG_STRING:
				p = *(*(char ***) (config_table[config_num].value) + host);
				if (p == NULL) {
					return;
				}
				if (*(p + strlen(p) - 1) == '/') {
					*(p + strlen(p) - 1) = '\0';
				}
				return;
			case CFG_STRING_LIST:
				for (l = *(*(cfgList ***) (config_table[config_num].value) + host); l != NULL; l = l->next) {
					if(*(l->str + strlen(l->str) - 1) == '/') {
						*(l->str + strlen(l->str) - 1) = '\0';
					}
				}
				return;
			default:
				internal_error(__FILE__, __LINE__);
			}
		}
	}
}


static void absolutize_path(int config_num, int host)
{
	int i;
	char *path_parameter[] = {
		"IgnoreLocalDir",
		"IgnoreRemoteDir",
		"IgnoreLocalFile",
		"IgnoreRemoteFile",
		"KeepLocalDir",
		"KeepRemoteDir",
		"KeepLocalFile",
		"KeepRemoteFile",
		"AsciiLocalDir",
		"AsciiRemoteDir",
		"AsciiLocalFile",
		"AsciiRemoteFile",
		"PreservePermissionLocalDir",
		"PreservePermissionRemoteDir",
		"PreservePermissionLocalFile",
		"PreservePermissionRemoteFile",
		"ChangePermissionLocalDir",
		"ChangePermissionRemoteDir",
		"ChangePermissionLocalFile",
		"ChangePermissionRemoteFile",
	};
	cfgList *l;
	char *temp;
	char *base_dir;

	for (i = 0; i < sizeof(path_parameter) / sizeof(path_parameter[0]); i++) {
		if (strcmp(path_parameter[i], config_table[config_num].parameterName) == 0) {
			if (strstr(path_parameter[i], "Local") != NULL) {
				base_dir = config.local_top_dir[host];
			} else if (strstr(path_parameter[i], "Remote") != NULL) {
				base_dir = config.remote_top_dir[host];
			} else {
				internal_error(__FILE__, __LINE__);
			}
			if (base_dir == NULL) {
				return;
			}

			for (l = *(*(cfgList ***) (config_table[config_num].value) + host); l != NULL; l = l->next) {
				if (strcmp(path_parameter[i] + strlen(path_parameter[i]) - 4, "File") == 0) { /* file */
					if (strchr(l->str, '/') == NULL) { /* not contain directory */
						continue;
					}
				} else if (strcmp(path_parameter[i] + strlen(path_parameter[i]) - 3, "Dir") != 0) {
					internal_error(__FILE__, __LINE__);
				}
				if (*(l->str) != '/') { /* relative path */
					temp = l->str;
					if (strncmp(temp, "./", 2) == 0) { /* begin with "./" */
						l->str = str_concat(base_dir, "/", temp + 2, NULL);
					} else {
						l->str = str_concat(base_dir, "/", temp, NULL);
					}
					free(temp);
				}
			}
			return;
		}
	}
}


static void check_and_convert_string_value(int max_hosts)
{
	int i;

	for (i = 0; i < max_hosts; i++) {
		if (strcasecmp(config.default_transfer_direction_string[i], "upload") == 0) {
			config.default_transfer_direction[i] = UPLOAD;
		} else if (strcasecmp(config.default_transfer_direction_string[i], "download") == 0) {
			config.default_transfer_direction[i] = DOWNLOAD;
		} else {
			fprintf(stderr, _("`%s' is wrong value for parameter `DefaultTransferDirection' in section `%s'.\n"
					  "It must be `upload' or `download'.\n"), config.default_transfer_direction_string[i], cfgSectionNumberToName(i));
			exit(1);
		}

		if (strcasecmp(config.detecting_modification_method_string[i], "time") == 0) {
			config.detecting_modification_method[i] = TIME;
		} else if (strcasecmp(config.detecting_modification_method_string[i], "size") ==0) {
			config.detecting_modification_method[i] = SIZE;
		} else if (strcasecmp(config.detecting_modification_method_string[i], "timesize") == 0) {
			config.detecting_modification_method[i] = TIMESIZE;
		} else {
			fprintf(stderr, _("`%s' is wrong value for parameter `DetectingModificationMethod' in section `%s'.\n"
					  "It must be `time' or `size' or `timesize'.\n"), config.detecting_modification_method_string[i], cfgSectionNumberToName(i));
			exit(1);
		}

		if (strcasecmp(config.access_method_string[i], "ftp") ==0) {
			config.access_method[i] = FTP;
		} else if (strcasecmp(config.access_method_string[i], "scp") ==0) {
			config.access_method[i] = SCP;
		} else if (strcasecmp(config.access_method_string[i], "rsync") ==0) {
			config.access_method[i] = RSYNC;
		} else {
			fprintf(stderr, _("`%s' is wrong value for parameter `AccessMethod' in section `%s'.\n"
					  "It must be `ftp' or `rsync' or `scp'.\n"), config.detecting_modification_method_string[i], cfgSectionNumberToName(i));
			exit(1);
		}
	}
}


static void setup_wizard(void)
{
	int max_hosts = 0;
	char *input;
	int num;

	printf(_("Welcome to weex setup wizard!\n"));
	printf(_("You can configure weex by just answering some questions with this wizard.\n"));
	printf(_("If you want to write weexrc file by hand or just you specified wrong\n"
		 "configuration file, kill the process by Ctrl-C.\n\n"));

	for (;;) {
		if (max_hosts == 0) {
			max_hosts = configure_new_host(max_hosts);
		}

		input = input_string(_("\n"
				       "1) configure new host\n"
				       "2) reconfigure existing host\n"
				       "3) display current configuration\n"
				       "4) write weexrc and exit\n\n"));
		num = atoi(input);
		free(input);

		switch (num) {
		case 1:
			max_hosts = configure_new_host(max_hosts);
			break;
		case 2:
			reconfigure_existing_host(max_hosts);
			break;
		case 3:
			print_current_configuration(max_hosts);
			break;
		case 4:
			max_hosts = cfgAllocForNewSection(config_table, "default");
			cfgStoreValue(config_table, "IgnoreLocalFile", "*~", CFG_INI, max_hosts - 1);
			cfgStoreValue(config_table, "IgnoreLocalFile", "*.tmp", CFG_INI, max_hosts - 1);
			cfgStoreValue(config_table, "IgnoreLocalFile", "*.bak", CFG_INI, max_hosts - 1);
			cfgStoreValue(config_table, "AsciiFile", "*.htm", CFG_INI, max_hosts - 1);
			cfgStoreValue(config_table, "AsciiFile", "*.html", CFG_INI, max_hosts - 1);
			cfgStoreValue(config_table, "AsciiFile", "*.txt", CFG_INI, max_hosts - 1);
			
			if (cfgDump(command_line_option.config_file, config_table, CFG_INI, max_hosts) == 0) {
				printf(_("Configuration file `%s' has been created.\n"), command_line_option.config_file);
				return;
			} else {
				fprintf(stderr, _("An error has occured while writing configuration file `%s'.\n"), command_line_option.config_file);
				exit(1);
			}
		default:
			printf(_("Enter between 1 and 4.\n\n"));
		}
	}
}


static int configure_new_host(int max_hosts)
{
	char *input;

	input = input_string(_("Enter host identifier. (e.g., myhost)\n"));
	max_hosts = cfgAllocForNewSection(config_table, input);
	free(input);

	configure_host(max_hosts - 1, NO_DEFAULT);

	return (max_hosts);
}


static void reconfigure_existing_host(int max_hosts)
{
	int i;
	int num;
	char *input;

	for (i = 0; i < max_hosts; i++) {
		printf(_("%d) HostIdentifier: %-10s    HostName: %s\n"), i + 1, cfgSectionNumberToName(i), config.host_name[i]);
	}
	printf("\n");
	input = input_string(_("Choose host you want to reconfigure.\n"));
	num = atoi(input);
	free(input);

	if (num < 1 || num > max_hosts) {
		printf(_("No such host.\n"));
		return;
	}

	configure_host(num - 1, DEFAULT_EXIST);
}


static void print_current_configuration(int max_hosts)
{
	int i;

	for (i = 0; i < max_hosts; i++) {
		printf("[%s]\n", cfgSectionNumberToName(i));
		printf("HostName     = %s\n", config.host_name[i]);
		printf("LoginName    = %s\n", config.login_name[i]);
		printf("Password     = ********\n");
		printf("LocalTopDir  = %s\n", config.local_top_dir[i]);
		printf("RemoteTopDir = %s\n\n", config.remote_top_dir[i]);
	}
}


static void configure_host(int host_num, int hasdefault)
{
	char *input;
	char password[128];
	int i;

	struct {
		char *parameter_name;
		char **variable;
		char *message;
	} question_table[] = {
		{ "HostName",     config.host_name,
		  _("Enter remote host name or IP address. (e.g., ftp.myhost.org, 192.168.0.1)\n") },
		{ "LoginName",    config.login_name,
		  _("Enter login name of the account.\n"), },
		{ "Password",     config.password,
		  _("Enter password granting access to the account.\n"), },
		{ "LocalTopDir",  config.local_top_dir,
		  _("Enter local top directory. It must be specified as absolute path.\n(e.g., /home/foo/www)\n"), },
		{ "RemoteTopDir", config.remote_top_dir,
		  _("Enter remote top directory. It must be specified as absolute path.\n(e.g., /public_html)\n"), },
	};

	for (i = 0; i < sizeof(question_table) / sizeof(question_table[0]); i++) {
		printf(question_table[i].message);

		if (strcmp(question_table[i].parameter_name, "Password") == 0) {
			for (;;) {
				if (hasdefault == DEFAULT_EXIST) {
					printf(_("Just press enter to leave default. (default is: ********)\n"));
				}
				input = getpass(">");
				if (input[0] == '\0') {
					if (hasdefault == DEFAULT_EXIST) {
						printf(_("Not changed.\n"));
						break;
					}
					continue;
				}
				strcpy(password, input);
				printf(_("\nRetype password for confirmation.\n"));
				input = getpass(">");
				if (strcmp(password, input) != 0) {
					printf(_("\nPasswords do not match.\n\n"));
					printf(question_table[i].message);
					continue;
				}
				cfgStoreValue(config_table, question_table[i].parameter_name, input, CFG_INI, host_num);
				break;
			}
		} else {
			if (hasdefault == DEFAULT_EXIST) {
				printf(_("Just press enter to leave default. (default is: %s)\n"), question_table[i].variable[host_num]);
			}
			for (;;) {
#ifdef HAVE_LIBREADLINE
				input = readline(">");
#else
				printf(">");
				input = str_fgets(stdin);
#endif
				if (input[0] != '\0') {
					cfgStoreValue(config_table, question_table[i].parameter_name, input, CFG_INI, host_num);
					break;
				} else {
					free(input);
					if (hasdefault == DEFAULT_EXIST) {
						printf(_("Not changed.\n"));
						break;
					}
				}
			}
		}
		printf("\n");
	}
}


static char *input_string(const char *message)
{
	char *input;

	printf(_(message));
	for (;;) {
#ifdef HAVE_LIBREADLINE
		input = readline(">");
#else
		printf(">");
		input = str_fgets(stdin);
#endif
		if (input[0] != '\0') {
			break;
		}
	}
	printf("\n");

	return (input);
}


static void check_permission(char *file)
{
	struct stat s;
	mode_t mode;

	if (stat(file, &s) != 0){
		perror("stat");
		fprintf(stderr, _("Cannot get file status: %s\n"), file);
		exit(1);
	}

	mode = s.st_mode & 0477;
	if (mode != 0400) {
		fprintf(stderr, _("Warning! `%s' permissions allow other users to read your configuration file.\n"
				  "Set the permissions to 0600.\n\n"), file);
	}
}


static cfgList *copy_cfg_list(const cfgList *src)
{
	cfgList *head;
	cfgList *dest;

	head = dest = str_malloc(sizeof(cfgList));
	dest->str = str_dup(src->str);

	for (src = src->next; src != NULL; src = src->next) {
		dest->next = str_malloc(sizeof(cfgList));
		dest = dest->next;
		dest->str = str_dup(src->str);
	}
	dest->next = NULL;

	return (head);
}
