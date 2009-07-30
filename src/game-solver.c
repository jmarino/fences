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
#include "game-solver.h"



/*
 * Description:
 * We're given a game state and a geometry
 * Find solution
 */

/*
 * NOTE: Note about tile_done
 * Both tiles with and without a number can be marked as 'undone'.
 * So, tile_done= FALSE doesn't guarantee a number tile.
 */



/*
 * Find line that touches both given tiles
 * NULL: if tiles don't share any line
 */
static struct line*
find_shared_side(struct tile *tile1, struct tile *tile2, int *i, int *j)
{
	for(*i=0; *i < tile1->nsides; ++(*i)) {
		for(*j=0; *j < tile1->nsides; ++(*j)) {
			if (tile1->sides[*i] == tile2->sides[*j]) {
				return tile1->sides[*i];
			}
		}
	}
	*i= *j= 0;
	return NULL;
}


/*
 * Check if line touches tile
 */
static inline gboolean
line_touches_tile(struct line *lin, struct tile *tile)
{
	if (lin->tiles[0] == tile ||
		(lin->ntiles == 2 && lin->tiles[1] == tile)) return TRUE;

	return FALSE;
}


/*
 * Is vertex a cornered with respect to one tile
 * I.e. the vertex has no exits outside this tile
 */
static gboolean
is_vertex_cornered(struct solution *sol, struct tile *tile,
		   struct vertex *vertex)
{
	int i;

	/* check lines coming out of vertex that don't belong to tile */
	for(i=0; i < vertex->nlines; ++i) {
		if (line_touches_tile(vertex->lines[i], tile))
			continue;
		/* if line ON or OFF, vertex not cornered */
		if (STATE(vertex->lines[i]) != LINE_CROSSED)
			return FALSE;
	}
	return TRUE;
}


/*
 * Find if lines are only 1 line away (used for bottleneck detection)
 * dir1 & dir2 point towards the loose end of lines
 * Return line connecting the two lines
 * NULL if lines are farther away
 */
static struct line *
find_line_connecting_lines(struct line *end1, int dir1,
			    struct line *end2, int dir2)
{
	struct vertex *vertex;
	int i;

	if (dir1 == DIRECTION_IN) vertex= end1->ends[0];
	else vertex= end1->ends[1];

	if (dir2 == DIRECTION_IN) {
		for(i=0; i < end2->nin; ++i) {
			if (end2->in[i]->ends[0] == vertex ||
			    end2->in[i]->ends[1] == vertex) break;
		}
		if (i < end2->nin)
			return end2->in[i];
	} else {
		for(i=0; i < end2->nout; ++i) {
			if (end2->out[i]->ends[0] == vertex ||
			    end2->out[i]->ends[1] == vertex) break;
		}
		if (i < end2->nout)
			return end2->out[i];
	}
	return NULL;
}


/*
 * Go through all tiles and cross sides of tiles with a 0
 * Update sol->nchanges to number of lines crossed out
 */
void
solve_zero_tiles(struct solution *sol)
{
	int i, j;
	struct tile *tile;
	struct geometry *geo=sol->geo;

	sol->nchanges= sol->ntile_changes= 0;
	for(i=0; i < geo->ntiles; ++i) {
		/* only care about unhandled 0 tiles */
		if (sol->numbers[i] != 0 || sol->tile_done[i]) continue;
		/* cross sides of tile */
		sol->tile_done[i]= TRUE;	// mark tile as handled
		++sol->num_tile_done;
		sol->tile_changes[sol->ntile_changes]= i;
		++sol->ntile_changes;
		tile= geo->tiles + i;
		for(j=0; j < tile->nsides; ++j)
			solve_set_line_cross(sol, tile->sides[j]);
	}
}


/*
 * Iterate over all unhandled tiles to find:
 *  - Numbered tiles with enough crossed sides that a solution is trivial.
 *  - Any tile with all lines either ON or CROSSED -> mark it handled.
 */
