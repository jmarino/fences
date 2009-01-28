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

#include <glib.h>
#include <string.h>
#include <stdio.h>

#include "gamedata.h"



struct loop {
	struct geometry *geo;
	int *state;
	int nlines;
	gboolean *mask; /* indicates lines that can be changed */
	int navailable; /* how many lines are changeable*/
};



/*
 * Check if square has non incoming line corners on any of its vertices
 * Note: routine ignores any side set on square
 * 	TRUE: a corner was found
 * 	FALSE: no corner
 */ 
static gboolean
square_has_corner(struct square *sq, struct loop *loop)
{
	int i, j, k;
	struct vertex *vertex;
	int count;
	gboolean skip;

	for(i=0; i < sq->nvertex; ++i) {
		vertex= sq->vertex[i];
		count= 0;
		for(j=0; j < vertex->nlines; ++j) {
			/* ignore square sides */
			skip= FALSE;
			for(k=0; k < sq->nsides; ++k) {
				if (sq->sides[k] == vertex->lines[j]) {
					skip= TRUE;
					break;
				}
			}
			if (skip) continue;
			if (loop->state[ vertex->lines[j]->id ] == LINE_ON) {
				if (count == 1) return TRUE;
				++count;
			}
		}
	}
	return FALSE;
}


/*
 * Count contiguos lines
 * Returns 0 if a discontinuous fragment is found
 */ 
static int
count_contiguous_lines(struct square *sq, struct loop *loop)
{
	int i;
	int total_on=0;
	int count=0, max=0;

	for(i=0; i < sq->nsides; ++i) {
		if (loop->state[ sq->sides[i]->id ] == LINE_ON) {
			++total_on;
			++count;
			if (count > max) max= count;
		} else count= 0;
	}
	if (max != total_on) return 0;
	return max;
}


/*
 * Is square availabe for growth
 * 	- All sides have to be masked available
 * 	- Must NOT have a non-incoming line corner
 * 	- Must have a contiguous line around of length >1 <sq->nsides/2
 */ 
static gboolean
is_square_available(struct square *sq, struct loop *loop, int index)
{
	int i;
	int num_lines;
	gboolean res;
	
	/* check all sides in square are available */
	for(i=0; i < sq->nsides; ++i) {
		if (!loop->mask[ sq->sides[i]->id ])
			break;
	}
	if (i < sq->nsides) {
		return FALSE;
	}
			
	/* one of square's vertices is a line corner -> skip */
	loop->state[index]= LINE_OFF;
	res= square_has_corner(sq, loop);
	loop->state[index]= LINE_ON;
	if (res) {
		return FALSE;
	}
				
	/* count contiguous lines on square */
	/* it should be between 1 and sq->nsides/2 */
	num_lines= count_contiguous_lines(sq, loop);
	if (num_lines == 0 || num_lines > sq->nsides/2) {
		return FALSE;
	}
	return TRUE;
}


/*
 * Build a single loop on a board
 * It sets lines in a separate loop structure
 */
struct loop*
build_loop(const struct geometry *geo)
{
	int i, j;
	int index=0;
	int count;
	static struct loop *loop=NULL;
	struct square *sq;
	struct line *lin;
	int prev_navailable=0;
	int stuck=0;
	int num_stuck=0;

	/* alloc and init loop structure */
	loop= (struct loop*)g_malloc(sizeof(struct loop));
	loop->geo= (struct geometry *)geo;
	loop->state= (int*)g_malloc(geo->nlines*sizeof(int));
	loop->nlines= 0;
	for(i=0; i < geo->nlines; ++i)
		loop->state[i]= LINE_OFF;

	/* allocate loop->mask: indicates lines available to be changed */
	loop->mask= (gboolean*)g_malloc(geo->nlines*sizeof(gboolean));
	for(i=0; i < geo->nlines; ++i)
		loop->mask[i]= TRUE;
	loop->navailable= 0;
	
	/* select a ramdom square to start the loop */
	sq= geo->squares +  g_random_int_range(0, geo->nsquares);
	for(i=0; i < sq->nsides; ++i) {
		loop->state[sq->sides[i]->id]= LINE_ON;
	}
	loop->nlines= sq->nsides;
	loop->navailable= sq->nsides;
	
	/* extend current loop */
	while(num_stuck < 3) {
		/* pick random line out of 'loop->navailable' */
		count= g_random_int_range(0, loop->navailable);
		for(i=0; i < geo->nlines; ++i) {
			if (loop->state[i] == LINE_ON &&
			    loop->mask[i]) --count;
			if (count < 0) {
				index= i;
				break;
			}
		}
		lin= geo->lines + index;

		/* 1st check to see if lin is suitable (necessary but not sufficient) */
		/*    ->  line has 2 squares associated */
		if ( lin->nsquares != 2 ) {
			loop->mask[index]= FALSE;	/* disable line */
			--loop->navailable;
			goto next_iteration;
		}
		
		/* pick one of the 2 squares at random */
		count= g_random_int_range(0, 2);

		/* check if either square is available, starting at 'count' */
		for(j=0; j < 2; ++j) {
			sq= lin->sq[count];

			/* is square available? */
			if (is_square_available(sq, loop, index)) {
				break;	// square is good
			}
			 sq= NULL;
			 count= (count + 1) % 2;
		}
		if (sq == NULL) {	// line not suitable
			loop->mask[index]= FALSE;	/* disable line */
			--loop->navailable;
			goto next_iteration;
		}
		
		/* go around sides of square switching lines */
		for(i=0; i < sq->nsides; ++i) {
			if (loop->state[ sq->sides[i]->id ] == LINE_ON) {
				loop->state[ sq->sides[i]->id ]= LINE_OFF;
				loop->mask[ sq->sides[i]->id ]= FALSE;
				--loop->nlines;
				--loop->navailable;
			} else {
				loop->state[ sq->sides[i]->id ]= LINE_ON;
				loop->mask[ sq->sides[i]->id ]= TRUE;
				++loop->nlines;
				++loop->navailable;
			}
		}

	next_iteration:
		if (loop->navailable == prev_navailable) {
			++stuck;
		} else stuck= 0;
		prev_navailable= loop->navailable;

		if (stuck > 3 || loop->navailable == 0) {
			++num_stuck;
			/* make every line available again */
			loop->navailable= 0;
			for(i=0; i < geo->nlines; ++i) {
				loop->mask[i]= TRUE;
				if (loop->state[i] == LINE_ON)
					++loop->navailable;
			}
		}
	}
	g_free(loop->mask);
	
	return loop;
}


void
try_loop(struct geometry *geo, struct game *game)
{
	struct loop *loop;
	int i;
	
	loop= build_loop(geo);
	
	/* copy loop on to game data */
	for(i=0; i < geo->nlines; ++i) {
		game->states[i]= loop->state[i];
	}
	
	g_free(loop->state);
	g_free(loop);
}
