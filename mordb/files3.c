/*
 * FILES3.C:
 *
 *	File I/O Routines.
 *
 *	Copyright (C) 1991, 1992, 1993 Brooke Paul
 *
 * $Id: files3.c,v 1.5 2001/03/09 05:10:09 develop Exp $
 *
 * $Log: files3.c,v $
 * Revision 1.5  2001/03/09 05:10:09  develop
 * potential fix for memory overflow when saving players
 *
 * Revision 1.4  2001/03/08 16:05:42  develop
 * *** empty log message ***
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../include/morutil.h"

#include "mordb.h"


/************************************************************************/
/*				write_obj_to_mem			*/
/************************************************************************/

/* Save an object to a block of memory.					*/
/* This function recursively saves the items that are contained inside	*/
/* the object as well.  If perm_only != 0 then only permanent objects	*/
/* within the object are saved with it.  This function returns the	*/
/* number of bytes that were written.					*/

int write_obj_to_mem(char *buf, object *obj_ptr, char perm_only, char *bufbegin )
{
	int 	n, cnt, cnt2=0, error=0;
	char	*bufstart;
	otag	*op;

	bufstart = buf;
	
	if(buf-bufbegin+sizeof(object) > 100000)
		return(-1);
	memcpy(buf, obj_ptr, sizeof(object));
	buf += sizeof(object);
	
	cnt = count_obj(obj_ptr, perm_only);

	if(buf-bufbegin+sizeof(int) > 100000)
		return(-1);
	memcpy(buf, &cnt, sizeof(int));
	buf += sizeof(int);

	if(cnt > 0) {
		op = obj_ptr->first_obj;
		while(op) {
			if(!perm_only || (perm_only && 
			   (F_ISSET(op->obj, OPERMT) || 
			   F_ISSET(op->obj, OPERM2)))) {
				if((n = write_obj_to_mem(buf, op->obj, perm_only,bufbegin)) < 0)
					error = 1;
				else
					buf += n;
				cnt2++;
			}
			op = op->next_tag;
		}
	}

	if(cnt != cnt2 || error)
		return(-1);
	else
		return(buf - bufstart);

}

/************************************************************************/
/*				write_crt_to_mem			*/
/************************************************************************/

/* Save a creature to memory.  This function 				*/
/* also saves all the items the creature is holding.  If perm_only != 0 */
/* then only those items which the creature is carrying that are	*/
/* permanent will be saved.						*/

int write_crt_to_mem(char *buf, creature *crt_ptr, char	perm_only)
{
	int 	n, cnt, cnt2=0, error=0;
	char	*bufstart;
	static  char *bufbegin;
	otag	*op;

	bufbegin = bufstart = buf;

	if(buf-bufbegin+sizeof(creature) > 100000)
		return(-1);
	memcpy(buf, crt_ptr, sizeof(creature));
	buf += sizeof(creature);

	cnt = count_inv(crt_ptr, perm_only);

	if(buf-bufbegin+sizeof(int) > 100000)
		return(-1);
	memcpy(buf, &cnt, sizeof(int));
	buf += sizeof(int);

	if(cnt > 0) {
		op = crt_ptr->first_obj;
		while(op && cnt2<cnt) {
			if(!perm_only || (perm_only && 
			   (F_ISSET(op->obj, OPERMT) || F_ISSET(op->obj, OPERM2)))) {
				if((n = write_obj_to_mem(buf, op->obj, perm_only,bufbegin)) < 0)
					error = 1;
				else  
					buf += n;
				
				cnt2++;
				
			}
			op = op->next_tag;
		}
	}

	if(cnt != cnt2 || error)
		return(-1);
	else
		return(buf - bufstart);
}

/************************************************************************/
/*				read_obj_from_mem			*/
/************************************************************************/

/* Loads the object from memory, returns the number of bytes read,	*/
/* and also loads every object which it might contain.  Returns -1 if	*/
/* there was an error.							*/

int read_obj_from_mem(char *buf, object *obj_ptr ) 
{
	int 		n, cnt, error=0;
	char		*bufstart;
	otag		*op;
	otag		**prev;
	object 		*obj;

	bufstart = buf;

	memcpy(obj_ptr, buf, sizeof(object));
	buf += sizeof(object);

	obj_ptr->first_obj = 0;
	obj_ptr->parent_obj = 0;
	obj_ptr->parent_rom = 0;
	obj_ptr->parent_crt = 0;
	if(obj_ptr->shotscur > obj_ptr->shotsmax)
		obj_ptr->shotscur = obj_ptr->shotsmax;

	memcpy(&cnt, buf, sizeof(int));
	buf += sizeof(int);

	prev = &obj_ptr->first_obj;
	while(cnt > 0) {
		cnt--;
		op = (otag *)malloc(sizeof(otag));
		if(op) {
			obj = (object *)malloc(sizeof(object));
			if(obj) {
				if((n = read_obj_from_mem(buf, obj)) < 0)
					error = 1;
				else
					buf += n;
				obj->parent_obj = obj_ptr;
				op->obj = obj;
				op->next_tag = 0;
				*prev = op;
				prev = &op->next_tag;
			}
			else
				merror("read_obj", FATAL);
		}
		else
			merror("read_obj", FATAL);
	}

	if(error)
		return(-1);
	else
		return(buf - bufstart);
}

/************************************************************************/
/*				read_crt_from_mem			*/
/************************************************************************/

/* Loads a creature from memory & returns bytes read.  The creature is	*/
/* loaded at the mem location specified by the second parameter.  In	*/
/* addition, all the creature's objects have memory allocated for them	*/
/* and are loaded as well.  Returns -1 on fail.				*/

