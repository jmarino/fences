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
	struct geometry *geo;	/* geometry of board */
	int *state;		/* state of lines */
	int nlines;		/* number of lines ON in loop */
	gboolean *mask;	/* indicates lines that can be changed */
	struct square *sq;	/* square where we are currently growing */
	int nexits;		/* lines ON available out of current square */
	int navailable;	/* how many lines are changeable */
};



/*
 * Check if square has non incoming line corners on any of its vertices
 * Note: routine ignores any side set on square
 *	TRUE: a corner was found
 *	FALSE: no corner
 */
static gboolean
square_has_corner(struct square *sq, struct loop *loop)
{
	int i, j;
	struct vertex *vertex;
	struct line *lin;
	int count;

	for(i=0; i < sq->nvertex; ++i) {
		vertex= sq->vertex[i];
		count= 0;
		for(j=0; j < vertex->nlines; ++j) {
			lin= vertex->lines[j];
			/* ignore lines that belong to square 'sq' */
			if (lin->sq[0] == sq ||
			    (lin->nsquares == 2 && lin->sq[1] == sq))
				continue;
			/* count lines ON for vertex */
			if (loop->state[ lin->id ] == LINE_ON) {
				if (count == 1) return TRUE;
				++count;
			}
		}
	}
	return FALSE;
}


/*
 * Count number of disconnected branches around square.
 * Basically it counts number of OFF -> ON or ON -> OFF jumps as it
 * travels around the square.
 * The number of branches is njumps/2.
 */
static int
branches_on_square(struct square *sq, struct loop *loop)
{
	int i;
	int njumps=0;
	int prev_on;

	/* count number of jumps */
	prev_on= (loop->state[ sq->sides[sq->nsides - 1]->id ] == LINE_ON);
	for(i=0; i < sq->nsides; ++i) {
		if (loop->state[ sq->sides[i]->id ] == LINE_ON) {
			if (!prev_on) {
				++njumps;
				prev_on= TRUE;
			}
		} else {
			if (prev_on) {
				++njumps;
				prev_on= FALSE;
			}
		}
	}

	return njumps/2;
}


/*
 * Is square availabe for growth
 *	- All sides have to be masked available
 *	- Number of OFF lines must be >= number of ON lines
 *	- Must NOT have a non-incoming line corner
 *	- Square must only touch loop in one region
 */
static gboolean
is_square_available(struct square *sq, struct loop *loop, int index)
{
	int i;
	gboolean res;
	int on=0;

	/* check that all sides in square are available */
	for(i=0; i < sq->nsides; ++i) {
		if (!loop->mask[ sq->sides[i]->id ])
			return FALSE;
	}

	/* square must have more OFF lines than ON to be eligible */
	for(i=0; i < sq->nsides; ++i) {
		if (loop->state[ sq->sides[i]->id ]== LINE_ON)
			++on;
		else
			--on;
	}
	if (on > 0) return FALSE;

	/* one of square's vertices is a line corner -> skip */
	loop->state[index]= LINE_OFF;
	res= square_has_corner(sq, loop);
	loop->state[index]= LINE_ON;
	if (res) {
		return FALSE;
	}

	/* more than one loop branch touches square -> skip */
	if (branches_on_square(sq, loop) > 1) {
		return FALSE;
	}

	return TRUE;
}


/*
 * Toggle lines around square
 */
static void
toggle_square_lines(struct loop *loop, struct square *sq)
{
	int i;
	int id;

	/* go around sides of square toggling lines */
	for(i=0; i < sq->nsides; ++i) {
		id= sq->sides[i]->id;
		if (loop->state[id] == LINE_ON) {
			loop->state[id]= LINE_OFF;
			--loop->nlines;
			if (loop->mask[id]) {
				--loop->navailable;
			}
		} else {
			loop->state[id]= LINE_ON;
			++loop->nlines;
			++loop->navailable;
		}
		loop->mask[id]= TRUE;
	}
}


/*
 * Find a new line on current loop to start growing an arm
 */
