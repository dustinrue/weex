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
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <sys/stat.h>
#include "intl.h"
#include "strlib.h"
#include "variable.h"
#include "proto.h"

#ifndef PATH_MAX
#  define PATH_MAX 512
#endif /* PATH_MAX */


/* --------------------------------------------------
 NAME       get_local_file_data
 FUNCTION   get the data of local files
 INPUT      local_data ... pointer to stored date
 OUTPUT     the maximum number of local files
-------------------------------------------------- */
int get_local_file_data(FileData **local_data)
{
	DIR *dir;
	struct dirent *ent;
	struct stat file_stat;
	struct tm *ftime;
	char time_temp[10];
	char date_temp[10];
	int isdir;
	int file_num=0;
	int i;

	dir=opendir(".");
	while((ent=readdir(dir))!=NULL){
		if(strcmp(ent->d_name,".")==0 || strcmp(ent->d_name,"..")==0){
			continue;
		}
		lstat(ent->d_name,&file_stat);

		/* follow symlinks */
		if (follow_symlinks[host_number] && (file_stat.st_mode & S_IFLNK)==S_IFLNK){
			char realfname[PATH_MAX];

			DebugPrint((stderr,"Local symlink is detected: %s\n",ent->d_name));
			if(realpath(ent->d_name,realfname)==NULL){
				char *temp;

				fprintf(stderr,_("Cannot expand symbolic link `%s'. Ignore this file.\n"),ent->d_name);
				temp=str_dup_printf(_("Expanding symbolic link failed: %s"),ent->d_name);
				log_write(temp);
				free(temp);
				continue;
			}
			stat(realfname,&file_stat);
			DebugPrint((stderr,"Realfname is %s\n",realfname));
		}
		isdir = ((file_stat.st_mode & S_IFDIR) == S_IFDIR);
		ftime=gmtime(&file_stat.st_mtime);

		if(isdir){
			if(is_ignore_dir(ent->d_name,LOCAL)){
				DebugPrint((stderr,"Ignore local dir: %s\n",ent->d_name));
				continue;
			}
		}else{
			if(is_ignore_file(ent->d_name,LOCAL)){
				DebugPrint((stderr,"Ignore local file: %s\n",ent->d_name));
				continue;
			}
		}

		sprintf(date_temp,"%04d%02d%02d",1900+ftime->tm_year,ftime->tm_mon+1,ftime->tm_mday);
		sprintf(time_temp,"%02d%02d%02d",ftime->tm_hour,ftime->tm_min,ftime->tm_sec);

		*local_data=str_realloc(*local_data,sizeof(**local_data)*(file_num+1));
		(*local_data)[file_num].date=atol(date_temp);
		(*local_data)[file_num].time=atol(time_temp);
		(*local_data)[file_num].isdir=isdir;
		(*local_data)[file_num].name=str_dup(ent->d_name);

		DebugPrint((stderr,"Local: [%08ld] [%06ld] [%08o] [%d] [%s]\n",(*local_data)[file_num].date,(*local_data)[file_num].time,file_stat.st_mode,(*local_data)[file_num].isdir,(*local_data)[file_num].name));

		if(conv_to_lower[host_number]){
			for(i=0;i<file_num;i++){
				if(strcasecmp((*local_data)[file_num].name,(*local_data)[i].name)==0){
					char *temp;

					fprintf(stderr,_("ConvToLower causes a conflict between `%s' and `%s'. Proceed anyway.\n"),(*local_data)[i].name,(*local_data)[file_num].name);

					temp=str_dup_printf(_("Confliction by ConvToLower: %s"),(*local_data)[i].name);
					log_write(temp);
					free(temp);
				}
			}
		}

		file_num++;
	}
	closedir(dir);
	return(file_num);
}


