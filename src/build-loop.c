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
	struct tile *tile;	/* tile where we are currently growing */
	int nexits;		/* lines ON available out of current tile */
	int navailable;	/* how many lines are changeable */
};



/*
 * Check if tile has non incoming line corners on any of its vertices
 * Note: routine ignores any side set on tile
 *	TRUE: a corner was found
 *	FALSE: no corner
 */
static gboolean
tile_has_corner(struct tile *tile, struct loop *loop)
{
	int i, j;
	struct vertex *vertex;
	struct line *lin;
	int count;

	for(i=0; i < tile->nvertex; ++i) {
		vertex= tile->vertex[i];
		count= 0;
		for(j=0; j < vertex->nlines; ++j) {
			lin= vertex->lines[j];
			/* ignore lines that belong to tile 'tile' */
			if (lin->tiles[0] == tile ||
			    (lin->ntiles == 2 && lin->tiles[1] == tile))
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
 * Count number of disconnected branches around tile.
 * Basically it counts number of OFF -> ON or ON -> OFF jumps as it
 * travels around the tile.
 * The number of branches is njumps/2.
 */
static int
branches_on_tile(struct tile *tile, struct loop *loop)
{
	int i;
	int njumps=0;
	int prev_on;

	/* count number of jumps */
	prev_on= (loop->state[ tile->sides[tile->nsides - 1]->id ] == LINE_ON);
	for(i=0; i < tile->nsides; ++i) {
		if (loop->state[ tile->sides[i]->id ] == LINE_ON) {
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
 * Is tile availabe for growth
 *	- All sides have to be masked available
 *	- Number of OFF lines must be >= number of ON lines
 *	- Must NOT have a non-incoming line corner
 *	- Tile must only touch loop in one region
 */
static gboolean
is_tile_available(struct tile *tile, struct loop *loop, int index)
{
	int i;
	gboolean res;
	int on=0;

	/* check that all sides in tile are available */
	for(i=0; i < tile->nsides; ++i) {
		if (!loop->mask[ tile->sides[i]->id ])
			return FALSE;
	}

	/* tile must have more OFF lines than ON to be eligible */
	for(i=0; i < tile->nsides; ++i) {
		if (loop->state[ tile->sides[i]->id ]== LINE_ON)
			++on;
		else
			--on;
	}
	if (on > 0) return FALSE;

	/* one of tile's vertices is a line corner -> skip */
	loop->state[index]= LINE_OFF;
	res= tile_has_corner(tile, loop);
	loop->state[index]= LINE_ON;
	if (res) {
		return FALSE;
	}

	/* more than one loop branch touches tile -> skip */
	if (branches_on_tile(tile, loop) > 1) {
		return FALSE;
	}

	return TRUE;
}


/*
 * Toggle lines around tile
 */
static void
toggle_tile_lines(struct loop *loop, struct tile *tile)
{
	int i;
	int id;

	/* go around sides of tile toggling lines */
	for(i=0; i < tile->nsides; ++i) {
		id= tile->sides[i]->id;
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
	struct tile *tile;

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

		/* select random tile out of chosen line */
		count= g_random_int_range(0, lin->ntiles);
		for (i=0; i < lin->ntiles; ++i) {
			if (is_tile_available(lin->tiles[count], loop, index))
				break;
			count= (count +1)%lin->ntiles;
		}

		/* line has no valid tiles -> invalidate */
		if (i == lin->ntiles) {
			loop->mask[index]= FALSE;
			--loop->navailable;
			continue;
		}

		/* set new found tile as growing tile */
		tile= lin->tiles[count];
		toggle_tile_lines(loop, tile);
		loop->tile= tile;
		loop->nexits= 0;
		for(i=0; i< tile->nsides; ++i) {
			if (loop->state[ tile->sides[i]->id ] == LINE_ON &&
			    loop->mask[ tile->sides[i]->id ])
				++loop->nexits;
		}
	}
}


/*
 * Count tiles not touching any line (zero tiles)
 */
static int
count_zero_tiles(struct loop *loop)
{
	int i, j;
	int count= 0;
	struct tile *tile;

	for(i=0; i < loop->geo->ntiles; ++i) {
		tile= loop->geo->tiles + i;
		for(j=0; j < tile->nsides; ++j)  {
			if (loop->state[tile->sides[j]->id] == LINE_ON)
				break;
		}
		if (j == tile->nsides) ++count;
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
	struct tile *tile;
	struct line *lin;
	int num_resets=0;

	while(loop->navailable && num_resets < 10) {
		/* do we need to choose a new arm starting point? */
		if (loop->nexits <= 0) {
			loop_find_new_shoulder(loop);
			goto end_of_loop;
		}

		/* select a line around current tile */
		count= g_random_int_range(0, loop->nexits);
		for(i=0; i < loop->tile->nsides; ++i) {
			index= loop->tile->sides[i]->id;
			if (loop->state[index] == LINE_ON && loop->mask[index])
				--count;
			if (count < 0)
				break;
		}
		lin= loop->geo->lines + index;

		/* we're growing from a tile, so line better have 2 tiles */
		if (lin->ntiles == 1) {
			loop->mask[index]= FALSE;
			--loop->navailable;
			--loop->nexits;
			goto end_of_loop;
			//continue;
		}

		/* pick next tile to grow in */
		tile= (lin->tiles[0] != loop->tile) ? lin->tiles[0] : lin->tiles[1];
		if (is_tile_available(tile, loop, index) == FALSE) {
			loop->mask[index]= FALSE;
			--loop->navailable;
			--loop->nexits;
			goto end_of_loop;
		}

		toggle_tile_lines(loop, tile);
		loop->tile= tile;
		loop->nexits= 0;
		for(i=0; i< tile->nsides; ++i) {
			if (loop->state[ tile->sides[i]->id ] == LINE_ON &&
			    loop->mask[ tile->sides[i]->id ])
				++loop->nexits;
		}

		end_of_loop:

		/* Are we done? Check for a decent loop. */
		if (loop->navailable <= 0) {
			/* count number of zeros, reset mask if >= 15% */
			count= count_zero_tiles(loop);
			if ((100.0*count)/loop->geo->ntiles >= 15.0) {
				++num_resets;	// record the reset
				loop->navailable= 0;
				for(i=0; i < loop->geo->nlines; ++i) {
					/* don't touch OFF lines, preserve
					 artificial zero tiles */
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
	struct tile *tile;
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

	/* force from 1 to 4 tiles to have 0 lines,
	   we don't check for repetitions, if they happen too bad */
	nzeros= (int)(geo->ntiles*3.0/100.0);		// 3%
	if (nzeros == 0) nzeros= 1;
	else if (nzeros > 4) nzeros= 4;
	for(i=0; i < nzeros; ++i) {
		tile= geo->tiles +  g_random_int_range(0, geo->ntiles);
		for(j=0; j < tile->nsides; ++j)
			loop->mask[tile->sides[j]->id]= FALSE;
	}

	/* select a ramdom tile to start the loop (not touching the 0 tile) */
	for(;;) {
		tile= geo->tiles +  g_random_int_range(0, geo->ntiles);
		/* check that lines around tile are not already disabled */
		for(i=0; i < tile->nsides; ++i) {
			if (loop->mask[tile->sides[i]->id] == FALSE)
				break;
		}
		if (i == tile->nsides) break;	// none disabled -> done
	}

	/* set lines around starting tile */
	for(i=0; i < tile->nsides; ++i) {
		loop->state[tile->sides[i]->id]= LINE_ON;
		loop->mask[tile->sides[i]->id]= TRUE;
	}
	loop->nlines= tile->nsides;
	loop->navailable= tile->nsides;
	loop->tile= tile;
	loop->nexits= tile->nsides;
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
