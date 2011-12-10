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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif  

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "intl.h"
#include "strlib.h"
#include "parsecfg.h"
#include "variable.h"
#include "proto.h"

static cfgStruct weex_config[]={
	/* parameter		type		address of variable */
	{ "HostName",		CFG_STRING,	&host_name		},
	{ "LoginName",		CFG_STRING,	&login_name		},
	{ "AuthorizationName",	CFG_STRING,	&authorization_name	},
	{ "Password",		CFG_STRING,	&password		},
	{ "SrcDir",		CFG_STRING,	&src_dir		},
	{ "DestDir",		CFG_STRING,	&dest_dir		},
	{ "IgnoreLocalDir",	CFG_STRING_LIST,&ignore_local_dir	},
	{ "IgnoreLocalFile",	CFG_STRING_LIST,&ignore_local_file	},
	{ "IgnoreRemoteDir",	CFG_STRING_LIST,&ignore_remote_dir	},
	{ "IgnoreRemoteFile",	CFG_STRING_LIST,&ignore_remote_file	},
	{ "KeepRemoteDir",	CFG_STRING_LIST,&keep_remote_dir	},
	{ "AsciiFile",		CFG_STRING_LIST,&ascii_file		},
	{ "ChangePermission",	CFG_STRING,	&change_permission	},
	{ "ChangePermissionDir",CFG_STRING_LIST,&change_permission_dir	},
	{ "PreservePermissionDir",CFG_STRING_LIST,&preserve_permission_dir},
	{ "ConvToLower",	CFG_BOOL,	&conv_to_lower		},
	{ "OverwriteOK",	CFG_BOOL,	&overwrite_ok		},
	{ "RenameOK",		CFG_BOOL,	&rename_ok		},
	{ "RecordLog",		CFG_BOOL,	&record_log		},
	{ "Silent",		CFG_BOOL,	&cfg_silent		},
	{ "Force",		CFG_BOOL,	&cfg_force		},
        { "Monochrome",         CFG_BOOL,       &cfg_monochrome         },
	{ "FtpPassive",		CFG_BOOL,	&ftp_passive		},
	{ "ShowHiddenFile",	CFG_BOOL,	&show_hidden_file	},
	{ "FollowSymlinks",	CFG_BOOL,	&follow_symlinks	},
	{ "ChdirAtConnection",	CFG_BOOL,	&chdir_at_connection	},
	{ "NestSpaces",		CFG_INT,	&nest_spaces		},
	{ "LogDetailLevel",	CFG_INT,	&log_detail_level	},
	{ "MaxRetryToSend",	CFG_INT,	&max_retry_to_send	},
	{ NULL, CFG_END, NULL },
};


/* --------------------------------------------------
 NAME       config_location
 FUNCTION   where is the configuration file
 INPUT      none
 OUTPUT     return NULL if the configuration file does not exist
            otherwise return location
-------------------------------------------------- */
char *config_location(void)
{
	char *temp;

	temp=str_concat(getenv("HOME"),"/.weex/weexrc",NULL);
	if(access(temp,F_OK)!=0){
		free(temp);
		temp=str_concat(getenv("HOME"),"/.weexrc",NULL);
		if(access(temp,F_OK)!=0){
			free(temp);
			return(NULL);
		}
	}
	return(temp);
}


/* --------------------------------------------------
 NAME       read_config
 FUNCTION   read the configuration file
 INPUT      none
 OUTPUT     the maximum hosts
-------------------------------------------------- */
int read_config(void)
{
	char *temp;
	int max_hosts;

	temp=config_location();
	if(temp==NULL){
		fprintf(stderr,_("Configuration file `%s/.weex/weexrc' does not exist.\n"),getenv("HOME"));
		exit(1);
	}
	max_hosts=cfgParse(temp,weex_config,CFG_INI);

	if(max_hosts==0){
		fprintf(stderr,_("There is no section in configuration file.\n"));
		exit(1);
	}

	set_default(max_hosts);
	check_permission(temp);
	free(temp);

	return(max_hosts);
}


/* --------------------------------------------------
 NAME       make_weex_directory
 FUNCTION   make the directory at home for weex
 INPUT      none
 OUTPUT     none
-------------------------------------------------- */
void make_weex_directory(void)
{
	char *temp;

	temp=str_concat(getenv("HOME"),"/.weex",NULL);
	if(access(temp,F_OK)!=0){
		if(mkdir(temp,0755)!=0){
			fprintf(stderr,_("Cannot make directory `%s'.\n"),temp);
			exit(1);
		}
		fprintf(stderr,_("Created directory `%s'.\n"),temp);
	}
	free(temp);
}