void
solve_trivial_tiles(struct solution *sol)
{
	int i, j;
	struct tile *tile;
	struct geometry *geo=sol->geo;

	sol->nchanges= sol->ntile_changes= 0;
	for(i=0; i < geo->ntiles; ++i) {
		/* only unhandled tiles */
		if (sol->tile_done[i])
			continue;

		tile= geo->tiles + i;
		/* enough lines crossed? -> set ON the OFF ones */
		if ( tile->nsides - sol->tile_count[i].cross == NUMBER(tile) ) {
			sol->tile_done[i]= TRUE;
			++sol->num_tile_done;
			sol->tile_changes[sol->ntile_changes]= i;
			++sol->ntile_changes;
			for(j=0; j < tile->nsides; ++j) {
				solve_set_line_on(sol, tile->sides[j]);
			}
		}
		/* only allow one trivial tile to be set at a time */
		if (sol->nchanges > 0) break;
	}
}


/*
 * Iterate over all vertex to find vertices with one line ON and only one other
 * possible line available, i.e., (nlines - 2) CROSSED and 1 OFF
 * Then set remaining line.
 */
void
solve_trivial_vertex(struct solution *sol)
{
	int i, j;
	int lines_off;
	struct vertex *vertex;
	struct geometry *geo=sol->geo;

	sol->nchanges= sol->ntile_changes= 0;
	for(i=0; i < geo->nvertex; ++i) {
		/* unfinished vertices with just one incoming ON line */
		if (sol->vertex_done[i] || sol->vertex_count[i].on != 1) continue;
		vertex= geo->vertex + i;
		lines_off= vertex->nlines -
			(sol->vertex_count[i].on + sol->vertex_count[i].cross);

		if (lines_off != 1) continue;

		/* here we have: vertex with one incoming and only one available exit */
		/* find OFF line and set it ON */
		for(j=0; j < vertex->nlines; ++j) {
			if (STATE(vertex->lines[j]) == LINE_OFF) {
				solve_set_line_on(sol, vertex->lines[j]);
				sol->vertex_done[i]= TRUE;
				++sol->num_vertex_done;
				break;
			}
		}

		/* only allow one trivial vertex to be set at a time */
		if (sol->nchanges > 0) break;
	}
}


/*
 * Find tiles with a number == nsides - 1
 * Check around all its vertices to see if it neighbors another tile
 * with number == nsides
 * If another found: determine if they're side by side or diagonally and set
 * lines accordingly
 */
void
solve_maxnumber_tiles(struct solution *sol)
{
	int i, j, k, k2;
	struct tile *tile, *tile2=NULL;
	int pos1, pos2;
	struct vertex *vertex;
	struct line *lin;
	struct geometry *geo=sol->geo;
	int cache;

	sol->nchanges= sol->ntile_changes= 0;
	/* iterate over all tiles */
	for(i=0; i < geo->ntiles; ++i) {
		tile= geo->tiles + i;
		/* ignore handled tiles or with number != sides - 1 */
		if (sol->tile_done[i] || sol->numbers[i] != tile->nsides - 1)
			continue;

		/* inspect vertices of tile */
		for(j=0; j < tile->nvertex; ++j) {
			vertex= tile->vertex[j];
			cache= sol->nchanges;
			/* vertex is in a corner -> enable lines touching it */
			if (vertex->nlines == 2) {
				solve_set_line_on(sol, vertex->lines[0]);
				solve_set_line_on(sol, vertex->lines[1]);
				/* any changes? -> record tile */
				if (sol->nchanges > cache) {
					sol->tile_changes[sol->ntile_changes]= i;
					++sol->ntile_changes;
				}
				continue;
			}
			/* at this point vertex touches at least 2 tiles (not in a corner) */
			/* find neighbor tile with MAX_NUMBER */
			for(k=0; k < vertex->ntiles; ++k) {
				tile2= vertex->tiles[k];
				if (tile2 == tile) continue; // ignore current tile
				if ( MAX_NUMBER(tile2) )
					break;
			}
			if (k == vertex->ntiles) // no max neighbor
				continue;

			/* find if the two MAX tiles share a line */
			lin= find_shared_side(tile, tile2, &pos1, &pos2);
			if (lin != NULL) {	// shared side: side by side tiles
				solve_set_line_on(sol, lin);
				/* set all lines around tiles except the ones touching lin */
				for(k=2; k < tile->nsides - 1; ++k) {
					k2= (pos1 + k) % tile->nsides;
					solve_set_line_on(sol, tile->sides[k2]);
				}
				for(k=2; k < tile2->nsides - 1; ++k) {
					k2= (pos2 + k) % tile2->nsides;
					solve_set_line_on(sol, tile2->sides[k2]);
				}
				/* cross any lines from vertex that are not part of tile or tile2 */
				for(k=0; k < vertex->nlines; ++k) {
					if (line_touches_tile(vertex->lines[k], tile) ||
					    line_touches_tile(vertex->lines[k], tile2))
						continue;
					solve_set_line_cross(sol, vertex->lines[k]);

				}
				/* lines changed -> record tile */
				if (sol->nchanges > cache) {
					sol->tile_changes[sol->ntile_changes]= i;
					++sol->ntile_changes;
				}
			} else {	// no shared side: diagonally opposed tiles
				/* set all lines around tiles that don't touch vertex */
				for(k=0; k < tile->nsides; ++k) {
					if (tile->sides[k]->ends[0] == vertex ||
					    tile->sides[k]->ends[1] == vertex) continue;
					solve_set_line_on(sol, tile->sides[k]);
				}
				for(k=0; k < tile2->nsides; ++k) {
					if (tile2->sides[k]->ends[0] == vertex ||
					    tile2->sides[k]->ends[1] == vertex) continue;
					solve_set_line_on(sol, tile2->sides[k]);
				}
				/* lines changed -> record tile */
				if (sol->nchanges > cache) {
					sol->tile_changes[sol->ntile_changes]= i;
					++sol->ntile_changes;
				}
			}
		}
	}
}


