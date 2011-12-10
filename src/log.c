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

#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "intl.h"
#include "strlib.h"
#include "variable.h"
#include "proto.h"


static char **log_str;
static int max_log;
static char weex_host[64];
static pid_t weex_pid;


/* --------------------------------------------------
 NAME       log_init
 FUNCTION   initialize logging facility
 INPUT      none
 OUTPUT     none
-------------------------------------------------- */
void log_init(void)
{
	weex_pid=getpid();
	if(gethostname(weex_host,63)==-1){
		fprintf(stderr,_("Cannot get a host name. Proceed anyway.\n"));
		strcpy(weex_host,"unknown");
	}
	log_str=NULL;
	max_log=0;
}


/* --------------------------------------------------
 NAME       log_open
 FUNCTION   create a lock file and open a log file
 INPUT      none
 OUTPUT     file pointer to the log file
-------------------------------------------------- */
FILE *log_open(void)
{
	char *log_file;
	char *lock_file;
	FILE *fp;
	int fd;
	int i=0;

	log_file=str_concat(getenv("HOME"),"/.weex/weex.log",NULL);
	lock_file=str_concat(getenv("HOME"),"/.weex/.lock",NULL);

	while(access(lock_file,F_OK)==0){
		i++;
		if(i>120){
			fprintf(stderr,_("Stopped.\n"));
			exit(1);
		}
		if((i%12)==0){
			fprintf(stderr,_("Log file `%s' seems to be always locked.\nRemove lock file `%s'.\n"),log_file,lock_file);
		}else{
			fprintf(stderr,_("Another weex is using log file `%s'. Just a moment.\n"),log_file);
		}
		sleep(5);
	}

	fd=creat(lock_file,S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if(fd==-1){
		fprintf(stderr,"Cannot create lock file `%s'.\n",lock_file);
		exit(1);
	}
	close(fd);
	
	fp=fopen(log_file,"a");
	if(fp==NULL){
		fprintf(stderr,"Cannot open log file `%s'.\n",log_file);
		remove(lock_file);
	}

	free(lock_file);
	free(log_file);

	return(fp);
}


/* --------------------------------------------------
 NAME       log_close
 FUNCTION   close a log file and remove a lock file
 INPUT      none
 OUTPUT     none
-------------------------------------------------- */
void log_close(FILE *fp)
{
	char *lock_file;

	lock_file=str_concat(getenv("HOME"),"/.weex/.lock",NULL);

	fclose(fp);
	remove(lock_file);

	free(lock_file);
}



/* --------------------------------------------------
 NAME       log_write
 FUNCTION   write a log
 INPUT      str ... message to write
 OUTPUT     none
-------------------------------------------------- */
void log_write(char *str)
{
	struct tm *nowtime;
	time_t t;
	char *month_str[12]={
		N_("Jan"),
		N_("Feb"),
		N_("Mar"),
		N_("Apr"),
		N_("May"),
		N_("Jun"),
		N_("Jul"),
		N_("Aug"),
		N_("Sep"),
		N_("Oct"),
		N_("Nov"),
		N_("Dec"),
	};

	t=time(NULL);
	nowtime=localtime(&t);

	max_log++;
	log_str=str_realloc(log_str,max_log*sizeof(*log_str));
	log_str[max_log-1]=str_dup_printf(_("%s %2d %02d:%02d:%02d %s weex[%d]: %s\n"),_(month_str[nowtime->tm_mon]),nowtime->tm_mday,nowtime->tm_hour,nowtime->tm_min,nowtime->tm_sec,weex_host,weex_pid,str);
}


/* --------------------------------------------------
 NAME       log_flush
 FUNCTION   flush a log
 OUTPUT     none
-------------------------------------------------- */
void log_flush(void)
{
	FILE *fp;
	int i;

	if(!record_log[host_number]){
		return;
	}

	fp=log_open();
	for(i=0;i<max_log;i++){
		fprintf(fp,log_str[i]);
		free(log_str[i]);
	}
	free(log_str);

	log_close(fp);
}