/* --------------------------------------------------
 NAME       get_remote_file_data
 FUNCTION   get the data of remote files
 INPUT      remote_data ... pointer to stored date
 OUTPUT     the maximum number of remote files
-------------------------------------------------- */
int get_remote_file_data(FileData **remote_data)
{
	char list_temp[32];
	int  fd;
	FILE *fp;
	char *fgets_temp;
	char *file_name;
	long date,time;
	char mod_temp[20];
	char dir_flag;
	char *temp;
	int *add_cache_num=NULL;
	int add_cache_files=0;
	int file_num=0;
	int i;

	if(strcmp(current_dir[REMOTE],dest_dir[host_number])!=0){
		change_dir_actually(REMOTE);
	}
	strcpy(list_temp, "/tmp/weexXXXXXX");
	fd=mkstemp(list_temp);
	if(fd==-1){
		fprintf(stderr,_("Cannot create a unique file name.\n"));
		ftp_disconnect();
		exit(1);
	}

	if((show_hidden_file[host_number] && FtpDir(list_temp,"-a",ftp_buf)==0) ||
	  (!show_hidden_file[host_number] && FtpDir(list_temp,NULL,ftp_buf)==0)){
		fprintf(stderr,_("Cannot get remote directory list.\n"));
		log_write(_("Getting remote directory list failed"));
		ftp_disconnect();
		exit(1);
	}
	fp=fdopen(fd,"r");
	if(fp==NULL){
		fprintf(stderr,_("Cannot open `%s'.\n"),list_temp);
		ftp_disconnect();
		exit(1);
	}

	while((fgets_temp=str_fgets(fp))!=NULL){
		if(strncmp(fgets_temp,"total",5)==0){
			free(fgets_temp);
			continue;
		}

		DebugPrint((stderr,"Remote: %s\n",fgets_temp));

		dir_flag=parse_file_and_filetype(fgets_temp,&file_name);

		free(fgets_temp);

		/*
		  Test to which this entry is directory or file.
		  This way is very dirty and slow.
		*/
		if(dir_flag=='l'){
			DebugPrint((stderr,"Remote symlink is detected: %s\n",file_name));
			dir_flag=FtpChdir(file_name,ftp_buf) ? 'd' : '-';
			FtpChdir(current_dir[REMOTE], ftp_buf);
			DebugPrint((stderr,"Symlink type is '%c'\n",dir_flag));
		}

		if(strcmp(file_name,".")==0 ||
		   strcmp(file_name,"..")==0 ||
		   (dir_flag=='d' && is_ignore_dir(file_name,REMOTE)) ||
		   (dir_flag!='d' && is_ignore_file(file_name,REMOTE)) ){
			DebugPrint((stderr,"[%c] Ignore remote `%s'\n",dir_flag,file_name));
			free(file_name);
			continue;
		}

		date=time=0;
		if(dir_flag!='d'){
			if(conv_to_lower[host_number]){
				temp=str_tolower(file_name);
				DebugPrint((stderr,"conv_to_lower from `%s' to `%s'\n",file_name,temp));
				free(file_name);
				file_name=temp;
			}
			if(get_cache(file_name,&date,&time)==-1){
				DebugPrint((stderr,"No cache: %s\n",file_name));
				if(FtpModDate(file_name,mod_temp,20,ftp_buf)==1){
					date=n_atol(mod_temp,8);
					time=n_atol(mod_temp+8,6);
					add_cache_num=str_realloc(add_cache_num,sizeof(*add_cache_num)*(add_cache_files+1));
					add_cache_num[add_cache_files++]=file_num;
				}
			}

		}

		*remote_data=str_realloc(*remote_data,sizeof(**remote_data)*(file_num+1));
		(*remote_data)[file_num].date=date;
		(*remote_data)[file_num].time=time;
		(*remote_data)[file_num].isdir=(dir_flag=='d');
		(*remote_data)[file_num].name=file_name;

		DebugPrint((stderr,"Remote: [%08ld] [%06ld] [%c] [%d] [%s]\n",(*remote_data)[file_num].date,(*remote_data)[file_num].time,dir_flag,(*remote_data)[file_num].isdir,(*remote_data)[file_num].name));

		file_num++;
	}

	for(i=0;i<add_cache_files;i++){
		update_cache((*remote_data)[add_cache_num[i]].name,(*remote_data)[add_cache_num[i]].date,(*remote_data)[add_cache_num[i]].time);
		DebugPrint((stderr,"Update cache `%s' : %ld %ld\n",(*remote_data)[add_cache_num[i]].name,(*remote_data)[add_cache_num[i]].date,(*remote_data)[add_cache_num[i]].time));
	}

	fclose(fp);
	if(remove(list_temp)==-1){
		fprintf(stderr,_("Cannot remove `%s'.\n"),list_temp);
		ftp_disconnect();
		exit(1);
	}
	free(add_cache_num);
	/* free(list_temp); */
	return(file_num);
}


