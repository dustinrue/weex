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
#include <ctype.h>
#include <sys/stat.h>
#include "intl.h"
#include "ftplib.h"
#include "strlib.h"
#include "variable.h"
#include "proto.h"


static int total_sent_file;
static int total_removed_file;


/* --------------------------------------------------
 NAME       ftp_main
 FUNCTION   main loop of sending
 INPUT      argc ... argument counter
            argv ... argument vector
            max_hosts ... maximum hosts in the configuration file
 OUTPUT     none
-------------------------------------------------- */
void ftp_main(int argc,char *argv[],int max_hosts)
{
	int i;
	char *temp;

	FtpInit();

	for(i=0;i<argc-1;i++){
		host_number=argv_to_host_num(argv[i+1]);
		
		if(argc-1>1){
			put_mes(PROCESS,0,cfgSectionNumberToName(host_number),NULL);
		}

		if(ftp_connect_init()==-1){
			log_flush();
			continue;
		}
		total_sent_file=total_removed_file=0;
		ftp_recursive_search();

		put_mes(LEAVE,0,dest_dir[host_number],NULL);
		temp=str_dup_printf(_("Complete: sent %d file(s), removed %d file(s)"),total_sent_file,total_removed_file);
		log_write(temp);
		free(temp);
		ftp_disconnect();
		if(!opt_test){
			save_cache();
		}
	}
}


/* --------------------------------------------------
 NAME       argv_to_host_num
 FUNCTION   conver the argument(name or number) to host number
 INPUT      arg ... argument
 OUTPUT     host number
-------------------------------------------------- */
int argv_to_host_num(char *arg)
{
	int num;

	num=atoi(arg)-1;
	if(num==-1){
		num=cfgSectionNameToNumber(arg);
		if(num==-1){
			fprintf(stderr,_("Specified host (section) name `%s' is undefined.\n"),arg);
			exit(1);
		}
	}else{
		if(cfgSectionNumberToName(num)==NULL){
			fprintf(stderr,_("Specified host (section) number `%s' is undefined.\n"),arg);
			exit(1);
		}
	}
	if(strcasecmp("default",cfgSectionNumberToName(num))==0){
		fprintf(stderr,_("Cannot specify a default configuration.\n"));
		exit(1);
	}

	return(num);
}


/* --------------------------------------------------
 NAME       ftp_connect_init
 FUNCTION   connect and login
 OUTPUT     0  ... success
            -1 ... fail
-------------------------------------------------- */
int ftp_connect_init(void)
{
	char *temp;

	log_init();

	put_mes(CONNECT,0,host_name[host_number],NULL);
	if(ftp_connect()==-1){
		return(-1);
	}
	if(ftp_login()==-1){
		return(-1);
	}

	load_cache();

	current_dir[LOCAL]=NULL;
	if(src_dir[host_number]==NULL){
		fprintf(stderr,_("The source directory (SrcDir) is not configured.\n"));
		exit(1);
	}
	change_dir(src_dir[host_number],LOCAL);

	current_dir[REMOTE]=NULL;
	if(dest_dir[host_number]==NULL){
		fprintf(stderr,_("The destination directory (DestDir) is not configured.\n"));
		exit(1);
	}

	change_dir(dest_dir[host_number],REMOTE);
	if(chdir_at_connection[host_number]){
		if(!is_cache_existent){
			temp=str_dup(dest_dir[host_number]);
			if(strcmp(temp,"/")!=0){
				temp[strlen(temp)-1]='\0';
			}
			if(FtpChdir(temp,ftp_buf)==0){
				if(FtpMkdir(temp,ftp_buf)==1){
					put_mes(MKDIR,0,dest_dir[host_number],NULL);
				}
				FtpChdir(temp,ftp_buf);
			}
			free(temp);
		}else{
			change_dir_actually(REMOTE);
		}
	}
	put_mes(ENTER,0,dest_dir[host_number],NULL);

	return(0);
}


