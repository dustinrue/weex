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
#include "intl.h"
#include "strlib.h"
#include "variable.h"
#include "proto.h"


static Cache_dir *cache_dir;
static int max_cache_dir;

/* --------------------------------------------------
 NAME       load_cache
 FUNCTION   load cache of time stamp
 OUTPUT     none
-------------------------------------------------- */
void load_cache(void)
{
	char *temp;
	FILE *fp;
	char *read_temp;
	Cache *cache;
	int max_cache_file;
	char *date,*time,*name;
	int i;


	cache_dir=NULL;
	max_cache_file=max_cache_dir=0;
	is_cache_existent=0;

	temp=str_concat(getenv("HOME"),"/.weex/weex.cache.",cfgSectionNumberToName(host_number),NULL);

	if(rebuild_cache){
		fprintf(stderr,_("Rebuilding cache file `%s'.\n"),temp);
		free(temp);
		return;
	}

	fp=fopen(temp,"r");
	if(fp==NULL){
		fprintf(stderr,_("Cache file `%s' does not exist."),temp);
		if(!opt_test){
			 fprintf(stderr,_(" Creating a new one.\n"));
		}else{
			 fprintf(stderr,"\n");
		}
		free(temp);
		return;
	}

	for(i=0;(read_temp=str_fgets(fp))!=NULL;i++){
		date=strtok(read_temp," ");
		time=strtok(NULL," ");
		if(date==NULL || time==NULL){
			fprintf(stderr,_("Cache file `%s' is broken at line %d.\n"),temp,i+1);
			exit(1);
		}

		name=time+strlen(time)+1;
		if(name==NULL || (i==0 && strcmp(date,"00000000")!=0)){
			fprintf(stderr,_("Cache file `%s' is broken at line %d.\n"),temp,i+1);
			exit(1);
		}
		if(strcmp(date,"00000000")==0){	/* directory */
			cache_dir=str_realloc(cache_dir,(max_cache_dir+1)*sizeof(*cache_dir));
			cache_dir[max_cache_dir].name=str_dup(name);
			cache_dir[max_cache_dir].ptr=NULL;
			if(max_cache_dir>0){
				cache_dir[max_cache_dir-1].max_file=max_cache_file;
			}
			max_cache_dir++;
			max_cache_file=0;
		}else{	/* file */
			cache_dir[max_cache_dir-1].ptr=str_realloc(cache_dir[max_cache_dir-1].ptr,(max_cache_file+1)*sizeof(*cache_dir[max_cache_dir-1].ptr));
			cache=cache_dir[max_cache_dir-1].ptr;

			cache[max_cache_file].name=str_dup(name);
			cache[max_cache_file].date=atol(date);
			cache[max_cache_file].time=atol(time);
			max_cache_file++;
		}
		free(read_temp);
	}
	if(max_cache_dir>0){
		cache_dir[max_cache_dir-1].max_file=max_cache_file;
	}
	fclose(fp);
	free(temp);
	is_cache_existent=1;
}


/* --------------------------------------------------
 NAME       find_cache
 FUNCTION   find the file in the cache
 INPUT      cache_dir_num ... the number of directory in a cache
            file_name ... file name
 OUTPUT     pointer to cached file data
            return NULL if not found
-------------------------------------------------- */
Cache *find_cache(char *file_name)
{
	int i;
	int cache_dir_num;
	Cache *cache;

	cache_dir_num=find_cache_dir(current_dir[REMOTE]);
	if(cache_dir_num<0){	/* no such a directory */
		return(NULL);
	}

	cache=cache_dir[cache_dir_num].ptr;
	if(cache==NULL){	/* no files in the directory */
		return(NULL);
	}

	for(i=0;i<cache_dir[cache_dir_num].max_file;i++){
		if(cache[i].name!=NULL && strcmp(cache[i].name,file_name)==0){
			return(cache+i);
		}
	}

	return(NULL);
}


/* --------------------------------------------------
 NAME       get_cache
 FUNCTION   get cached date and time
 INPUT      file_name ... file name
            date ... stored date
            time ... stored time
 OUTPUT     return 0 if successful, -1 otherwise
-------------------------------------------------- */
int get_cache(char *file_name,long *date,long *time)
{
	Cache *cache;

	cache=find_cache(file_name);
	if(cache!=NULL){
		*date=(*cache).date;
		*time=(*cache).time;
		return(0);
	}
	return(-1);
}