/*
 * Applies to tiles with a number == nsides - 1
 * If a tile with number == (nsides - 1) has an incoming line in a vertex,
 * the line must go around the tile and exit through a vertex one line away
 * from the current vertex. This translates into:
 *  - set ON all lines around tile that don't touch this vertex
 *  - line must go in to tile, can't create a corner. Thus, cross any line
 *    going out from vertex that does not go into tile
 */
void
solve_maxnumber_incoming_line(struct solution *sol)
{
	int i, j, k;
	struct tile *tile;
	struct vertex *vertex;
	struct line *lin=NULL;
	struct geometry *geo=sol->geo;
	int cache;

	sol->nchanges= sol->ntile_changes= 0;
	/* iterate over all tiles */
	for(i=0; i < geo->ntiles; ++i) {
		tile= geo->tiles + i;
		/* ignore tiles without number or number < nsides -1 */
		if (sol->numbers[i] != tile->nsides - 1 || sol->tile_done[i])
			continue;

		/* inspect vertices of tile */
		for(j=0; j < tile->nvertex; ++j) {
			vertex= tile->vertex[j];
			/* check that vertex has exactly one ON line */
			if (sol->vertex_count[vertex->id].on != 1) continue;

			/* find ON line */
			for(k=0; k < vertex->nlines; ++k) {
				if (STATE(vertex->lines[k]) == LINE_ON) {
					lin= vertex->lines[k];
					break;
				}
			}
			g_assert(k < vertex->nlines);
			/* make sure ON line is not part of tile */
			if (line_touches_tile(lin, tile)) continue;

			cache= sol->nchanges;
			/* cross lines going out from vertex that don't
			 * touch tile */
			for(k=0; k < vertex->nlines; ++k) {
				if ( !line_touches_tile(vertex->lines[k], tile) ) {
					solve_set_line_cross(sol, vertex->lines[k]);
				}
			}

			/* set lines in tile not touching vertex ON */
			for(k=0; k < tile->nsides; ++k) {
				if (tile->sides[k]->ends[0] == vertex ||
				    tile->sides[k]->ends[1] == vertex) continue;
				solve_set_line_on(sol, tile->sides[k]);
			}
			/* lines changed? record tile */
			if (sol->nchanges > cache) {
				sol->tile_changes[sol->ntile_changes]= i;
				++sol->ntile_changes;
			}
			break;
		}
	}
}


/*
 * A vertex of a tile with number == nsides - 1
 * One vertex of tile doesn't have any lines ON and all other lines of
 * tile are ON.
 * Vertex has only one possible output away from tile.
 *  - Turn exit line ON
 */