/* --------------------------------------------------
 NAME       ftp_connect
 FUNCTION   connect the host
 OUTPUT     0  ... success
            -1 ... fail
-------------------------------------------------- */
int ftp_connect(void)
{
	char *temp;

	if(host_name[host_number]==NULL){
		fprintf(stderr,_("The host name is not configured.\n"));
		exit(1);
	}
	if(!FtpConnect(host_name[host_number],&ftp_buf)){
		fprintf(stderr,_("Cannot connect host `%s'.\n\n"),host_name[host_number]);
		temp=str_dup_printf(_("Connection failed: %s (%s)"),host_name[host_number],cfgSectionNumberToName(host_number));
		log_write(temp);
		free(temp);
		return(-1);
	}
	temp=str_dup_printf(_("Connected: %s (%s)"),host_name[host_number],cfgSectionNumberToName(host_number));
	log_write(temp);
	free(temp);

	return(0);
}


/* --------------------------------------------------
 NAME       ftp_disconnect
 FUNCTION   disconnect the host
 OUTPUT     none
-------------------------------------------------- */
void ftp_disconnect(void)
{
	char *temp;

	put_mes(DISCONNECT,0,host_name[host_number],NULL);

	if(!cfg_silent[host_number] && !opt_silent){
		printf("\n");
	}

	FtpQuit(ftp_buf);
	free(current_dir[LOCAL]);
	free(current_dir[REMOTE]);

	temp=str_dup_printf(_("Disconnected: %s (%s)"),host_name[host_number],cfgSectionNumberToName(host_number));
	log_write(temp);
	free(temp);

	log_flush();
}


/* --------------------------------------------------
 NAME       ftp_login
 FUNCTION   login to the host
 OUTPUT     0  ... success
            -1 ... fail
-------------------------------------------------- */
int ftp_login(void)
{
	if(login_name[host_number]==NULL){
		fprintf(stderr,_("The login name is not configured.\n"));
		exit(1);
	}
	if(password[host_number]==NULL){
		/* Changed by Waleed Kadous <waleed@cse.unsw.edu.au> to ask 
		   for a password if none is given in the weexrc file */ 
		fprintf(stderr,_("No password specified in configuration file for `%s'. Asking ...\n"),host_name[host_number]); 
		password[host_number]=str_dup(getpass(_("Password:")));
	}
	if(!FtpLogin(login_name[host_number],password[host_number],ftp_buf)){
		char *temp;

		fprintf(stderr,_("Login failed. Check your ID and password.\n\n"));
		temp=str_dup_printf(_("Login failed: %s (%s)"),host_name[host_number],cfgSectionNumberToName(host_number));
		log_write(temp);
		free(temp);
		return(-1);
	}
	if(authorization_name[host_number]){
	  while(!FtpAuthorize(authorization_name[host_number], ftp_buf)){
	    fprintf(stderr,_("Authorization failed; try again\n\n"));
	  }
	}
	return(0);
}


/* --------------------------------------------------
 NAME       change_dir
 FUNCTION   change the current working directory
 INPUT      path ... change to path
            side ... LOCAL or REMOTE
 OUTPUT     none
-------------------------------------------------- */
void change_dir(char *path,LocalOrRemote side)
{
	if(current_dir[side]==NULL){
		current_dir[side]=str_dup(path);
	}else{
		current_dir[side]=str_realloc(current_dir[side],strlen(current_dir[side])+strlen(path)+2);
		strcat(current_dir[side],path);
		strcat(current_dir[side],"/");
	}

	if(side==LOCAL){
		change_dir_actually(LOCAL);
	}
}


/* --------------------------------------------------
 NAME       up_dir
 FUNCTION   change the current working directory to the parent one
 INPUT      side ... LOCAL or REMOTE
 OUTPUT     none
-------------------------------------------------- */
void up_dir(LocalOrRemote side)
{
	*strrchr(current_dir[side],'/')='\0';
	*(strrchr(current_dir[side],'/')+1)='\0';
	current_dir[side]=str_realloc(current_dir[side],strlen(current_dir[side])+1);

	if(side==LOCAL){
		change_dir_actually(LOCAL);
	}
}