/* --------------------------------------------------
 NAME       update_cache
 FUNCTION   update file data of cache
 INPUT      file_name ... file name
            date ... date
            time ... time
 OUTPUT     none
-------------------------------------------------- */
void update_cache(char *file_name,long date,long time)
{
	Cache *cache;
	int cache_dir_num;
	int max_cache_file;

	cache=find_cache(file_name);
	if(cache!=NULL){
		(*cache).date=date;
		(*cache).time=time;
		return;
	}

	cache_dir_num=find_cache_dir(current_dir[REMOTE]);
	if(cache_dir_num<0){
		fprintf(stderr,_("Internal error: cache facility is broken.\n"));
		exit(1);
	}
	max_cache_file=cache_dir[cache_dir_num].max_file;
	cache_dir[cache_dir_num].ptr=str_realloc(cache_dir[cache_dir_num].ptr,(max_cache_file+1)*sizeof(*cache_dir[cache_dir_num].ptr));
	cache=cache_dir[cache_dir_num].ptr;
	cache[max_cache_file].name=str_dup(file_name);
	cache[max_cache_file].date=date;
	cache[max_cache_file].time=time;
	cache_dir[cache_dir_num].max_file++;
}

/* --------------------------------------------------
 NAME       del_cache
 FUNCTION   delete file data from cache
 INPUT      file_name ... file name
 OUTPUT     none
-------------------------------------------------- */
void del_cache(char *file_name)
{
	Cache *cache;

	cache=find_cache(file_name);
	if(cache!=NULL){
		free((*cache).name);
		(*cache).name=NULL;
	}
}


void del_cache_dir(char *dir_name)
{
	int cache_dir_num;
	char *temp;

	temp=str_concat(current_dir[REMOTE],dir_name,"/",NULL);
	cache_dir_num=find_cache_dir(temp);
	free(temp);

	if(cache_dir_num>=0){
		free(cache_dir[cache_dir_num].name);
		cache_dir[cache_dir_num].name=NULL;
	}
}


void update_cache_directory(char *dir)
{
	if(find_cache_dir(dir)<0){
		cache_dir=str_realloc(cache_dir,sizeof(*cache_dir)*(max_cache_dir+1));
		cache_dir[max_cache_dir].name=str_dup(dir);
		cache_dir[max_cache_dir].max_file=0;
		cache_dir[max_cache_dir].ptr=NULL;
		max_cache_dir++;
	}
}


int find_cache_dir(char *name)
{
	int i;

	for(i=0;i<max_cache_dir;i++){
		if(cache_dir[i].name!=NULL && strcmp(cache_dir[i].name,name)==0){
			return(i);
		}
	}
	return(-1);
}


/* --------------------------------------------------
 NAME       save_cache
 FUNCTION   save cache of time stamp
 OUTPUT     none
-------------------------------------------------- */
void save_cache(void)
{
	char *temp;
	FILE *fp;
	Cache *cache;
	int i,j;


	temp=str_concat(getenv("HOME"),"/.weex/weex.cache.",cfgSectionNumberToName(host_number),NULL);
	fp=fopen(temp,"w");
	if(fp==NULL){
		fprintf(stderr,_("Cannot create cache file `%s'.\n"),temp);
		exit(1);
	}

	for(i=0;i<max_cache_dir;i++){
		if(cache_dir[i].name==NULL){	/* deleted directory */
			continue;
		}
		fprintf(fp,"00000000 000000 %s\n",cache_dir[i].name);

		if(cache_dir[i].ptr!=NULL){
			cache=cache_dir[i].ptr;
			for(j=0;j<cache_dir[i].max_file;j++){
				if(cache[j].name==NULL){	/* deleted file */
					continue;
				}
				fprintf(fp,"%08ld %06ld %s\n",cache[j].date,cache[j].time,cache[j].name);
				free(cache[j].name);
			}
			free(cache);
		}
		free(cache_dir[i].name);
	}

	fclose(fp);
	free(temp);
	free(cache_dir);
}


int get_remote_file_data_from_cache(FileData **remote_data)
{
	int cache_dir_num;
	Cache *cache;
	int i;
	int cache_max_file;

	update_cache_directory(current_dir[REMOTE]);

	if(!is_cache_existent){
		return(get_remote_file_data(remote_data));
	}
	cache_dir_num=find_cache_dir(current_dir[REMOTE]);

	cache_max_file=cache_dir[cache_dir_num].max_file;

	*remote_data=str_malloc(sizeof(**remote_data)*cache_max_file);
	cache=cache_dir[cache_dir_num].ptr;
	for(i=0;i<cache_max_file;i++){
		(*remote_data)[i].date=cache[i].date;
		(*remote_data)[i].time=cache[i].time;
		(*remote_data)[i].isdir=(cache[i].date==99999999);
		(*remote_data)[i].name=str_dup(cache[i].name);
	}
	return(cache_max_file);
}

