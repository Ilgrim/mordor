/*
 * ROOM.C:
 *
 *	Room routines.
 *
 *	Copyright (C) 1991, 1992, 1993 Brett J. Vickers
 *
 */

#include "mstruct.h"
#include "mextern.h"
#ifdef DMALLOC
  #include "/usr/local/include/dmalloc.h"
#endif

/**********************************************************************/
/*				add_ply_rom			      */
/**********************************************************************/

/* This function is called to add a player to a room.  It inserts the */
/* player into the room's linked player list, alphabetically.  Also,  */
/* the player's room pointer is updated.			      */

void add_ply_rom(ply_ptr, rom_ptr)
creature	*ply_ptr;
room		*rom_ptr;
{
	ctag	*cg, *cp, *temp, *prev;

	ply_ptr->parent_rom = rom_ptr;
	ply_ptr->rom_num = rom_ptr->rom_num;
	rom_ptr->beenhere++;
	
	cg = 0;
	cg = (ctag *)malloc(sizeof(ctag));
	if(!cg)
		merror("add_ply_rom", FATAL);
	cg->crt = ply_ptr;
	cg->next_tag = 0;

	if(!F_ISSET(ply_ptr, PDMINV) && !F_ISSET(ply_ptr, PHIDDN)) {
		broadcast_rom(ply_ptr->fd, ply_ptr->rom_num, 
		              "%M just arrived.", ply_ptr);
	}

	add_permcrt_rom(rom_ptr);
	add_permobj_rom(rom_ptr);
	check_exits(rom_ptr);

	display_rom(ply_ptr, rom_ptr);

	if(!rom_ptr->first_ply) {
		rom_ptr->first_ply = cg;
		cp = rom_ptr->first_mon;
		while(cp) {
			add_active(cp->crt);
			cp = cp->next_tag;
		}
		return;
	}

	temp = rom_ptr->first_ply;
	if(strcmp(temp->crt->name, ply_ptr->name) > 0) {
		cg->next_tag = temp;
		rom_ptr->first_ply = cg;
		return;
	}

	while(temp) {
		if(strcmp(temp->crt->name, ply_ptr->name) > 0) 
			break;
		prev = temp;
		temp = temp->next_tag;
	}
	cg->next_tag = prev->next_tag;
	prev->next_tag = cg;

}

/**********************************************************************/
/*				del_ply_rom			      */
/**********************************************************************/

/* This function removes a player from a room's linked list of players. */
/* It also resets the player's room pointer.				*/

void del_ply_rom(ply_ptr, rom_ptr)
creature	*ply_ptr;
room		*rom_ptr;
{
	ctag 	*cp, *temp, *prev;

	ply_ptr->parent_rom = 0;
	/* 6/17/95 BP */
	/*	if(!rom_ptr->first_ply)
		return; */
		
	if(rom_ptr->first_ply->crt == ply_ptr) {
		temp = rom_ptr->first_ply->next_tag;
		free(rom_ptr->first_ply);
		rom_ptr->first_ply = temp;

		if(!temp) {
			cp = rom_ptr->first_mon;
			while(cp) {
				del_active(cp->crt);
				cp = cp->next_tag;
			}
		}
		return;
	}

	prev = rom_ptr->first_ply;
	temp = prev->next_tag;
	while(temp) {
		if(temp->crt == ply_ptr) {
			prev->next_tag = temp->next_tag;
			free(temp);
			return;
		}
		prev = temp;
		temp = temp->next_tag;
	}
}

/**********************************************************************/
/*				add_obj_rom			      */
/**********************************************************************/

/* This function adds the object pointed to by the first parameter to */
/* the object list of the room pointed to by the second parameter.    */
/* The object is added alphabetically to the room.		      */