void
solve_maxnumber_exit_line(struct solution *sol)
{
	int i, j;
	int pos=0;
	int pos2=0;
	struct tile *tile;
	struct vertex *vertex;
	int nlines_off;
	struct geometry *geo=sol->geo;

	sol->nchanges= sol->ntile_changes= 0;
	/* iterate over all tiles */
	for(i=0; i < geo->ntiles; ++i) {
		tile= geo->tiles + i;
		/* ignore handled tiles, without number or number < nsides -1 */
		if (sol->numbers[i] != tile->nsides - 1 || sol->tile_done[i])
			continue;

		/* count lines OFF on tile */
		nlines_off= 0;
		for(j=0; j < tile->nsides; ++j) {
			if (STATE(tile->sides[j]) == LINE_OFF) {
				++nlines_off;
				pos2= pos;	// keep track of 2nd to last OFF line
				pos= j;		// hold pos of last OFF line
			}
		}
		/* must have 2 lines OFF */
		if (nlines_off != 2) continue;

		/* find shared vertex between pos and pos2 lines */
		if (tile->sides[pos]->ends[0] == tile->sides[pos2]->ends[0] ||
			tile->sides[pos]->ends[0] == tile->sides[pos2]->ends[1]) {
			vertex= tile->sides[pos]->ends[0];
		} else if (tile->sides[pos]->ends[1] == tile->sides[pos2]->ends[0] ||
			tile->sides[pos]->ends[1] == tile->sides[pos2]->ends[1]) {
			vertex= tile->sides[pos]->ends[1];
		} else {
			continue;	// no shared vertex, not a candidate
		}

		/* count lines OFF going out from vertex and not part of tile */
		nlines_off= 0;
		for(j=0; j < vertex->nlines; ++j) {
			/* if any line is ON, stop right here */
			if (STATE(vertex->lines[j]) == LINE_ON)
				break;
			if (STATE(vertex->lines[j]) == LINE_OFF &&
				!line_touches_tile(vertex->lines[j], tile)) {
				++nlines_off;
				pos= j;
			}
		}
		if (j < vertex->nlines || nlines_off != 1) continue;

		/* only one line OFF -> set it ON */
		solve_set_line_on(sol, vertex->lines[pos]);
		sol->tile_changes[sol->ntile_changes]= i;
		++sol->ntile_changes;
		break;
	}
}


/*
 * Find tiles with (number == nsides - 1) || (number == 1)
 * If one corner has no exit:
 *	- (nsides - 1) set both lines
 *	- (1) cross both lines
 */
void
solve_corner(struct solution *sol)
{
	int i, j, k;
	struct tile *tile;
	struct vertex *vertex;
	struct geometry *geo=sol->geo;
	int cache;

	sol->nchanges= sol->ntile_changes= 0;
	/* iterate over all tiles */
	for(i=0; i < geo->ntiles; ++i) {
		tile= geo->tiles + i;
		/* ignore handled tiles, unnumbered tiles or number != nsides -1 */
		if (sol->tile_done[i] ||
		    (sol->numbers[i] != tile->nsides - 1 && sol->numbers[i] != 1))
			continue;

		cache= sol->nchanges;
		/* inspect vertices of tile */
		for(j=0; j < tile->nvertex; ++j) {
			vertex= tile->vertex[j];

			/* check vertex is a no-exit corner */
			if (is_vertex_cornered(sol, tile, vertex) == FALSE)
				continue;
			/* ON or CROSS corner lines */
			for(k=0; k < vertex->nlines; ++k) {
				if (line_touches_tile(vertex->lines[k], tile) == FALSE)
					continue;
				if (sol->numbers[i] == 1) {
					solve_set_line_cross(sol, vertex->lines[k]);
				} else {
					solve_set_line_on(sol, vertex->lines[k]);
				}
			}
		}
		/* any line changes? record tile */
		if (sol->nchanges > cache) {
			sol->tile_changes[sol->ntile_changes]= i;
			++sol->ntile_changes;
		}
	}
}


/*
 * Find numbered tiles with (number - lines_on) == 1
 * If a vertex has one incoming line and only available lines are part of the
 * tile, cross all the rest lines of tile
 */