void change_dir_actually(LocalOrRemote side)
{
	char *temp;

	temp=str_dup(current_dir[side]);
	if(strcmp(temp,"/")!=0){
		temp[strlen(temp)-1]='\0';
	}
	if((side==LOCAL && chdir(temp)==-1) || (side==REMOTE && FtpChdir(temp,ftp_buf)==0)){
		free(temp);
		fprintf(stderr,_("Cannot change %s current working directory to `%s'.\n"),side==LOCAL ? _("local") : _("remote"),current_dir[side]);
		temp=str_dup_printf(_("Changing %s current working directory failed: %s"),side==LOCAL ? _("local") : _("remote"),current_dir[side]);
		log_write(temp);
		free(temp);
		ftp_disconnect();
		exit(1);
	}
	free(temp);
}


/* --------------------------------------------------
 NAME       ftp_recursive_search
 FUNCTION   search files and directories recursively
 OUTPUT     none
-------------------------------------------------- */
void ftp_recursive_search(void)
{
	static int dir_nest=1;
	FileData *local_data=NULL;
	FileData *remote_data=NULL;
	int max_local_file;
	int max_remote_file;
	char *file_name;
	int remote_num;
	int trans_mode;
	int i,j;
	int opt_silent_bak;
	char *temp;
	char *put_temp;
	char *mode_temp=NULL;
	int dir_sent_file=0;
	int dir_removed_file=0;

	max_local_file=get_local_file_data(&local_data);
	max_remote_file=get_remote_file_data_from_cache(&remote_data);

	for(i=0;i<max_local_file;i++){
		if(conv_to_lower[host_number]){
			file_name=str_tolower(local_data[i].name);
		}else{
			file_name=local_data[i].name;
		}
		remote_num=get_equivalent_remote_number(file_name,max_remote_file,remote_data);
		DebugPrint((stderr,"remote_num of `%s' is %d\n",file_name,remote_num));
		if(local_data[i].isdir){
			change_dir(file_name,LOCAL);
			ftp_enter(file_name,(remote_num==-1) ? 0 : remote_data[remote_num].isdir,dir_nest,remote_num,&dir_removed_file);

			dir_nest++;
			ftp_recursive_search();
			dir_nest--;

			put_mes(LEAVE,dir_nest,current_dir[REMOTE],NULL);
			up_dir(LOCAL);
			up_dir(REMOTE);
			if(conv_to_lower[host_number]){
				free(file_name);
			}
			continue;
		}else if(remote_num!=-1 && remote_data[remote_num].isdir){

			remove_remote_dir(file_name,dir_nest,&dir_removed_file);
		}

		if(opt_force || cfg_force[host_number] || remote_num==-1 ||
		   remote_data[remote_num].isdir ||
		   local_data[i].date>remote_data[remote_num].date ||
		  (local_data[i].date==remote_data[remote_num].date &&
		   local_data[i].time>remote_data[remote_num].time)){
			if(!overwrite_ok[host_number] && !rename_ok[host_number]){
				ftp_remove(file_name,dir_nest,&dir_removed_file);
			}
			if(is_ascii_file(file_name)){
				trans_mode=FTPLIB_ASCII;
				if(conv_to_lower[host_number] && strcmp(local_data[i].name,file_name)!=0){
					put_mes(ASCII_LOWER_SEND,dir_nest,local_data[i].name,file_name,NULL);
				}else{
					put_mes(ASCII_SEND,dir_nest,file_name,NULL);
				}
			}else{
				trans_mode=FTPLIB_IMAGE;
				if(conv_to_lower[host_number] && strcmp(local_data[i].name,file_name)!=0){
					put_mes(BINARY_LOWER_SEND,dir_nest,local_data[i].name,file_name,NULL);
				}else{
					put_mes(BINARY_SEND,dir_nest,file_name,NULL);
				}
			}
			if(!opt_test){
			        int success = 0;
				opt_silent_bak=opt_silent;
				if(cfg_silent[host_number]){
					opt_silent=1;
				}
				if (rename_ok[host_number]) {
				  put_temp=str_concat(current_dir[REMOTE],"weex.tmp",NULL);
				} else {
				  put_temp=str_concat(current_dir[REMOTE],file_name,NULL);
				}
				for(j=0;;j++){
					if(FtpPut(local_data[i].name,put_temp,trans_mode,ftp_buf)==1){
						if(log_detail_level[host_number]>=3){
							temp=str_dup_printf(_("Sent: %s%s"),current_dir[REMOTE],file_name);
							log_write(temp);
							free(temp);
						}
						dir_sent_file++;
						update_cache(file_name,local_data[i].date,local_data[i].time);

						if(is_change_permission_dir()){
							mode_temp=str_dup(change_permission[host_number]);
						}else if(is_preserve_permission_dir()){
							struct stat local_file_stat;

							stat(file_name,&local_file_stat);
							mode_temp=str_dup_printf("%o",local_file_stat.st_mode & 0777);
						}

						if(is_preserve_permission_dir() || is_change_permission_dir()){
							put_mes(CHMOD,dir_nest,file_name,NULL);
							if(FtpChmod(put_temp,mode_temp,ftp_buf)==0){
								fprintf(stderr,_("Cannot change the access permissions of `%s%s' to `%s'.\n"),current_dir[REMOTE],file_name,change_permission[host_number]);
								temp=str_dup_printf(_("Changing the access permissions failed: %s%s"),current_dir[REMOTE],file_name);
								log_write(temp);
								free(temp);
							}
							free(mode_temp);
						}
						success = 1;
						break;
					}
					if(j>=max_retry_to_send[host_number]){
						fprintf(stderr,_("Cannot send file `%s%s'.\n"),current_dir[REMOTE],file_name);
						temp=str_dup_printf(_("Sending failed: %s%s"),current_dir[REMOTE],file_name);
						log_write(temp);
						free(temp);

						break;
					}
					temp=str_dup_printf(_("Retry to send: %s%s"),current_dir[REMOTE],file_name);
					log_write(temp);
					free(temp);
					fprintf(stderr,_("Retrying...\n"));
				}
				if (success && rename_ok[host_number]) {
				  char *put_temp2;
				  if(!overwrite_ok[host_number]){
				    ftp_remove(file_name,dir_nest,&dir_removed_file);
				  }
				  put_temp2 = str_concat(current_dir[REMOTE],file_name,NULL);
				  if (!FtpRename(put_temp, put_temp2, ftp_buf)) {
				    fprintf(stderr,_("Cannot rename temporary file to `%s%s'.\n"),current_dir[REMOTE],file_name);
				    temp=str_dup_printf(_("Sending failed: %s%s"),current_dir[REMOTE],file_name);
				    log_write(temp);
				    free(temp);
				  }
				  free(put_temp2);
				}
				free(put_temp);
				opt_silent=opt_silent_bak;
			}
		}
		if(conv_to_lower[host_number]){
			free(file_name);
		}
	}
	remove_files_on_remote_only(max_local_file,max_remote_file,local_data,remote_data,dir_nest,&dir_removed_file);

	free_file_data(local_data,max_local_file);
	free_file_data(remote_data,max_remote_file);

	total_sent_file+=dir_sent_file;
	total_removed_file+=dir_removed_file;

	if(log_detail_level[host_number]>=2){
		temp=str_dup_printf(_("%s : sent %d file(s), removed %d file(s)"),current_dir[REMOTE],dir_sent_file,dir_removed_file);
		log_write(temp);
		free(temp);
	}
}