void add_obj_rom(obj_ptr, rom_ptr)
object	*obj_ptr;
room	*rom_ptr;
{
	otag	*op, *temp, *prev;
	int num;

	
	obj_ptr->parent_rom = rom_ptr;
	obj_ptr->parent_obj = 0;
	obj_ptr->parent_crt = 0;

	op = 0;
	op = (otag *)malloc(sizeof(otag));
	if(!op)
		merror("add_obj_rom", FATAL);
	op->obj = obj_ptr;
	op->next_tag = 0;

	if(!rom_ptr->first_obj) {
		rom_ptr->first_obj = op;
		return;
	}

	prev = temp = rom_ptr->first_obj;
	if(strcmp(temp->obj->name, obj_ptr->name) > 0 ||
	   (!strcmp(temp->obj->name, obj_ptr->name) &&
	   temp->obj->adjustment > obj_ptr->adjustment)) {
		op->next_tag = temp;
		rom_ptr->first_obj = op;
		return;
	}

	while(temp) {
		if(strcmp(temp->obj->name, obj_ptr->name) > 0 ||
		   (!strcmp(temp->obj->name, obj_ptr->name) &&
		   temp->obj->adjustment > obj_ptr->adjustment))
			break;
		prev = temp;
		temp = temp->next_tag;
	}
	op->next_tag = prev->next_tag;
	prev->next_tag = op;

}

/**********************************************************************/
/*				del_obj_rom			      */
/**********************************************************************/

/* This function removes the object pointer to by the first parameter */
/* from the room pointed to by the second.			      */

void del_obj_rom(obj_ptr, rom_ptr)
object	*obj_ptr;
room	*rom_ptr;
{
	otag 	*temp, *prev;

	obj_ptr->parent_rom = 0;
	if(rom_ptr->first_obj->obj == obj_ptr) {
		temp = rom_ptr->first_obj->next_tag;
		free(rom_ptr->first_obj);
		rom_ptr->first_obj = temp;
		return;
	}

	prev = rom_ptr->first_obj;
	temp = prev->next_tag;
	while(temp) {
		if(temp->obj == obj_ptr) {
			prev->next_tag = temp->next_tag;
			free(temp);
			return;
		}
		prev = temp;
		temp = temp->next_tag;
	}
}

/**********************************************************************/
/*				add_crt_rom			      */
/**********************************************************************/

/* This function adds the monster pointed to by the first parameter to	*/
/* the room pointed to by the second.  The third parameter determines	*/
/* how the people in the room should be notified.  If it is equal to	*/
/* zero, then no one will be notified that a monster entered the room.	*/
/* If it is non-zero, then the room will be told that "num" monsters	*/
/* of that name entered the room.					*/

void add_crt_rom(crt_ptr, rom_ptr, num)
creature	*crt_ptr;
room		*rom_ptr;
int		num;
{
	ctag	*cg, *temp, *prev;
	char	str[160];

	crt_ptr->parent_rom = rom_ptr;
	crt_ptr->rom_num = rom_ptr->rom_num;

	cg = 0;
	cg = (ctag *)malloc(sizeof(ctag));
	if(!cg)
		merror("add_crt_rom", FATAL);
	cg->crt = crt_ptr;
	cg->next_tag = 0;

	sprintf(str, "%%%dM just arrived.", num);
	if(!F_ISSET(crt_ptr, MINVIS) && !F_ISSET(crt_ptr, MPERMT) && num)
		broadcast_rom(crt_ptr->fd, crt_ptr->rom_num, str, crt_ptr);

	if(!rom_ptr->first_mon) {
		rom_ptr->first_mon = cg;
		return;
	}

	temp = rom_ptr->first_mon;
	if(strcmp(temp->crt->name, crt_ptr->name) > 0) {
		cg->next_tag = temp;
		rom_ptr->first_mon = cg;
		return;
	}

	while(temp) {
		if(strcmp(temp->crt->name, crt_ptr->name) > 0) 
			break;
		prev = temp;
		temp = temp->next_tag;
	}
	cg->next_tag = prev->next_tag;
	prev->next_tag = cg;

}

/**********************************************************************/
/*				del_crt_rom			      */
/**********************************************************************/

/* This function removes the monster pointed to by the first parameter */
/* from the room pointed to by the second.			       */