int read_crt_from_mem(char *buf, creature *crt_ptr )
{
	int 		n, cnt, error=0;
	char		*bufstart;
	otag		*op;
	otag		**prev;
	object 		*obj;

	bufstart = buf;

	memcpy(crt_ptr, buf, sizeof(creature));
	buf += sizeof(creature);

	crt_ptr->first_obj = 0;
	crt_ptr->first_fol = 0;
	crt_ptr->first_enm = 0;
	crt_ptr->parent_rom = 0;
	crt_ptr->following = 0;
	crt_ptr->first_tlk = 0;

	for(n=0; n<20; n++)
		crt_ptr->ready[n] = 0;
	if(crt_ptr->mpcur > crt_ptr->mpmax)
		crt_ptr->mpcur = crt_ptr->mpmax;
	if(crt_ptr->hpcur > crt_ptr->hpmax)
		crt_ptr->hpcur = crt_ptr->hpmax;

	memcpy(&cnt, buf, sizeof(int));
	buf += sizeof(int);

	prev = &crt_ptr->first_obj;
	while(cnt > 0) {
		cnt--;
		op = (otag *)malloc(sizeof(otag));
		if(op) {
			obj = (object *)malloc(sizeof(object));
			if(obj) {
				if((n = read_obj_from_mem(buf, obj)) < 0)
					error = 1;
				else
					buf += n;
				obj->parent_crt = crt_ptr;
				op->obj = obj;
				op->next_tag = 0;
				*prev = op;
				prev = &op->next_tag;
			}
			else
				merror("read_crt", FATAL);
		}
		else
			merror("read_crt", FATAL);
	}

	if(error)
		return(-1);
	else
		return(buf - bufstart);
}

/************************************************************************/
/*				load_crt_tlk				*/
/************************************************************************/

/* This function loads a creature's talk responses if they exist.	*/

int load_crt_tlk( creature *crt_ptr )
{
	char	crt_name[80], path[256];
	char	keystr[80], responsestr[1024];
	int	i, len1, len2;
	ttag	*tp, **prev;
	FILE	*fp;

	if(!F_ISSET(crt_ptr, MTALKS) || crt_ptr->first_tlk)
		return(0);

	strcpy(crt_name, crt_ptr->name);
	for(i=0; crt_name[i]; i++)
		if(crt_name[i] == ' ')
			crt_name[i] = '_';

	sprintf(path, "%s/talk/%s-%d", get_monster_path(), crt_name, crt_ptr->level);
	fp = fopen(path, "r");
	if(!fp) return(0);

	i = 0;
	prev = &crt_ptr->first_tlk;
	while(!feof(fp)) {
		fgets(keystr, 80, fp);
		len1 = strlen(keystr);
		if(!len1) break;
		keystr[len1-1] = 0;
		fgets(responsestr, 1024, fp);
		len2 = strlen(responsestr);
		if(!len2) break;
		responsestr[len2-1] = 0;

		i++;

		tp = (ttag *)malloc(sizeof(ttag));
		if(!tp)
			merror("load_crt_tlk", FATAL);
		tp->key = (char *)malloc(len1);
		if(!tp->key)
			merror("load_crt_tlk", FATAL);
		tp->response = (char *)malloc(len2);
		if(!tp->response)
			merror("load_crt_tlk", FATAL);
		tp->next_tag = 0;

		strcpy(tp->key, keystr);
		talk_crt_act(keystr,tp);
		strcpy(tp->response, responsestr);

		*prev = tp;
		prev = &tp->next_tag;
	}

	fclose(fp);
	return(i);
}
/***********************************************************************/
/*                          talk_crt_act                               */
/***********************************************************************/
/* the talk_crt_act function, is passed the  key word line from a     *
 * monster talk file, and parses the key word, as well as any monster *
 * monster action (i.e. cast a spell, attack, do a social command)    *
 * The parsed information is then assigned to the fields of the       *
 * monster talkstructure. */

int talk_crt_act(char *str, ttag *tlk )
{

	int 	index =0, num =0,i, n;
	char	*word[4];


	if (!str){
		tlk->key = 0;
		tlk->action = 0;
		tlk->target = 0;
		tlk->type = 0;
		return (0);
	}	

	
	for (i=0;i<4;i++)
		word[i] = 0;

	for (n=0;n<4;n++){

		i=0;
		while(isalpha((int)str[i +index]) || isdigit((int)str[i +index]) || 
			str[i +index] == '-')
			i++;
		word[n] = (char *)malloc(sizeof(char)*i + 1);
		if(!word[n])
			merror("talk_crt_act", FATAL);

		memcpy(word[n],&str[index],i);
		word[n][i] = 0;

		while(isspace((int)str[index +i]))
			i++;
	
		index += i;
		num++;
		if(str[index] == 0)
			break;

	}

	tlk->key = word[0];

	if (num < 2){
		tlk->action = 0;
		tlk->target = 0;
		tlk->type = 0;
		return(0);
	}

	if (!strcmp(word[1],"ATTACK")){
		tlk->type = 1;
		tlk->target = 0;
		tlk->action = 0;
	}
	else if(!strcmp(word[1],"ACTION") && num > 2){
		tlk->type = 2;
		tlk->action = word[2];
		tlk->target = word[3];
	}
	else if(!strcmp(word[1],"CAST") && num > 2){
		tlk->type = 3;
		tlk->action = word[2];
		tlk->target = word[3];
	}
	else if(!strcmp(word[1],"GIVE")){
		tlk->type = 4;
		tlk->action = word[2];
		tlk->target =  0;
	}
	else{
		tlk->type = 0;
		tlk->action = 0;
		tlk->target = 0;
	}
	return(0);
}