void
solve_tiles_net_1(struct solution *sol)
{
	int i, j, k;
	struct tile *tile;
	int num_exits;
	struct line *lin=NULL;
	struct vertex *vertex;
	struct geometry *geo=sol->geo;
	int cache;

	sol->nchanges= sol->ntile_changes= 0;
	/* iterate over all tiles */
	for(i=0; i < geo->ntiles; ++i) {
		tile= geo->tiles + i;
		/* ignore handled tiles and unnumbered tiles */
		if (sol->tile_done[i] || sol->numbers[i] == -1)
			continue;

		/* we're looking for tiles with just one line left to be set */
		if (NUMBER(tile) - sol->tile_count[i].on != 1) continue;

		cache= sol->nchanges;
		/* inspect vertices of tile */
		for(j=0; j < tile->nvertex; ++j) {
			vertex= tile->vertex[j];
			/* check we have just one line ON */
			if (sol->vertex_count[vertex->id].on != 1) continue;

			/* there must be exactly 2 OFF lines */
			num_exits= vertex->nlines -	(sol->vertex_count[vertex->id].on +
										 sol->vertex_count[vertex->id].cross);
			if (num_exits != 2) continue;

			/* final check: the 1 ON line must not be a side of the tile,
			   and the 2 OFF lines must be sides of the tile */
			num_exits= 0;
			for(k=0; k < vertex->nlines; ++k) {
				if (STATE(vertex->lines[k]) == LINE_ON) {
					lin= vertex->lines[k];
				} else if (STATE(vertex->lines[k]) == LINE_OFF &&
				    line_touches_tile(vertex->lines[k], tile)) {
					++num_exits;
				}
			}
			if (num_exits != 2 || line_touches_tile(lin, tile)) continue;

			/* cross all lines away from this vertex */
			for(k=0; k < tile->nsides; ++k) {
				if (tile->sides[k]->ends[0] == vertex ||
				    tile->sides[k]->ends[1] == vertex) continue;
				solve_set_line_cross(sol, tile->sides[k]);
			}
		}
		/* any line changes? record tile */
		if (sol->nchanges > cache) {
			sol->tile_changes[sol->ntile_changes]= i;
			++sol->ntile_changes;
		}
	}
}


/*
 * Cross out trivial lines.
 * Find vertices with 2 lines ON, vertex busy -> cross any OFF line left.
 * Find vertices with all but one crossed (0 ON and 1 OFF) -> cross it.
 * Repeat process until no more lines are crossed out.
 * Find tiles with enough lines ON, cross out any OFF lines around it.
 * **NOTE: this function should only modify 'sol->states' field, so it can be
 * used with solve strategies that do temporary changes.
 */
void
solve_cross_lines(struct solution *sol)
{
	int i, j;
	int num_off;
	struct vertex *vertex;
	struct tile *tile;
	struct geometry *geo=sol->geo;
	int old_count=-1;
	int cache;

	sol->nchanges= sol->ntile_changes= 0;
	/* go through all tiles */
	for(i=0; i < geo->ntiles; ++i) {
		/* only unhandled tiles */
		if (sol->tile_done[i]) continue;

		tile= geo->tiles + i;
		/* all sides either ON or CROSS -> tile handled */
		if (sol->tile_count[i].on + sol->tile_count[i].cross == tile->nsides) {
			sol->tile_done[i]= TRUE;
			++sol->num_tile_done;
			continue;
		}

		/* not-numbered tiles -> we want nsides - 1 ON
		   numbered tiles -> we want NUMBER lines ON */
		if (sol->numbers[i] == -1) {
			if (sol->tile_count[i].on != tile->nsides - 1) continue;
		} else {	// numbered tiles
			if (sol->tile_count[i].on != NUMBER(tile)) continue;
		}

		cache= sol->nchanges;
		/* tile is complete, cross any OFF left */
		for(j=0; j < tile->nsides; ++j) {
			solve_set_line_cross(sol, tile->sides[j]);
		}
		/* mark tile as handled */
		sol->tile_done[i]= TRUE;
		++sol->num_tile_done;
		/* any line changes? record tile */
		if (sol->nchanges > cache) {
			sol->tile_changes[sol->ntile_changes]= i;
			++sol->ntile_changes;
		}
	}

	while (old_count != sol->nchanges) {
		old_count= sol->nchanges;
		/* go through all vertices */
		for(i=0; i < geo->nvertex; ++i) {
			if (sol->vertex_done[i]) continue;
			vertex= geo->vertex + i;

			num_off= vertex->nlines - (sol->vertex_count[i].on +
									   sol->vertex_count[i].cross);
			/* no OFF lines -> vertex done */
			if (num_off == 0) {
				sol->vertex_done[i]= TRUE;
				++sol->num_vertex_done;
				continue;
			}

			/* check: vertex has 2 lines ON -> vertex is done
			   vertex has 0 lines ON and just 1 OFF -> no exit */
			if (sol->vertex_count[i].on == 2 ||
				(sol->vertex_count[i].on == 0 && num_off == 1)) {
				/* cross any line left OFF */
				for(j=0; j < vertex->nlines; ++j)
					solve_set_line_cross(sol, vertex->lines[j]);
				sol->vertex_done[i]= TRUE;
				++sol->num_vertex_done;
			}
		}
	}
}


