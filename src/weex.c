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

/* $Id: weex.c,v 1.2 2004/03/22 05:57:19 gmkun Exp $ */

#define GLOBAL_VALUE_DEFINE

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef ENABLE_NLS
#  include <locale.h>
#endif

#include "intl.h"
#include "ftplib.h"
#include "shhopt.h"
#include "strlib.h"
#include "variable.h"
#include "proto.h"


static void put_usage_and_exit(void);
static void put_version_and_exit(void);
static void make_weex_directory(void);


static optStruct option_table[] = {
	/* short long               type        var/func                          special */
	{ 'a', "all-hosts",         OPT_FLAG,   &command_line_option.all_hosts,         0 },
	{ 'c', "config-file",       OPT_STRING, &command_line_option.config_file,       0 },
	{ 'q', "quiet",             OPT_FLAG,   &command_line_option.quiet,             0 },
	{ 'p', "suppress-progress", OPT_FLAG,   &command_line_option.suppress_progress, 0 },
	{ 'f', "force",             OPT_FLAG,   &command_line_option.force,             0 },
	{ 'm', "monochrome",        OPT_FLAG,   &command_line_option.monochrome,        0 },
	{ 'r', "rebuild-cache",     OPT_FLAG,   &command_line_option.rebuild_cache,     0 },
	{ 'R', "catch-up",          OPT_FLAG,   &command_line_option.catch_up,          0 },
	{ 'w', "not-mkdir-weex",    OPT_FLAG,   &command_line_option.not_mkdir_weex,    0 },
	{ 'C', "confirmation",      OPT_FLAG,   &command_line_option.confirmation,      0 },
	{ 's', "simulate",          OPT_FLAG,   &command_line_option.simulate,          0 },
	{ 'l', "list",              OPT_FLAG,   &command_line_option.list,              0 },
	{ 'n', "nlist",             OPT_FLAG,   &command_line_option.nlist,             0 },
	{ 'v', "view",              OPT_FLAG,   &command_line_option.view,              0 },
	{ 'd', "download",          OPT_FLAG,   &command_line_option.download,          0 },
	{ 'u', "upload",            OPT_FLAG,   &command_line_option.upload,            0 },
	{ 'g', "debug",             OPT_FLAG,   &command_line_option.debug,             0 },
	{ 'h', "help",              OPT_FLAG,   put_usage_and_exit,          OPT_CALLFUNC },
	{ 'V', "version",           OPT_FLAG,   put_version_and_exit,        OPT_CALLFUNC },
	{ 0, 0, OPT_END, 0, 0 }
};


/* --- MAIN FUNCTION --- */

int main(int argc, char *argv[])
{
	int max_hosts;

#ifdef ENABLE_NLS
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
#endif

	optParseOptions(&argc, argv, option_table, 0);

	if (!command_line_option.not_mkdir_weex) {
		make_weex_directory();
	}

	if (command_line_option.debug) {
		ftplib_debug = 3;
	}

	if (command_line_option.config_file == NULL) {
		command_line_option.config_file = str_concat(getenv("HOME"), "/", DEFAULT_CONFIG_DIR, "/", DEFAULT_CONFIG_FILE, NULL);
	}

	max_hosts = parse_config_file();

	if (command_line_option.view) {
		display_host_definition(argc, argv, max_hosts);
		exit(0);
	}

	if (argc < 2 && !command_line_option.all_hosts) {	/* No host is specified. */
		put_usage_and_exit();
	}

	main_loop(argc, argv, max_hosts);

	return (0);
}


/* --- PUBLIC FUNCTIONS --- */

void internal_error(const char *file, int line)
{
	fprintf(stderr, _("Internal error has occurred in %s at line %d.\n"), file, line);
	fprintf(stderr, _("It must be a bug. Please send a bug report to the author.\n"));
	exit(1);
}


void minor_error(int host)
{
	if (config.exit_on_minor_error[host]) {
		disconnect_from_remote_host(host);
		exit(1);
	}
}


void fatal_error(int host)
{
	disconnect_from_remote_host(host);
	exit(1);
}


/* --- PRIVATE FUNCTIONS --- */

static void put_usage_and_exit(void)
{
	printf(_("weex version %s Copyright (C) 1999-2001 Yuuki NINOMIYA <gm@debian.or.jp>\n\n"), VERSION);
	printf(_("This program comes with ABSOLUTELY NO WARRANTY, to the extent permitted by law.\n"
		 "You may redistribute it under the terms of the GNU General Public License.\n"
		 "For more information about these matters, see the file named COPYING.\n\n"));
	printf(_("usage: weex [OPTIONS] HOST...\n\n"));
	printf(_("options:\n"));
	printf(_("  -c, --config-file=FILE   use an alternate configuration file\n"));
	printf(_("  -w, --not-mkdir-weex     not make ~/.weex directory on startup\n"));
	printf(_("  -v, --view               display the host difinitions\n"));
	printf(_("  -l, --list               display a listing of files needing update\n"));
	printf(_("  -n, --nlist              display the number of files needing update\n"));
	printf(_("  -r, --rebuild-cache      rebuild a cache file\n"));
	printf(_("  -R, --catch-up           rebuild a cache file as all files are updated\n"));
	printf(_("  -u, --upload             transfer files from local to remote\n"));
	printf(_("  -d, --download           transfer files from remote to local\n"));
	printf(_("  -a, --all-hosts          process all defined hosts\n"));
//	printf(_("  -C, --confirmation       request confirmation when change files\n"));
	printf(_("  -f, --force              transfer all files whether updated or not\n"));
//	printf(_("  -s, --simulate           perform ordering simulation (not actually change)\n"));
	printf(_("  -q, --quiet              suppress non-error messages\n"));
	printf(_("  -p, --suppress-progress  suppress progress information\n"));
	printf(_("  -m, --monochrome         output messages in monochromee\n"));
	printf(_("  -g, --debug              enable debug output\n"));
	printf(_("  -h, --help               display this help and exit\n"));
	printf(_("  -V, --version            display version information and exit\n"));
	printf("\n");

	exit(0);
}


static void put_version_and_exit(void)
{
	printf(_("weex %s\nCopyright (C) 1999-2001 Yuuki NINOMIYA <gm@debian.or.jp>\n\n"), VERSION);
	printf(_("This program comes with ABSOLUTELY NO WARRANTY, to the extent permitted by law.\n"
		 "You may redistribute it under the terms of the GNU General Public License.\n"
		 "For more information about these matters, see the file named COPYING.\n\n"));

	exit(0);
}


static void make_weex_directory(void)
{
	char *temp;

	temp = str_concat(getenv("HOME"), "/.weex", NULL);
	if (access(temp, X_OK) != 0) {
		if (mkdir(temp, 0755) != 0) {
			perror("mkdir");
			fprintf(stderr, _("Cannot make directory `%s'.\n"), temp);
			exit(1);
		}
		fprintf(stderr, _("Directory `%s' has been made.\n"), temp);
	}
	free(temp);
}
