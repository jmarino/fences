/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */

#ifndef __INCLUDED_BRUTE_FORCE_H__
#define __INCLUDED_BRUTE_FORCE_H__


/* contains data to record each step taken (useful to backtrack) */
/* WARNING: 'int routes' limits number of possible line connections to 32 (or 64)
 * (seems reasonable) */
struct step {
	int id;		/* id of line just added in this step */
	int direction;	/* direction of flow on current line of this step */
	int routes;	/* **NOTE** this is highly arch dependant */
};

/* contains stack of steps */
struct stack {
	struct step *step;
	int *ini_states;
	int pos;
	int size;
};



gboolean brute_force_solve(struct solution *sol, struct stack *stack, 
			   gboolean trace_mode);
int brute_force_test(struct geometry *geo, struct game *game);


#endif