/*
 * Assume a situation where we just found one big open loop, and not all
 * tiles have been handled. Normally, we would then cross out the
 * connecting line. However, if the only unhandled tiles are touching the
 * connecting line and would be handled by turning this line ON, we can't
 * cross it out.
 * Returns: TRUE if the situation described is detected, line should not
 * be crossed out.
 */
static gboolean
bottleneck_is_final_line(struct solution *sol, struct line *lin)
{
	struct tile *tile;
	int num=0;
	int num_unhandled=0;
	int id;
	int i;

	/* check line touches at least 1 numbered and unhandled tile */
	for(i=0; i < lin->ntiles; ++i) {
		id= lin->tiles[i]->id;
		if (sol->numbers[id] != -1 && sol->tile_done[id] == FALSE) ++num;
	}
	if (num == 0) return FALSE;

	/* check if setting line would handle unhandled tiles */
	for(i=0; i < lin->ntiles; ++i) {
		tile= lin->tiles[i];
		if (sol->numbers[tile->id] == -1 || sol->tile_done[tile->id]) continue;

		/* would line handle tile? */
		if (NUMBER(tile) != sol->tile_count[tile->id].on + 1) return FALSE;
	}

	/* finally check if unhandled tiles are the only ones left */
	for(i=0; i < sol->geo->ntiles; ++i) {
		if (sol->numbers[i] != -1 && sol->tile_done[i] == FALSE)
			++num_unhandled;
	}
	if (num != num_unhandled) return FALSE;

	return TRUE;
}


/*
 * Find two ends of a partial loop. If only separated by a line, cross it
 * Returns length of partial loop
 */
void
solve_bottleneck(struct solution *sol)
{
	int i;

	struct line *end1;
	struct line *end2;
	struct line *next;
	int dir1, dir2;		// direction each end is going
	int stuck;
	int nlines_on=0;		// total number of lines on
	struct geometry *geo=sol->geo;
	int length=0;
	gboolean all_handled=TRUE;

	sol->nchanges= sol->ntile_changes= 0;
	/* check if all numbered tiles have been handled */
	for(i=0; i < geo->ntiles; ++i) {
		if (sol->numbers[i] != -1 && sol->tile_done[i] == FALSE) {
			all_handled= FALSE;
			break;
		}
	}

	/* initialize line mask */
	for(i=0; i < geo->nlines; ++i) {
		if (sol->states[i] == LINE_ON) {
			sol->lin_mask[i]= TRUE;
			++nlines_on;
		} else {
			sol->lin_mask[i]= FALSE;	// mask out not-ON lines
		}
	}

	/* find a line on and follow it */
	for(i=0; i < geo->nlines; ++i) {
		if (sol->lin_mask[i] == FALSE) continue;
		sol->lin_mask[i]= FALSE;
		dir1= DIRECTION_IN;
		dir2= DIRECTION_OUT;
		end1= end2= geo->lines + i;
		stuck= 0;
		length= -1;	// starting line is counted twice
		/* follow along end1 and end2 until they end or they meet */
		while(stuck != 3) {
			/* move end1 */
			if ((stuck & 1) == 0) {
				next= follow_line(sol, end1, &dir1);
				if (next == end2) break;
				++length;
				if (next != NULL) {
					end1= next;
					if (sol->lin_mask[next->id] == FALSE)
						return;
					sol->lin_mask[next->id]= FALSE;
				} else stuck|= 1;
			}
			/* move end2 */
			if ((stuck & 2) == 0) {
				next= follow_line(sol, end2, &dir2);
				if (next == end1) break;
				++length;
				if (next != NULL) {
					end2= next;
					if (sol->lin_mask[next->id] == FALSE)
						return;
					sol->lin_mask[next->id]= FALSE;
				} else stuck|= 2;
			}
		}
		/* while quitted unexpectedly -> we have a closed loop */
		if (stuck != 3) {
			return;
		}

		/* check if ends are within a line away */
		next= find_line_connecting_lines(end1, dir1, end2, dir2);
		if (next != NULL && STATE(next) != LINE_CROSSED) {
			/* we have just one big open loop */
			if (length == nlines_on) {
				/* avoid creating artificial solutions */
				if (all_handled) return;
				/* assume a situation where the only un-handled tile(s)
				   would be handled by setting this line. In this situation,
				   we can't cross out this line. */
				if (bottleneck_is_final_line(sol, next)) return;
			}
			solve_set_line_cross(sol, next);

			//return length;
			return;
		}
	}
}