/* --------------------------------------------------
 NAME       free_file_data
 FUNCTION   release memory stored file data
 INPUT      file_data ... file data
            max_file_data ... the maximum number of file data
 OUTPUT     none
-------------------------------------------------- */
void free_file_data(FileData *file_data,int max_file_data)
{
	int i;

	for(i=0;i<max_file_data;i++){
		free(file_data[i].name);
	}
	free(file_data);
}


/* --------------------------------------------------
 NAME       parse_file_and_filetype
 FUNCTION   parse file and filetype (whether dir) from file list
 INPUT      str .... file list (a single line)
            file ... pointer to stored file name
 OUTPUT     dir_flag ... directory is 'd'
                         symlink is 'l'
                         others are '-'
                         return -1 if error
-------------------------------------------------- */
int parse_file_and_filetype(char *str,char **file)
{
	char dir_flag;
	int i;
	char *ptr;
	char *lnktmp;

	ptr=strtok(str," ");
	if(ptr==NULL){
		fprintf(stderr,_("Cannot get the remote directory list correctly.\n"));
		ftp_disconnect();
		exit(1);
	}
	if(isdigit(*ptr) && isdigit(*(ptr+1)) && *(ptr+2)=='-'){	/* Remote system type is Windows_NT */
		for(i=0;i<2;i++){
			if((ptr=strtok(NULL," "))==NULL){
				fprintf(stderr,_("Cannot get the remote directory list correctly.\n"));
				ftp_disconnect();
				exit(1);
			}
		}
		dir_flag=(strcmp(ptr,"<DIR>")==0) ? 'd' : '-';
	}else{	/* Remote system type is UNIX or MACOS */
		dir_flag=str[0];

		for(i=0;i<7;i++){
			if((ptr=strtok(NULL," "))==NULL){
				fprintf(stderr,_("Cannot get the remote directory list correctly.\n"));
				ftp_disconnect();
				exit(1);
			}
			if((i==0 && strcmp(ptr,"folder")==0) ||
			   (i==1 && strcmp(ptr,"0000/0000")==0) ||
			   (i==3 && isupper(*ptr) && islower(*(ptr+1)) && islower(*(ptr+2)) && *(ptr+3)=='\0')){
				i++;
			}
		}
	}
	ptr=ptr+strlen(ptr)+1;
	if((lnktmp=strstr(ptr," -> "))!=NULL){	/* This file is a symbolic link. */
		*lnktmp='\0';
	}

	/* Strip leading spaces from the filename which can occur
	   with an FTP server based on Windows NT */
	while (*ptr == ' '){
		ptr++;
	}

	*file=str_dup(ptr);

	return(dir_flag);
}


/* --------------------------------------------------
 NAME       is_ignore_dir
 FUNCTION   whether ???IgnoreDir or not
 INPUT      dir_name ... name of the directory
            side ....... LOCAL or REMOTE
 OUTPUT     return 1 if match ???IgnoreDir otherwise return 0
-------------------------------------------------- */
int is_ignore_dir(char *dir_name,LocalOrRemote side)
{
	cfgList *l;
	char *temp;

	if((side==LOCAL && ignore_local_dir[host_number]==NULL) || (side==REMOTE && ignore_remote_dir[host_number]==NULL)){
		return(0);
	}
	temp=str_concat(current_dir[side],dir_name,"/",NULL);
	if(side==LOCAL){
		for(l=ignore_local_dir[host_number];l!=NULL;l=l->next){
			if(cmp_file_with_wildcard(temp,l->str)==0){
				free(temp);
				return(1);
			}
		}
	}else if(side==REMOTE){
		for(l=ignore_remote_dir[host_number];l!=NULL;l=l->next){
			if(cmp_file_with_wildcard(temp,l->str)==0){
				free(temp);
				return(1);
			}
		}
	}
	free(temp);
	return(0);
}