/* --------------------------------------------------
 NAME       remove_files_on_remote_only
 FUNCTION   remove the files on the remote side only
 INPUT      max_local_file .... the maximum number of local files
            max_remote_file ... the maximum number of remote files
            local_data .... local file data
            remote_data ... remote file data
            dir_nest ...... nest level of directory
            dir_removed_file ... the number of removed files in the directory
 OUTPUT     none
-------------------------------------------------- */
void remove_files_on_remote_only(int max_local_file,int max_remote_file,FileData *local_data,FileData *remote_data,int dir_nest,int *dir_removed_file)
{
	int i;
	int local_num;
	char *file_name;

	for(i=0;i<max_remote_file;i++){
		file_name=remote_data[i].name;
		local_num=get_equivalent_local_number(file_name,max_local_file,local_data);
		if(local_num==-1){
			if(remote_data[i].isdir){
				remove_remote_dir(file_name,dir_nest,dir_removed_file);
			}else{
				ftp_remove(file_name,dir_nest,dir_removed_file);
			}
		}
	}
}


/* --------------------------------------------------
 NAME       get_equivalent_remote_number
 FUNCTION   get remote file number equivalent to local one
 INPUT      local_name ... file name of the local file
            max_remote_file ... the maximum number of remote files
            remote_data ... remote file data
 OUTPUT     remote file number equivalent to local one
            return -1 if no equivalent file
-------------------------------------------------- */
int get_equivalent_remote_number(char *local_name,int max_remote_file,FileData *remote_data)
{
	int i;

	for(i=0;i<max_remote_file;i++){
		if(strcmp(local_name,remote_data[i].name)==0){
			return(i);
		}
	}
	return(-1);
}