/*
 * Check state of solution to see if a valid solution has been found
 */
static gboolean
solve_check_solution(struct solution *sol)
{
	int i;
	struct geometry *geo=sol->geo;
	struct line *lin;
	struct line *start=NULL;
	int direction=DIRECTION_IN;
	int nlines_total=0;
	int nlines_loop=0;

	/* check that all numbered tiles are happy */
	for(i=0; i < geo->ntiles; ++i) {
		if (sol->numbers[i] != -1 && sol->tile_done[i] == FALSE)
			return FALSE;
	}

	/* count how many lines are ON, keep the first one found */
	for(i=0; i < geo->nlines; ++i) {
		if (sol->states[i] == LINE_ON) {
			if (nlines_total == 0)
				start= geo->lines + i;
			++nlines_total;
		}
	}
	if (nlines_total == 0) return FALSE;

	/* follow loop starting at 'start' */
	lin= start;
	while(lin != NULL) {
		++nlines_loop;
		lin= follow_line(sol, lin, &direction);
		if (lin == start)	// closed the loop
			break;
	}
	if (lin == NULL) return FALSE;

	/* check number of lines total is == to lines in loop */
	if (nlines_loop != nlines_total) return FALSE;

	return TRUE;
}


/*
 * Calculate difficulty of game from level_count
 */
static void
calculate_difficulty(struct solution *sol)
{
	int i;
	double max_diff[SOLVE_NUM_LEVELS]={1.0, 1.0, 3.0, 4.0, 5.0, 7.0, 8.0, 9.0, 10.0};
	double weights[SOLVE_NUM_LEVELS]={0.5, 0.75, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0};
	double score=0;
	int top_level=0;
	double step;

	/* find top level used and calculate levels' score*/
	for(i=0; i < SOLVE_NUM_LEVELS; ++i) {
		if (sol->level_count[i] > 0)
			top_level= i;
		score+= weights[i] * sol->level_count[i];
		printf("(%1d) %3d (@ %4.2lf) -> %4.2lf\n", i, sol->level_count[i], weights[i],
		       weights[i] * sol->level_count[i]);
	}

	printf("total: 0.5 * %4.2lf/%3d ", score, sol->level_count[0]);
	if (sol->level_count[0] > 0) {
		score= score/sol->level_count[0];
	}
	if (score > 2.0) score= 2.0;
	score= score/2.0;
	printf("= %4.2lf\n", score);

	if (top_level == 0) {
		/* consider unlikely case of top_level == 0 */
		sol->difficulty= score * max_diff[0];
	} else {
		step= max_diff[top_level] - max_diff[top_level - 1];
		sol->difficulty= max_diff[top_level - 1] + score * step;
	}
}


/*
 * Main solution loop
 * max_iter: maximum number of iterations (<=0 -> no limit)
 * max_level: maximum solving level to attempt (-1=try all)
 * level_count: array to count level scores (if NULL don't count)
 */
