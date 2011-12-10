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

#define GLOBAL_VALUE_DEFINE

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif  
      
#include <stdio.h>
#include <stdlib.h>

#ifdef ENABLE_NLS
#  include <locale.h>
#endif

#include "intl.h"
#include "ftplib.h"
#include "shhopt.h"
#include "variable.h"
#include "proto.h"


int main(int argc,char *argv[])
{
	int max_hosts;
	int put_config_flag=0;
	optStruct opt[]={
		/* short long          type      var/func	         special */
		{ 'h', "help",         OPT_FLAG, put_usage,        OPT_CALLFUNC },
		{ 'V', "version",      OPT_FLAG, put_version,      OPT_CALLFUNC },
		{ 's', "silent",       OPT_FLAG, &opt_silent,      0            },
		{ 'f', "force",        OPT_FLAG, &opt_force,       0            },
		{ 'm', "monochrome",   OPT_FLAG, &opt_monochrome,  0            },
		{ 't', "test",         OPT_FLAG, &opt_test,        0            },
		{ 'r', "rebuild-cache",OPT_FLAG, &rebuild_cache,   0            },
		{ 'd', "debug-config", OPT_FLAG, &put_config_flag, 0            },
		{ 'D', "debug-ftplib", OPT_FLAG, &ftplib_debug,    0            },
		{ 0, 0, OPT_END, 0, 0 }  /* no more options */
	};

#ifdef ENABLE_NLS
	setlocale(LC_ALL,"");
	bindtextdomain(PACKAGE,LOCALEDIR);
	textdomain(PACKAGE);
#endif

	make_weex_directory();

	optParseOptions(&argc,argv,opt,0);

	ftplib_debug*=3;
	if(put_config_flag){
		max_hosts=read_config();
		put_config(max_hosts);
		exit(0);
	}

	if(argc<2){
		put_usage();
	}

	max_hosts=read_config();

	ftp_main(argc,argv,max_hosts);

	return(0);
}


/* --------------------------------------------------
 NAME       put_usage
 FUNCTION   output usage and exit
 INPUT      none
 OUTPUT     never return
-------------------------------------------------- */
void put_usage(void)
{
	char *temp;

	temp=config_location();
	if(temp==NULL){
		fprintf(stderr,_("Warning! Configuration file `%s/.weex/weexrc' does not exist.\n\n"),getenv("HOME"));
	}else{
		check_permission(temp);
	}

	printf(_("\
Fast Webpage Exchanger Ver %s Copyright (C) 1999-2000 Yuuki NINOMIYA\n\n\
This is free software with ABSOLUTELY NO WARRANTY.\n\
For details please see the file COPYING.\n\n\
usage: weex [OPTIONS] HOST...\n\
\n\
options:\n\
  -h | --help           display this help and exit\n\
  -s | --silent         suppress normal non-error messages\n\
  -f | --force          force sending all files\n\
  -m | --monochrome     output messages in monochrome\n\
  -t | --test           not modify remote files\n\
  -r | --rebuild-cache  rebuild cache file\n\
  -d | --debug-config   display configuration of each hosts\n\
  -D | --debug-ftplib   output debugging information for ftplib\n\
  -V | --version        display version information and exit\n\
\n"),VERSION);

	exit(0);
}


/* --------------------------------------------------
 NAME       put_version
 FUNCTION   output version information and exit
 INPUT      none
 OUTPUT     never return
-------------------------------------------------- */
void put_version(void)
{
	char *temp;

	temp=config_location();
	if(temp==NULL){
		fprintf(stderr,_("Warning! Configuration file `%s/.weex/weexrc' does not exist.\n\n"),getenv("HOME"));
	}else{
		check_permission(temp);
	}

	printf(_("Fast Webpage Exchanger Ver %s Copyright (C) 1999-2000 Yuuki NINOMIYA\n"),VERSION);
	exit(0);
}