/* --------------------------------------------------
 NAME       get_equivalent_local_number
 FUNCTION   get local file number equivalent to remote one
 INPUT      remote_name ... file name of the remote file
            max_local_file ... the maximum number of local files
            local_data ... local file data
 OUTPUT     local file number equivalent to remote one
            return -1 if no equivalent file
-------------------------------------------------- */
int get_equivalent_local_number(char *remote_name,int max_local_file,FileData *local_data)
{
	int i;

	for(i=0;i<max_local_file;i++){
		if(conv_to_lower[host_number]){
			if(strcasecmp(remote_name,local_data[i].name)==0){
				return(i);
			}
		}else{			
			if(strcmp(remote_name,local_data[i].name)==0){
				return(i);
			}
		}
	}
	return(-1);
}


/* --------------------------------------------------
 NAME       ftp_enter
 FUNCTION   enter the directory in remote
 INPUT      dir_name ....... name of entered directory
            remote_isdir ... whether equivalent remote file is directory
            dir_level ...... nest level of directory
            remote_num ..... equivalent remote file number
            dir_removed_file ... the number of removed files in the directory
 OUTPUT     none
-------------------------------------------------- */
void ftp_enter(char *dir_name,int remote_isdir,int dir_nest,int remote_num,int *dir_removed_file)
{
	if(remote_num==-1){
		put_mes(MKDIR,dir_nest,current_dir[REMOTE],dir_name,"/",NULL);
		DebugPrint((stderr,"FtpMkdir: %s(remote_num==-1)\n",dir_name));

		ftp_mkdir(dir_name);
	}else if(!remote_isdir){
		ftp_remove(dir_name,dir_nest,dir_removed_file);

		put_mes(MKDIR,dir_nest,current_dir[REMOTE],dir_name,"/",NULL);
		DebugPrint((stderr,"FtpMkdir: %s(remote_num!=-1 and !isdir)\n",dir_name));

		ftp_mkdir(dir_name);
	}

	put_mes(ENTER,dir_nest,current_dir[REMOTE],dir_name,"/",NULL);

	update_cache(dir_name,99999999,999999);

	change_dir(dir_name,REMOTE);
}