void del_crt_rom(crt_ptr, rom_ptr)
creature	*crt_ptr;
room		*rom_ptr;
{
	ctag 	*temp, *prev;

	crt_ptr->parent_rom = 0;
	crt_ptr->rom_num = 0;
	if(!rom_ptr)
		return;
	if(!rom_ptr->first_mon)
		return;

	if(rom_ptr->first_mon->crt == crt_ptr) {
		temp = rom_ptr->first_mon->next_tag;
		free(rom_ptr->first_mon);
		rom_ptr->first_mon = temp;
		return;
	}


	prev = rom_ptr->first_mon;
	temp = prev->next_tag;
	while(temp) {
		if(temp->crt == crt_ptr) {
			prev->next_tag = temp->next_tag;
			free(temp);
			return;
		}
		prev = temp;
		temp = temp->next_tag;
	}
}

/**********************************************************************/
/*				add_permcrt_rom			      */
/**********************************************************************/

/* This function checks a room to see if any permanent monsters need to */
/* be loaded.  If so, the monsters are loaded to the room, and their    */
/* permanent flag is set.						*/

void add_permcrt_rom(rom_ptr)
room	*rom_ptr;
{
	short		checklist[10];
	creature	*crt_ptr;
	object		*obj_ptr;
	ctag		*cp;
	long		t;
	int		h, i, j, k, l, m, n;

	t = time(0);

	for(i=0; i<10; i++)
		checklist[i] = 0;

	for(i=0; i<10; i++) {

		if(checklist[i]) continue;
		if(!rom_ptr->perm_mon[i].misc) continue;
		if(rom_ptr->perm_mon[i].ltime + rom_ptr->perm_mon[i].interval >
		   t) continue;

		n = 1;
		for(j=i+1; j<10; j++) 
			if(rom_ptr->perm_mon[i].misc == 
			   rom_ptr->perm_mon[j].misc && 
			   (rom_ptr->perm_mon[j].ltime + 
			   rom_ptr->perm_mon[j].interval) < t) {
				n++;
				checklist[j] = 1;
			}

		if(load_crt(rom_ptr->perm_mon[i].misc, &crt_ptr) < 0)
			continue;

		cp = rom_ptr->first_mon;
		m = 0;
		while(cp) {
			if(F_ISSET(cp->crt, MPERMT) && 
			   !strcmp(cp->crt->name, crt_ptr->name))
				m++;
			cp = cp->next_tag;
		}

		free_crt(crt_ptr);

		for(j=0; j<n-m; j++) {
			l = load_crt(rom_ptr->perm_mon[i].misc, &crt_ptr);
			if(l < 0) continue;
			l = mrand(1,100);
			if(l<90) l=1;
			else if(l<96) l=2;
			else l=3;
			for(k=0; k<l; k++) {
				h = mrand(0,9);
				if(crt_ptr->carry[h]) {
					h = load_obj(crt_ptr->carry[h], 
						     &obj_ptr);
					if(F_ISSET(obj_ptr, ORENCH))
						rand_enchant(obj_ptr);
					if(h > -1)
						add_obj_crt(obj_ptr, crt_ptr);
				}
			}
			if(!F_ISSET(crt_ptr, MNRGLD) && crt_ptr->gold)
				crt_ptr->gold = mrand(crt_ptr->gold/10,
					crt_ptr->gold);
			F_SET(crt_ptr, MPERMT);
			add_crt_rom(crt_ptr, rom_ptr);
			if(rom_ptr->first_ply)
				add_active(crt_ptr);
		}
	}
}

/**********************************************************************/
/*				add_permobj_rom			      */
/**********************************************************************/

/* This function checks a room to see if any permanent objects need to  */
/* be loaded.  If so, the objects are loaded to the room, and their     */
/* permanent flag is set.						*/