/* --------------------------------------------------
 NAME       put_config
 FUNCTION   output the configuration
 INPUT      max_hosts ... the maximum number of hosts
 OUTPUT     none
-------------------------------------------------- */
void put_config(int max_hosts)
{
	int i;
	cfgList *l;

	for(i=0;i<max_hosts;i++){
		printf("[%s]\n",cfgSectionNumberToName(i));
		printf("HostName               %s\n",host_name[i]==NULL ? "(NULL)" : host_name[i]);
		printf("LoginName              %s\n",login_name[i]==NULL ? "(NULL)" : login_name[i]);
		printf("Password               %s\n",password[i]==NULL ? "(NULL)" : password[i]);
		printf("AuthorizationName      %s\n",authorization_name[i]==NULL ? "(NULL)" : authorization_name[i]);
		printf("SrcDir                 %s\n",src_dir[i]==NULL ? "(NULL)" : src_dir[i]);
		printf("DestDir                %s\n",dest_dir[i]==NULL ? "(NULL)" : dest_dir[i]);
		if(ignore_local_dir[i]!=NULL){
			for(l=ignore_local_dir[i];l!=NULL;l=l->next){
				printf("IgnoreLocalDir         %s\n",l->str);
			}
		}else{
				printf("IgnoreLocalDir         (NULL)\n");
		}
		if(ignore_local_file[i]!=NULL){
			for(l=ignore_local_file[i];l!=NULL;l=l->next){
				printf("IgnoreLocalFile        %s\n",l->str);
			}
		}else{
				printf("IgnoreLocalFile        (NULL)\n");
		}
		if(ignore_remote_dir[i]!=NULL){
			for(l=ignore_remote_dir[i];l!=NULL;l=l->next){
				printf("IgnoreRemoteDir        %s\n",l->str);
			}
		}else{
				printf("IgnoreRemoteDir        (NULL)\n");
		}
		if(ignore_remote_file[i]!=NULL){
			for(l=ignore_remote_file[i];l!=NULL;l=l->next){
				printf("IgnoreRemoteFile       %s\n",l->str);
			}
		}else{
				printf("IgnoreRemoteFile       (NULL)\n");
		}
		if(ascii_file[i]!=NULL){
			for(l=ascii_file[i];l!=NULL;l=l->next){
				printf("AsciiFile              %s\n",l->str);
			}
		}else{
				printf("AsciiFile              (NULL)\n");
		}
		if(keep_remote_dir[i]!=NULL){
			for(l=keep_remote_dir[i];l!=NULL;l=l->next){
				printf("KeepRemoteDir          %s\n",l->str);
			}
		}else{
				printf("KeepRemoteDir          (NULL)\n");
		}
		if(change_permission_dir[i]!=NULL){
			for(l=change_permission_dir[i];l!=NULL;l=l->next){
				printf("ChangePermissionDir    %s\n",l->str);
			}
		}else{
				printf("ChangePermissionDir    (NULL)\n");
		}
		printf("ChangePermission       %s\n",change_permission[i]==NULL ? "(NULL)" : change_permission[i]);
		if(preserve_permission_dir[i]!=NULL){
			for(l=preserve_permission_dir[i];l!=NULL;l=l->next){
				printf("PreservePermissionDir  %s\n",l->str);
			}
		}else{
				printf("PreservePermissionDir  (NULL)\n");
		}
		printf("ConvToLower            %s\n",conv_to_lower[i] ? "True" : "False");
		printf("OverwriteOK            %s\n",overwrite_ok[i] ? "True" : "False");
		printf("RenameOK               %s\n",rename_ok[i] ? "True" : "False");
		printf("RecordLog              %s\n",record_log[i] ? "True" : "False");
		printf("Silent                 %s\n",cfg_silent[i] ? "True" : "False");
		printf("Force                  %s\n",cfg_force[i] ? "True" : "False");
		printf("Monochrome             %s\n",cfg_monochrome[i] ? "True" : "False");
		printf("FtpPassive             %s\n",ftp_passive[i] ? "True" : "False");
		printf("ShowHiddenFile         %s\n",show_hidden_file[i] ? "True" : "False");
		printf("NestSpaces             %d\n",nest_spaces[i]);
		printf("LogDetailLevel         %d\n",log_detail_level[i]);
		printf("MaxRetryToSend         %d\n\n",max_retry_to_send[i]);
	}
}