/* --------------------------------------------------
 NAME       check_permission
 FUNCTION   check permission of file and warn
 INPUT      file ... the name of checked file
 OUTPUT     none
-------------------------------------------------- */
void check_permission(char *file)
{
	struct stat s;
	mode_t mode;

	if(stat(file,&s)!=0){
		fprintf(stderr,_("Cannot get file status: `%s'\n"),file);
		exit(1);
	}

	mode=s.st_mode & 0477;
	if(mode!=0400){
		fprintf(stderr,"Warning! Set `%s' permissions to 600.\n\n",file);
	}
}


/* --------------------------------------------------
 NAME       set_default
 FUNCTION   set the default value
 INPUT      max_hosts ... the maximum number of hosts
 OUTPUT     none
-------------------------------------------------- */
void set_default(int max_hosts)
{
	int i;
	int default_num;

	default_num=cfgSectionNameToNumber("default");
	if(default_num==-1){
		fprintf(stderr,_("A `default' section is required.\n"));
		exit(1);
	}
	for(i=0;i<max_hosts;i++){
		set_str_default(i,default_num,login_name);
		set_str_default(i,default_num,authorization_name);
		set_str_default(i,default_num,password);
		set_str_default(i,default_num,src_dir);
		set_str_default(i,default_num,dest_dir);
		set_str_default(i,default_num,change_permission);

		set_list_default(i,default_num,ignore_local_dir);
		set_list_default(i,default_num,ignore_local_file);
		set_list_default(i,default_num,ignore_remote_dir);
		set_list_default(i,default_num,ignore_remote_file);
		set_list_default(i,default_num,keep_remote_dir);
		set_list_default(i,default_num,ascii_file);
		set_list_default(i,default_num,change_permission_dir);
		set_list_default(i,default_num,preserve_permission_dir);

		set_bool_default(i,default_num,0,conv_to_lower);		/*                    */
		set_bool_default(i,default_num,0,cfg_silent);			/*                    */
		set_bool_default(i,default_num,0,cfg_force);			/* default is `false' */
		set_bool_default(i,default_num,0,cfg_monochrome);		/*                    */
		set_bool_default(i,default_num,0,show_hidden_file);		/*                    */
		set_bool_default(i,default_num,0,follow_symlinks);		/*                    */
		set_bool_default(i,default_num,0,rename_ok);			/*                    */

		set_bool_default(i,default_num,1,overwrite_ok);		/*                    */
		set_bool_default(i,default_num,1,record_log);		/*  default is `true' */
		set_bool_default(i,default_num,1,ftp_passive);		/*                    */
		set_bool_default(i,default_num,1,chdir_at_connection);	/*                    */

		set_int_default(i,default_num,4,nest_spaces);
		set_int_default(i,default_num,1,log_detail_level);
		set_int_default(i,default_num,8,max_retry_to_send);

		/* attach '/' to the trailing in directory names */
		str_attach_slash_to_trailing(i,src_dir);
		str_attach_slash_to_trailing(i,dest_dir);
		list_attach_slash_to_trailing(i,ignore_local_dir);
		list_attach_slash_to_trailing(i,ignore_remote_dir);
		list_attach_slash_to_trailing(i,keep_remote_dir);
		list_attach_slash_to_trailing(i,change_permission_dir);
		list_attach_slash_to_trailing(i,preserve_permission_dir);

		/* for relative-path */
		absolutize_path(i,src_dir[i],ignore_local_dir);
		absolutize_path(i,dest_dir[i],ignore_remote_dir);
		absolutize_path(i,dest_dir[i],keep_remote_dir);
		absolutize_path(i,src_dir[i],change_permission_dir);
		absolutize_path(i,src_dir[i],preserve_permission_dir);
		absolutize_file(i,src_dir[i],ignore_local_file);
		absolutize_file(i,dest_dir[i],ignore_remote_file);
		absolutize_file(i,src_dir[i],ascii_file);

		/* check PermissionDir */
		if(change_permission_dir[i]!=NULL && change_permission[i]==NULL){
			fprintf(stderr,_("When you set ChangePermissionDir, you must configure ChangePermission.\n"));
			exit(1);
		}
	}
}


/* --------------------------------------------------
 NAME       set_str_default
 FUNCTION   set the default value of string variable
 INPUT      host .......... host number to set
            default_num ... host number of default
            str_pointer ... pointer to string
 OUTPUT     none
-------------------------------------------------- */
void set_str_default(int host,int default_num,char **str_pointer)
{
	if(str_pointer[host] == NULL) {
		str_pointer[host] = str_dup(str_pointer[default_num]);
	}
}