void add_permobj_rom(rom_ptr)
room	*rom_ptr;
{
	short	checklist[10];
	object	*obj_ptr;
	otag	*op;
	long	t;
	int	i, j, m, n;

	t = time(0);

	for(i=0; i<10; i++)
		checklist[i] = 0;

	for(i=0; i<10; i++) {

		if(checklist[i]) continue;
		if(!rom_ptr->perm_obj[i].misc) continue;
		if(rom_ptr->perm_obj[i].ltime + rom_ptr->perm_obj[i].interval >
		   t) continue;

		n = 1;
		for(j=i+1; j<10; j++) 
			if(rom_ptr->perm_obj[i].misc == 
			   rom_ptr->perm_obj[j].misc && 
			   (rom_ptr->perm_obj[j].ltime + 
			   rom_ptr->perm_obj[j].interval) < t) {
				n++;
				checklist[j] = 1;
			}

		if(load_obj(rom_ptr->perm_obj[i].misc, &obj_ptr) < 0)
			continue;

		op = rom_ptr->first_obj;
		m = 0;
		while(op) {
			if(F_ISSET(op->obj, OPERMT) && 
			   !strcmp(op->obj->name, obj_ptr->name))
				m++;
			op = op->next_tag;
		}

		free_obj(obj_ptr);

		for(j=0; j<n-m; j++) {
			if(load_obj(rom_ptr->perm_obj[i].misc, &obj_ptr) < 0)
				continue;
			if(F_ISSET(obj_ptr, ORENCH))
				rand_enchant(obj_ptr);

				
			F_SET(obj_ptr, OPERMT);
			add_obj_rom(obj_ptr, rom_ptr);
		}
	}
}

/**********************************************************************/
/*				check_exits			      */
/**********************************************************************/

/* This function checks the status of the exits in a room.  If any of */
/* the exits are closable or lockable, and the correct time interval  */
/* has occurred since the last opening/unlocking, then the doors are  */
/* re-shut/re-closed.						      */

void check_exits(rom_ptr)
room	*rom_ptr;
{
	xtag	*xp;
	int	tmp;
	long	t;

	t = time(0);

	xp = rom_ptr->first_ext;
	while(xp) {
		if(F_ISSET(xp->ext, XLOCKS) && (xp->ext->ltime.ltime + 
		   xp->ext->ltime.interval) < t) {
			F_SET(xp->ext, XLOCKD);
			F_SET(xp->ext, XCLOSD);
		}
		else if(F_ISSET(xp->ext, XCLOSS) && (xp->ext->ltime.ltime +
			 xp->ext->ltime.interval) < t)
			F_SET(xp->ext, XCLOSD);

		xp = xp->next_tag;
	}

}

/**********************************************************************/
/*				display_rom			      */
/**********************************************************************/

/* This function displays the descriptions of a room, all the players */
/* in a room, all the monsters in a room, all the objects in a room,  */
/* and all the exits in a room.  That is, unless they are not visible */
/* or the room is dark.						      */