static void
loop_find_new_shoulder(struct loop *loop)
{
	int i;
	int index=0;
	int count;
	struct line *lin;
	struct square *sq;

	while(loop->nexits <= 0 && loop->navailable > 0) {
		/* select a line from navailable */
		count= g_random_int_range(0, loop->navailable);
		for(i=0; i < loop->geo->nlines; ++i) {
			if (loop->state[i] == LINE_ON && loop->mask[i])
				--count;
			if (count < 0) {
				index= i;
				break;
			}
		}
		lin= loop->geo->lines + index;

		/* select random square out of chosen line */
		count= g_random_int_range(0, lin->nsquares);
		for (i=0; i < lin->nsquares; ++i) {
			if (is_square_available(lin->sq[count], loop, index))
				break;
			count= (count +1)%lin->nsquares;
		}

		/* line has no valid squares -> invalidate */
		if (i == lin->nsquares) {
			loop->mask[index]= FALSE;
			--loop->navailable;
			continue;
		}

		/* set new found square as growing square */
		sq= lin->sq[count];
		toggle_square_lines(loop, sq);
		loop->sq= sq;
		loop->nexits= 0;
		for(i=0; i< sq->nsides; ++i) {
			if (loop->state[ sq->sides[i]->id ] == LINE_ON &&
			    loop->mask[ sq->sides[i]->id ])
				++loop->nexits;
		}
	}
}


/*
 * Count squares not touching any line (zero squares)
 */
static int
count_zero_squares(struct loop *loop)
{
	int i, j;
	int count= 0;
	struct square *sq;

	for(i=0; i < loop->geo->nsquares; ++i) {
		sq= loop->geo->squares + i;
		for(j=0; j < sq->nsides; ++j)  {
			if (loop->state[sq->sides[j]->id] == LINE_ON)
				break;
		}
		if (j == sq->nsides) ++count;
	}
	return count;
}


/*
 * Build a single loop on a board
 * Loop is built in loop structure, must be copied to a game structure later.
 * Returns 0: if new loop was successfully created
 * Returns 1: if problems, infinite loop -> must start from scratch
 */
static int
build_loop(struct loop *loop, gboolean trace)
{
	int i;
	int index=0;
	int count;
	struct square *sq;
	struct line *lin;
	int num_resets=0;

	while(loop->navailable && num_resets < 10) {
		/* do we need to choose a new arm starting point? */
		if (loop->nexits <= 0) {
			loop_find_new_shoulder(loop);
			goto end_of_loop;
		}

		/* select a line around current square */
		count= g_random_int_range(0, loop->nexits);
		for(i=0; i < loop->sq->nsides; ++i) {
			index= loop->sq->sides[i]->id;
			if (loop->state[index] == LINE_ON && loop->mask[index])
				--count;
			if (count < 0)
				break;
		}
		lin= loop->geo->lines + index;

		/* we're growing from a square, so line better have 2 squares */
		if (lin->nsquares == 1) {
			loop->mask[index]= FALSE;
			--loop->navailable;
			--loop->nexits;
			goto end_of_loop;
			//continue;
		}

		/* pick next square to grow in */
		sq= (lin->sq[0] != loop->sq) ? lin->sq[0] : lin->sq[1];
		if (is_square_available(sq, loop, index) == FALSE) {
			loop->mask[index]= FALSE;
			--loop->navailable;
			--loop->nexits;
			goto end_of_loop;
		}

		toggle_square_lines(loop, sq);
		loop->sq= sq;
		loop->nexits= 0;
		for(i=0; i< sq->nsides; ++i) {
			if (loop->state[ sq->sides[i]->id ] == LINE_ON &&
			    loop->mask[ sq->sides[i]->id ])
				++loop->nexits;
		}

		end_of_loop:

		/* Are we done? Check for a decent loop. */
		if (loop->navailable <= 0) {
			/* count number of zeros, reset mask if >= 15% */
			count= count_zero_squares(loop);
			if ((100.0*count)/loop->geo->nsquares >= 15.0) {
				++num_resets;	// record the reset
				loop->navailable= 0;
				for(i=0; i < loop->geo->nlines; ++i) {
					/* don't touch OFF lines, preserve
					 artificial zero squares */
					if (loop->state[i] == LINE_ON) {
						loop->mask[i]= TRUE;
						++loop->navailable;
					}
				}
			}
		}
		if (trace) break;
	}

	/* too many resets, we exited to avoid infinite loop */
	if (num_resets >= 10) {
		g_debug("build-loop: infinite loop stopped!!");
		return TRUE;	/* signal problem */
	}

	return FALSE;
}