void
solution_loop(struct solution *sol, int max_iter, int max_level)
{
	int iter=0;
	int level=0;

	if (max_level < 0) max_level= SOLVE_MAX_LEVEL;
	while(level <= max_level) {
		/* */
		if (level == 0) {
			solve_cross_lines(sol);
			solve_trivial_vertex(sol);
		} else if (level == 1) {
			solve_trivial_tiles(sol);
		} else if (level == 2) {
			solve_bottleneck(sol);
		} else if (level == 3) {
			solve_corner(sol);
		} else if (level == 4) {
			solve_maxnumber_incoming_line(sol);
			if (sol->nchanges == 0) solve_maxnumber_exit_line(sol);
		} else if (level == 5) {
			solve_tiles_net_1(sol);
		} else if (level == 6) {
			solve_try_combinations(sol, 0);
		} else if (level == 7) {
			solve_try_combinations(sol, 1);
		} else if (level == 8) {
			solve_try_combinations(sol, 2);
		}

		/* if nothing found go to next level */
		if (sol->nchanges == 0) {
			++level;
		} else {
			if (sol->level_count != NULL) {
				++sol->level_count[level];
				//printf("level %d: %d\n", level, count);
			}
			sol->last_level= level;
			level= 0;
			++iter;
		}

		/* reached maximum number of iterations -> stop */
		if (max_iter > 0 && iter >= max_iter)
			break;
	}
}


/*
 * Solve game
 */
struct solution*
solve_game(struct geometry *geo, struct game *game, double *final_score)
{
	struct solution *sol;

	/* init solution structure */
	sol= solve_create_solution_data(geo, game);

	/* These two tests only run once at the very start */
	solve_zero_tiles(sol);
	printf("zero: changes %d\n", sol->nchanges);
	solve_maxnumber_tiles(sol);
	printf("maxnumber: changes %d\n", sol->nchanges);

	/* run solution loop with no limits */
	solution_loop(sol, -1, -1);

	/* print level counts */
	int i;
	for(i=0; i < SOLVE_NUM_LEVELS; ++i)
		printf("-Level %1d: %3d\n", i, sol->level_count[i]);

	calculate_difficulty(sol);

	/* check if we have a valid solution */
	if (solve_check_solution(sol)) {
		sol->solved= TRUE;
		printf("Solution good! (%lf)\n", sol->difficulty);
	} else {
		sol->solved= FALSE;
		sol->difficulty+= 10;
		printf("Solution BAD! (%lf)\n", sol->difficulty);
	}

	return sol;
}


void
test_solve_game(struct geometry *geo, struct game *game)
{
	int i;
	struct solution *sol;
	double score;

	//game->numbers[0]= 3;
	//game->numbers[35]= 3;
	sol= solve_game(geo, game, &score);

	for(i=0; i < geo->nlines; ++i)
		game->states[i]= sol->states[i];

	solve_free_solution_data(sol);
}


/*
 * Trace through solution
 */
void
test_solve_game_trace(struct geometry *geo, struct game *game)
{
	int i;
	static struct solution *sol;
	static gboolean first=TRUE;

	static int level=-2;

	if (first) {
		/* init solution structure */
		sol= solve_create_solution_data(geo, game);
		first= FALSE;
	}

	if (level == -2)  {
		solve_zero_tiles(sol);
		++level;
	} else if (level == -1) {
		solve_maxnumber_tiles(sol);
		++level;
	} else {
		/* run solution loop for 1 iteration */
		solution_loop(sol, 1, -1);
	}

	calculate_difficulty(sol);

	/* print current level counts */
	for(i=0; i < SOLVE_NUM_LEVELS; ++i) {
		printf("Level %1d: %d\n", i, sol->level_count[i]);
	}
	printf("-----> %5.2lf <------\n", sol->difficulty);



	/* record solution */
	for(i=0; i < geo->nlines; ++i)
		game->states[i]= sol->states[i];

	if (solve_check_solution(sol)) {
		printf("**Solution found! (%lf)\n", sol->difficulty);
	}

	//solve_free_solution_data(sol);
}


/*
 * Solve game from given solution struct
 */
void
solve_game_solution(struct solution *sol, int max_level)
{
	/* These two tests only run once at the very start */
	solve_zero_tiles(sol);
	solve_maxnumber_tiles(sol);

	/* run solution loop with no limits */
	solution_loop(sol, -1, max_level);

	calculate_difficulty(sol);

	/* check if we have a valid solution */
	if (solve_check_solution(sol)) {
		sol->solved= TRUE;
		printf("Solution good! (%lf)\n", sol->difficulty);
	} else {
		sol->solved= FALSE;
		printf("Solution BAD! (%lf)\n", sol->difficulty);
	}
}