void display_rom(ply_ptr, rom_ptr)
creature	*ply_ptr;
room		*rom_ptr;
{
	xtag		*xp;
	ctag		*cp;
	otag		*op;
	creature	*crt_ptr;
	char		str[2048];
	int		fd, n=0, m, t, light = 0;

	fd = ply_ptr->fd;

	print(fd, "\n");

	t = Time%24L;
	if(F_ISSET(ply_ptr, PBLIND) || F_ISSET(rom_ptr, RDARKR) || 
	   (F_ISSET(rom_ptr, RDARKN) && (t<6 || t>20))) {
		if(!has_light(ply_ptr)) {
			cp = rom_ptr->first_ply;
			while(cp) {
				if(has_light(cp->crt)) {
					light = 1;
					break;
				}
				cp = cp->next_tag;
			}
			if(ply_ptr->race == ELF ||
			    ply_ptr->race == DWARF ||
			    ply_ptr->class >= CARETAKER)
				light = 1;
		}
		else
			light = 1;

		if(!light || F_ISSET(ply_ptr, PBLIND)) {
			ANSI(fd, RED);
			if(F_ISSET(ply_ptr, PBLIND)) print(fd, "You're blind!\n");
			ANSI(fd, YELLOW);
			print(fd, "It's too dark to see.\n");
			ANSI(fd, WHITE);
			return;
		}
		
	}
	
	if(!F_ISSET(ply_ptr, PNORNM)) {
		ANSI(fd, CYAN);
		if (ply_ptr->class >= CARETAKER)
			print(fd, "%d - %s\n\n", rom_ptr->rom_num, rom_ptr->name);
		else
			print(fd, "%s\n\n", rom_ptr->name);
		ANSI(fd, WHITE);
	}

	if(!F_ISSET(ply_ptr, PNOSDS) && rom_ptr->short_desc)
		print(fd, "%s\n", rom_ptr->short_desc);

	if(!F_ISSET(ply_ptr, PNOLDS) && rom_ptr->long_desc)
		print(fd, "%s\n", rom_ptr->long_desc);

	if(!F_ISSET(ply_ptr, PNOEXT)) {
		n=0; str[0] = 0;
		strcat(str, "Obvious exits: ");
		xp = rom_ptr->first_ext;
		while(xp) {
			if(!F_ISSET(xp->ext, XSECRT) && 
			   !F_ISSET(xp->ext, XNOSEE) &&
			   (F_ISSET(ply_ptr, PDINVI) ? 
			   1:!F_ISSET(xp->ext, XINVIS))) {
				strcat(str, xp->ext->name);
				strcat(str, ", ");
				n++;
			}
			xp = xp->next_tag;
		}
		if(!n)
			strcat(str, "none");
		else
			str[strlen(str)-2] = 0;
		ANSI(fd, GREEN);
		print(fd, "%s.\n", str);
		ANSI(fd, WHITE);
	}

	cp = rom_ptr->first_ply; n=0; str[0]=0;
	strcat(str, "You see ");
	while(cp) {
		if((F_ISSET(ply_ptr, PDINVI) ? 1:!F_ISSET(cp->crt, PINVIS)) &&
		   !F_ISSET(cp->crt, PHIDDN) &&
		   !(ply_ptr->class < DM && F_ISSET(cp->crt, PDMINV)) &&
		   cp->crt != ply_ptr) {
			strcat(str, cp->crt->name);
			strcat(str, ", ");
			n++;
		}
		cp = cp->next_tag;
	}
	if(n) {
		str[strlen(str)-2] = 0;
		ANSI(fd, CYAN);
		print(fd, "%s.\n", str);
		ANSI(fd, WHITE);
	}

	cp = rom_ptr->first_mon; n=0; str[0]=0;
	strcat(str, "You see ");
	while(cp) {
		if((F_ISSET(ply_ptr, PDINVI) ? 1:!F_ISSET(cp->crt, MINVIS)) &&
		   !F_ISSET(cp->crt, MHIDDN)) {
			m=1;
			while(cp->next_tag) {
				if(!strcmp(cp->next_tag->crt->name, 
					   cp->crt->name) &&
				   (F_ISSET(ply_ptr, PDINVI) ? 
				   1:!F_ISSET(cp->next_tag->crt, MINVIS)) &&
				   !F_ISSET(cp->next_tag->crt, MHIDDN)) {
					m++;
					cp = cp->next_tag;
				}
				else
					break;
			}
			strcat(str, crt_str(cp->crt, m, 0));
			strcat(str, ", ");
			n++;
		}
		cp = cp->next_tag;
	}
	if(n) {
		str[strlen(str)-2] = 0;
		print(fd, "%s.\n", str);
	}

	str[0]=0;  strcat(str, "You see ");
	n = list_obj(&str[8], ply_ptr, rom_ptr->first_obj);

	if(n)
		print(fd, "%s.\n", str);

	cp = rom_ptr->first_mon;
	while(cp) {
		if(cp->crt->first_enm) {
			crt_ptr = find_crt(ply_ptr, rom_ptr->first_ply,
					   cp->crt->first_enm->enemy, 1);
			if(crt_ptr == ply_ptr)
				print(fd, "%M is attacking you.\n", cp->crt);
			else if(crt_ptr)
				print(fd, "%M is attacking %m.\n", cp->crt,
				      crt_ptr);
		}
		cp = cp->next_tag;
	}

	print(fd, "\n");

}

/**********************************************************************/
/*				find_ext			      */
/**********************************************************************/