/* --------------------------------------------------
 NAME       ftp_remove
 FUNCTION   remove the remote file
 INPUT      file_name ....... remote file that you want to remove
            dir_nest ........ nest level of directory
            dir_removed_file ... the number of removed files in the directory
 OUTPUT     none
-------------------------------------------------- */
void ftp_remove(char *file_name,int dir_nest,int *dir_removed_file)
{
	char *temp;
	char *del_temp;

	if(is_keep_remote_dir()){
		return;
	}
	put_mes(REMOVE,dir_nest,file_name,NULL);
	if(opt_test){
		return;
	}
	del_temp=str_concat(current_dir[REMOTE],file_name,NULL);
	if(FtpDelete(del_temp,ftp_buf)==0){
		fprintf(stderr,_("Cannot remove remote file `%s%s'.\n"),current_dir[REMOTE],file_name);
		temp=str_dup_printf(_("Removing failed: %s%s"),current_dir[REMOTE],file_name);
		log_write(temp);
		free(temp);
	}else{
		if(log_detail_level[host_number]>=3){
			temp=str_dup_printf(_("Removed: %s%s"),current_dir[REMOTE],file_name);
			log_write(temp);
			free(temp);
		}
		(*dir_removed_file)++;
		del_cache(file_name);
	}
	free(del_temp);
}


/* --------------------------------------------------
 NAME       remove_remote_dir
 FUNCTION   remove the remote directory recursively
 INPUT      dir_name ... name of the remote directory that you want to remove
            dir_nest ... nest level of directory
            dir_removed_file ... the number of removed files in the directory
 OUTPUT     none
-------------------------------------------------- */
void remove_remote_dir(char *dir_name,int dir_nest,int *dir_removed_file)
{
	FileData *remote_data=NULL;
	int max_remote_file;
	char *file_name;
	int i;
	char *rm_temp;

	put_mes(ENTER,dir_nest,current_dir[REMOTE],dir_name,"/",NULL);
	change_dir(dir_name,REMOTE);

	if(is_keep_remote_dir()){
		put_mes(LEAVE,dir_nest,current_dir[REMOTE],NULL);
		up_dir(REMOTE);
		return;
	}

	dir_nest++;

	max_remote_file=get_remote_file_data_from_cache(&remote_data);
	for(i=0;i<max_remote_file;i++){
		file_name=remote_data[i].name;
		if(remote_data[i].isdir){
			remove_remote_dir(file_name,dir_nest,dir_removed_file);
		}else{
			ftp_remove(file_name,dir_nest,dir_removed_file);
		}
	}

	dir_nest--;
	put_mes(LEAVE,dir_nest,current_dir[REMOTE],NULL);
	up_dir(REMOTE);
	put_mes(REMOVE,dir_nest,current_dir[REMOTE],dir_name,"/",NULL);
	if(opt_test){
		return;
	}
	if(!is_cache_existent){
		change_dir_actually(REMOTE);
	}
	rm_temp=str_concat(current_dir[REMOTE],dir_name,NULL);
	if(FtpRmdir(rm_temp,ftp_buf)==0){
		char *temp;

		fprintf(stderr,_("Cannot remove remote directory `%s%s/'.\n"),current_dir[REMOTE],dir_name);
		temp=str_dup_printf(_("Removing failed: %s%s/"),current_dir[REMOTE],dir_name);
		log_write(temp);
		free(temp);
	}else{
		del_cache(dir_name);
		del_cache_dir(dir_name);
	}
	free(rm_temp);
}


/* --------------------------------------------------
 NAME       ftp_mkdir
 FUNCTION   make the directory
 INPUT      dir_name ... name of directory that you want to create
 OUTPUT     none
-------------------------------------------------- */
void ftp_mkdir(char *dir_name)
{
	char *temp;
	
	if(opt_test){
		return;
	}
	temp=str_concat(current_dir[REMOTE],dir_name,NULL);
	if(FtpMkdir(temp,ftp_buf)==0){
		free(temp);
		fprintf(stderr,_("Cannot make remote directory `%s%s/'.\n"),current_dir[REMOTE],dir_name);
		temp=str_dup_printf(_("Making directory failed: %s%s/"),current_dir[REMOTE],dir_name);
		log_write(temp);
		free(temp);
		ftp_disconnect();
		exit(1);
	}
	free(temp);
}