/* --------------------------------------------------
 NAME       is_ignore_file
 FUNCTION   whether ???IgnoreFile or not
 INPUT      file_name ... file name
            side ........ LOCAL or REMOTE
 OUTPUT     return 1 if match ???IgnoreFile otherwise return 0
-------------------------------------------------- */
int is_ignore_file(char *file_name,LocalOrRemote side)
{
	cfgList *l;
	char *temp;

	if((side==LOCAL && ignore_local_file[host_number]==NULL) || (side==REMOTE && ignore_remote_file[host_number]==NULL)){
		return(0);
	}
	temp=str_concat(current_dir[side],file_name,NULL);
	if(side==LOCAL){
		for(l=ignore_local_file[host_number];l!=NULL;l=l->next){
			if(strchr(l->str,'/')==NULL){
				if(cmp_file_with_wildcard(file_name,l->str)==0){
					free(temp);
					return(1);
				}
			}else{
				if(cmp_file_with_wildcard(temp,l->str)==0){
					free(temp);
					return(1);
				}
			}
		}
	}else if(side==REMOTE){
		for(l=ignore_remote_file[host_number];l!=NULL;l=l->next){
			if(strchr(l->str,'/')==NULL){
				if(cmp_file_with_wildcard(file_name,l->str)==0){
					free(temp);
					return(1);
				}
			}else{
				if(cmp_file_with_wildcard(temp,l->str)==0){
					free(temp);
					return(1);
				}
			}
		}
	}
	free(temp);
	return(0);
}


/* --------------------------------------------------
 NAME       is_ascii_file
 FUNCTION   whether AsciiFile or not
 INPUT      file_name ... file name
 OUTPUT     return 1 if match AsciiFile otherwise return 0
-------------------------------------------------- */
int is_ascii_file(char *file_name)
{
	cfgList *l;
	char *temp;

	if(ascii_file[host_number]==NULL){
		return(0);
	}
	temp=str_concat(current_dir[LOCAL],file_name,NULL);
	for(l=ascii_file[host_number];l!=NULL;l=l->next){
		if(strchr(l->str,'/')==NULL){
			if(cmp_file_with_wildcard(file_name,l->str)==0){
				free(temp);
				return(1);
			}
		}else{
			if(cmp_file_with_wildcard(temp,l->str)==0){
				free(temp);
				return(1);
			}
		}
	}
	free(temp);
	return(0);
}


/* --------------------------------------------------
 NAME       is_change_permission_dir
 FUNCTION   whether ChangePermissionDir or not
 OUTPUT     return 1 if match ChangePermissionDir otherwise return 0
-------------------------------------------------- */
int is_change_permission_dir(void)
{
	cfgList *l;

	if(change_permission_dir[host_number]==NULL){
		return(0);
	}
	for(l=change_permission_dir[host_number];l!=NULL;l=l->next){
		if(cmp_file_with_wildcard(current_dir[LOCAL],l->str)==0){
			return(1);
		}
	}
	return(0);
}


/* --------------------------------------------------
 NAME       is_preserve_permission_dir
 FUNCTION   whether PreservePermissionDir or not
 OUTPUT     return 1 if match PreservePermissionDir otherwise return 0
-------------------------------------------------- */
int is_preserve_permission_dir(void)
{
	cfgList *l;

	if(preserve_permission_dir[host_number]==NULL){
		return(0);
	}
	for(l=preserve_permission_dir[host_number];l!=NULL;l=l->next){
		if(cmp_file_with_wildcard(current_dir[LOCAL],l->str)==0){
			return(1);
		}
	}
	return(0);
}


/* --------------------------------------------------
 NAME       is_keep_remote_dir
 FUNCTION   whether KeepRemoteDir or not
 OUTPUT     return 1 if match KeepRemoteDir otherwise return 0
-------------------------------------------------- */
int is_keep_remote_dir(void)
{
	cfgList *l;

	if(keep_remote_dir[host_number]==NULL){
		return(0);
	}
	for(l=keep_remote_dir[host_number];l!=NULL;l=l->next){
		if(cmp_file_with_wildcard(current_dir[REMOTE],l->str)==0){
			return(1);
		}
	}
	return(0);
}