/* This function attempts to find the exit specified by the given string */
/* and value by looking through the exit list headed by the second para- */
/* meter.  If found, a pointer to the exit is returned.			 */

exit_ *find_ext(ply_ptr, first_xt, str, val)
creature	*ply_ptr;
xtag		*first_xt;
char		*str;
int		val;
{
	xtag	*xp;
	int	match = 0, found = 0;

	xp = first_xt;
	while(xp) {
		if(!strncmp(xp->ext->name, str, strlen(str)) &&
		  (!F_ISSET(xp->ext, XNOSEE)) &&
		   (F_ISSET(ply_ptr, PDINVI) ? 1:!F_ISSET(xp->ext, XINVIS)))
			match++;
		if(match == val) {
			found = 1;
			break;
		}
		xp = xp->next_tag;
	}

	if(!found)
		return(0);

	return(xp->ext);

}

/**********************************************************************/
/*				check_traps			      */
/**********************************************************************/

/* This function checks a room to see if there are any traps and whether */
/* the player pointed to by the first parameter fell into any of them.   */

void check_traps(ply_ptr, rom_ptr)
creature	*ply_ptr;
room		*rom_ptr;
{
	room	*new_rom;
	ctag	*cp;
	creature *tmp_crt;
	int		i, dmg, fd;

	if(!rom_ptr->trap) {
		F_CLR(ply_ptr, PPREPA);
		return;
	}
	if(ply_ptr->class > CARETAKER){
		ANSI(ply_ptr->fd, MAGENTA);
		print(ply_ptr->fd, "Trap passed.\n");
		ANSI(ply_ptr->fd, NORMAL);
		return;
	}

	switch(rom_ptr->trap){

	case TRAP_PIT:
	case TRAP_DART:
	case TRAP_BLOCK:
	case TRAP_NAKED:
	case TRAP_ALARM:
	    if(F_ISSET(ply_ptr, PPREPA) && mrand(1,20) < ply_ptr->dexterity) {
		F_CLR(ply_ptr, PPREPA);
		return;
	     }
	    F_CLR(ply_ptr, PPREPA);
	    if(mrand(1,100) < ply_ptr->dexterity)
		return;

	break;
	case TRAP_MPDAM:
	case TRAP_RMSPL:
	    if(F_ISSET(ply_ptr, PPREPA) && mrand(1,25) < 
		(ply_ptr->intelligence)) {
		F_CLR(ply_ptr, PPREPA);
		return;
	     }
	    F_CLR(ply_ptr, PPREPA);
	    if(mrand(1,100) < ply_ptr->intelligence)
		return;

	break;
	default:
		return;
	}


	fd = ply_ptr->fd;

	switch (rom_ptr->trap){
		case TRAP_PIT:
			if(!F_ISSET(ply_ptr, PLEVIT)){
				print(fd, "You fell into a pit trap!\n");
				broadcast_rom(fd, ply_ptr->rom_num, 
				"%M fell into a pit trap!", ply_ptr);
				del_ply_rom(ply_ptr, rom_ptr);
				load_rom(rom_ptr->trapexit, &new_rom);
				add_ply_rom(ply_ptr, new_rom);
				dmg = mrand(1,15);
				print(fd, "You lost %d hit points.\n", dmg);
				ply_ptr->hpcur -= dmg;
				if(ply_ptr->hpcur < 1)
					die(ply_ptr, ply_ptr);
			}
			break;

		case TRAP_DART:
			print(fd, "You triggered a hidden dart!\n");
			broadcast_rom(fd, ply_ptr->rom_num,
				"%M gets hit by a hidden dart.", ply_ptr);
			dmg = mrand(1,10);
			print(fd, "You lost %d hit points.\n", dmg);
			F_SET(ply_ptr, PPOISN);
			ply_ptr->hpcur -= dmg;
			if(ply_ptr->hpcur < 1)
				die(ply_ptr, ply_ptr);
			break;

		case TRAP_BLOCK:
			print(fd, "You triggered a falling block!\n");
			broadcast_rom(fd, ply_ptr->rom_num,
				"A large block falls on %m.", ply_ptr);
			dmg = ply_ptr->hpmax / 2;
			print(fd, "You lost %d hit points.\n", dmg);
			ply_ptr->hpcur -= dmg;
			if(ply_ptr->hpcur < 1)
				die(ply_ptr, ply_ptr);
			break;

		case TRAP_MPDAM:
			print(fd, "You feel an exploding force in your mind!\n");
			broadcast_rom(fd, ply_ptr->rom_num,
				"An energy bolts strikes %m.", ply_ptr);
			dmg = MIN(ply_ptr->mpcur,ply_ptr->mpmax / 2);
			print(fd, "You lost %d magic points.\n", dmg);
			ply_ptr->mpcur -= dmg;
			dmg =  mrand(1,6);
			ply_ptr->hpcur -= dmg;
			print(fd, "You lost %d hit points.\n", dmg);
			if(ply_ptr->hpcur < 1) die(ply_ptr, ply_ptr);
			break;

		case TRAP_RMSPL:
			print(fd, "A foul smelling charcoal cloud surrounds you.\n");
			print(fd, "Your magic begins to dissolve.\n");
			broadcast_rom(fd, ply_ptr->rom_num,
				"A charcoal cloud surrounds %m.", ply_ptr);
		
			ply_ptr->lasttime[LT_PROTE].interval =
			ply_ptr->lasttime[LT_BLESS].interval = 
			ply_ptr->lasttime[LT_RFIRE].interval =
			ply_ptr->lasttime[LT_RCOLD].interval = 
			ply_ptr->lasttime[LT_BRWAT].interval =
			ply_ptr->lasttime[LT_SSHLD].interval =
			ply_ptr->lasttime[LT_RMAGI].interval =
			ply_ptr->lasttime[LT_LIGHT].interval =
			ply_ptr->lasttime[LT_DINVI].interval =
			ply_ptr->lasttime[LT_INVIS].interval = 
			ply_ptr->lasttime[LT_KNOWA].interval =
			ply_ptr->lasttime[LT_DMAGI].interval = 0;
			break;
                            
		case TRAP_NAKED:	
			print(fd, "You are covered in oozing green slime.\n");
			print(fd, "All your possessions dissolve away.\n");
			broadcast_rom(fd, ply_ptr->rom_num,
				"A oozing green slime envelops %m.", ply_ptr);
			lose_all(ply_ptr);
			break;

		case TRAP_ALARM:
			print(fd, "You set off an alarm!\n");
			print(fd, "You hope there aren't any guards around.\n\n");
			broadcast_rom(fd, ply_ptr->rom_num, 
			"%M sets off an alarm!\n", ply_ptr);
			load_rom(rom_ptr->trapexit, &new_rom);
			add_permcrt_rom(new_rom);
			cp = new_rom->first_mon;
			while(cp) {
				tmp_crt = cp->crt;
				cp = cp->next_tag;
				if(F_ISSET(tmp_crt, MPERMT)){
			if(new_rom->first_ply)
			broadcast_rom(tmp_crt->fd, tmp_crt->rom_num, 
			"%M hears an alarm and leaves to investigate.",tmp_crt);
			else
       		                add_active(tmp_crt); 
			F_CLR(tmp_crt,MPERMT);
			F_SET(tmp_crt,MAGGRE);
                        die_perm_crt(tmp_crt);
                     	del_crt_rom(tmp_crt, new_rom);
                        add_crt_rom(tmp_crt, rom_ptr, 1);
			broadcast_rom(tmp_crt->fd, tmp_crt->rom_num, 
			"%M comes to investigate the alarm.", tmp_crt);
					}
				}
				break;
	}
	return;


}

/**********************************************************************/
/*				count_ply			      */
/**********************************************************************/

/* This function counts the number of (non-DM-invisible) players in a */
/* room and returns that number.				      */

int count_vis_ply(rom_ptr)
room	*rom_ptr;
{
	ctag	*cp;
	int	num = 0;

	cp = rom_ptr->first_ply;
	while(cp) {
		if(!F_ISSET(cp->crt, PDMINV)) num++;
		cp = cp->next_tag;
	}

	return num;
}