/* --------------------------------------------------
 NAME       set_list_default
 FUNCTION   set the default value of linked-list variable
 INPUT      host .......... host number to set
            default_num ... host number of default
            list_pointer .. pointer to linked-list
 OUTPUT     none
-------------------------------------------------- */
void set_list_default(int host,int default_num,cfgList **list_pointer)
{
	cfgList *head;
	cfgList *src;
	cfgList *dest;

	src = list_pointer[default_num];

	if (list_pointer[host] != NULL || src == NULL) {
		return;
	}

	head = dest = str_malloc(sizeof(cfgList));
	dest->str = str_dup(src->str);

	for (src = src->next; src != NULL; src = src->next) {
		dest->next = str_malloc(sizeof(cfgList));
		dest = dest->next;
		dest->str = str_dup(src->str);
	}
	dest->next = NULL;

	list_pointer[host] = head;
}


/* --------------------------------------------------
 NAME       set_bool_default
 FUNCTION   set the default value of boolean variable
 INPUT      host ........... host number to set
            default_num .... host number of default
            bool_default ... default value (both `host' and `default_num'
                             are unconfigured)
            bool_pointer ... pointer to boolean
 OUTPUT     none
-------------------------------------------------- */
void set_bool_default(int host,int default_num,int bool_default,int *bool_pointer)
{
	if(bool_pointer[host]==-1){
		bool_pointer[host]=(bool_pointer[default_num]==-1) ? bool_default : bool_pointer[default_num];
	}
}


/* --------------------------------------------------
 NAME       set_int_default
 FUNCTION   set the default value of integer variable
            if negative-number, set 0
 INPUT      host .......... host number to set
            default_num ... host number of default
            default_val ... default_val (both `host' and `default_num'
                            are unconfigured)
            int_pointer ... pointer to integer
 OUTPUT     none
-------------------------------------------------- */
void set_int_default(int host,int default_num,int default_val,int *int_pointer)
{
	if(int_pointer[host]==0){
		if(int_pointer[default_num]==0){
			int_pointer[host]=default_val;
		}else{
			int_pointer[host]=(int_pointer[host]<0) ? 0 : int_pointer[default_num];
		}
	}
}


/* --------------------------------------------------
 NAME       str_attach_slash_to_trailing
 FUNCTION   attach '/' to the trailing in string
 INPUT      host .......... host number
            str_pointer ... pointer to string
 OUTPUT     none
-------------------------------------------------- */
void str_attach_slash_to_trailing(int host,char **str_pointer)
{
	if(str_pointer[host]==NULL){
		return;
	}
	if(*(str_pointer[host]+strlen(str_pointer[host])-1)!='/'){
		str_pointer[host]=str_realloc(str_pointer[host],strlen(str_pointer[host])+2);
		strcat(str_pointer[host],"/");
	}
}


/* --------------------------------------------------
 NAME       list_attach_slash_to_trailing
 FUNCTION   attach '/' to the trailing in linked-list string
 INPUT      host ...... host number
            list_pointer ... pointer to linked-list
 OUTPUT     none
-------------------------------------------------- */
void list_attach_slash_to_trailing(int host,cfgList **list_pointer)
{
	cfgList *l;

	if(list_pointer[host]==NULL){
		return;
	}
	for(l=list_pointer[host];l!=NULL;l=l->next){
		if(*(l->str+strlen(l->str)-1)!='/'){
			l->str=str_realloc(l->str,strlen(l->str)+2);
			strcat(l->str,"/");
		}
	}
}


/* --------------------------------------------------
 NAME       absolutize_path
 FUNCTION   absolutize a directory
 INPUT      host ... host number
            base_dir ... base directory
            list_absolute_path ... pointer to linked-list stored absolute path
 OUTPUT     none
-------------------------------------------------- */
void absolutize_path(int host,char *base_dir,cfgList **list_absolute_path)
{
	cfgList *l;
	char *temp;

	if(list_absolute_path[host]==NULL){
		return;
	}
	for(l=list_absolute_path[host];l!=NULL;l=l->next){
		if(strcmp(l->str,"./")==0){
			temp=l->str;
			l->str=str_dup(base_dir);
			free(temp);
		}else if(*(l->str)!='/'){
			temp=l->str;
			l->str=str_concat(base_dir,temp,NULL);
			free(temp);
		}
	}
}


/* --------------------------------------------------
 NAME       absolutize_file
 FUNCTION   absolutize a file with directory
 INPUT      host ... host number
            base_dir ... base directory
            list_absolute_file ... pointer to linked-list stored absolute path
 OUTPUT     none
-------------------------------------------------- */
void absolutize_file(int host,char *base_dir,cfgList **list_absolute_file)
{
	cfgList *l;
	char *temp;

	if(list_absolute_file[host]==NULL){
		return;
	}
	for(l=list_absolute_file[host];l!=NULL;l=l->next){
		if(strchr(l->str,'/')==NULL){	/* just a file (without directory) */
			continue;
		}
		if(*(l->str)!='/'){
			temp=l->str;
			if(strncmp(temp,"./",2)==0){
				l->str=str_concat(base_dir,temp+2,NULL);
			}else{
				l->str=str_concat(base_dir,temp,NULL);
			}
			free(temp);
		}
	}
}