/*
 * Create and initialize new loop structure
 */
static void
initialize_loop(struct loop *loop)
{
	struct square *sq;
	struct vertex *vertex;
	struct geometry *geo=loop->geo;
	int i, j;
	int nzeros;

	/* init lines to OFF */
	loop->nlines= 0;
	for(i=0; i < geo->nlines; ++i)
		loop->state[i]= LINE_OFF;

	/* all lines initially allowed */
	for(i=0; i < geo->nlines; ++i) {
		loop->mask[i]= TRUE;
	}
	loop->navailable= 0;

	/* force from 1 to 4 squares to have 0 lines,
	   we don't check for repetitions, if they happen too bad */
	nzeros= (int)(geo->nsquares*3.0/100.0);		// 3%
	if (nzeros == 0) nzeros= 1;
	else if (nzeros > 4) nzeros= 4;
	for(i=0; i < nzeros; ++i) {
		sq= geo->squares +  g_random_int_range(0, geo->nsquares);
		for(j=0; j < sq->nsides; ++j)
			loop->mask[sq->sides[j]->id]= FALSE;
	}

	/* select a ramdom square to start the loop (not touching the 0 square) */
	for(;;) {
		sq= geo->squares +  g_random_int_range(0, geo->nsquares);
		for(i=0; i < sq->nvertex; ++i) {
			vertex= sq->vertex[i];
			for(j=0; j < vertex->nlines; ++j) {
				if (loop->mask[vertex->lines[j]->id] == FALSE)
					break;
			}
			if (j < vertex->nlines) break;
		}
		if (i == sq->nsides) break;
	}

	/* set lines around starting square */
	for(i=0; i < sq->nsides; ++i) {
		loop->state[sq->sides[i]->id]= LINE_ON;
		loop->mask[sq->sides[i]->id]= TRUE;
	}
	loop->nlines= sq->nsides;
	loop->navailable= sq->nsides;
	loop->sq= sq;
	loop->nexits= sq->nsides;
}


/*
 * Allocate loop structure
 */
static struct loop*
allocate_loop(struct geometry *geo)
{
	struct loop *loop;

	/* alloc and init loop structure */
	loop= (struct loop*)g_malloc(sizeof(struct loop));
	loop->geo= (struct geometry *)geo;
	loop->state= (int*)g_malloc(geo->nlines*sizeof(int));
	loop->mask= (gboolean*)g_malloc(geo->nlines*sizeof(gboolean));

	return loop;
}

/*
 * Free memory used by loop
 */
static void
destroy_loop(struct loop *loop)
{
	g_free(loop->mask);
	g_free(loop->state);
	g_free(loop);
}


/*
 * Build new loop
 * Return the loop in given game structure
 * It allows trace mode
 */
void
build_new_loop(struct geometry *geo, struct game *game, gboolean trace)
{
	int i;
	static struct loop *loop;
	static gboolean first_time=TRUE;

	if (first_time || trace == FALSE) {
		loop= allocate_loop(geo);
		initialize_loop(loop);
		first_time= FALSE;
	}

	/* keep trying until a good loop is produced */
	while (build_loop(loop, trace) == 1 && trace == FALSE) {
		/* blank loop and start from scratch */
		initialize_loop(loop);
	}

	/* copy loop on to game data */
	for(i=0; i < geo->nlines; ++i) {
		game->states[i]= loop->state[i];
		//if (loop->state[i] == LINE_ON && loop->mask[i] == FALSE)
		//	game->states[i]= LINE_CROSSED;
	}

	if (!trace) {
		destroy_loop(loop);
	}
}
