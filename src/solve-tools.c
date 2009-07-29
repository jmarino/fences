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
 * Check game data for inconsistencies:
 * Numbered tile:
 *  - Number of lines on around tile <= number
 *  - Must have enough lines OFF to possibly satisfy tile number
 * Non-numbered tile:
 *  - **TODO** number ON must be < number of sides in tile
 * Vertices:
 *  - Vertex with one ON must have at least one OFF
 *  - Vertex with more than two ON lines
 * Return TRUE: all fine
 * Return FALSE: found problem
 */
gboolean
solve_check_valid_game(struct solution *sol)
{
	int i, j;
	int num_on;
	int num_off;
	struct tile *tile;
	struct vertex *vertex;

	/* check all tiles */
	tile= sol->geo->tiles;
	for(i=0; i < sol->geo->ntiles ; ++i) {
		/* only numbered tiles */
		if (sol->numbers[i] == -1) continue;
		/* count lines on and compare with number in tile */
		num_on= num_off= 0;
		for(j=0; j < tile[i].nsides; ++j) {
			if (STATE(tile[i].sides[j]) == LINE_ON)
				++num_on;
			else if (STATE(tile[i].sides[j]) == LINE_OFF)
				++num_off;
		}
		if (num_on > sol->numbers[i]) return FALSE;
		if (num_on + num_off < sol->numbers[i]) return FALSE;
	}

	/* check all vertices, look for no exit lines & more than 2 lines */
	vertex= sol->geo->vertex;
	for(i=0; i < sol->geo->nvertex; ++i) {
		num_on= num_off= 0;
		for(j=0; j < vertex->nlines; ++j) {
			if (STATE(vertex->lines[j]) == LINE_ON)
				++num_on;
			else if (STATE(vertex->lines[j]) == LINE_OFF)
				++num_off;
		}
		if (num_on == 1 && num_off == 0) return FALSE;
		if (num_on > 2) return FALSE;
		++vertex;
	}

	return TRUE;
}


/*
 * Follow on to next line
 * Return next line and new direction to continue
 * Returns NULL if next line doesn't exist
 */
struct line*
goto_next_line(struct line *lin, int *direction, int which)
{
	struct line *next=NULL;

	if (*direction == DIRECTION_IN) {
		if (which >= lin->nin) return NULL;
		next= lin->in[which];
		/* new direction that continues the flow away from lin */
		if (next->ends[0] == lin->ends[0])
			*direction= DIRECTION_OUT;
		else
			*direction= DIRECTION_IN;
	} else if (*direction == DIRECTION_OUT) {
		if (which >= lin->nout) return NULL;
		next= lin->out[which];
		/* new direction that continues the flow away from lin */
		if (next->ends[0] == lin->ends[1])
			*direction= DIRECTION_OUT;
		else
			*direction= DIRECTION_IN;
	} else
		g_debug("illegal direction: %d", *direction);

	return next;
}


/*
 * Follow ON line direction
 * Return next line and new direction to continue
 * Returns NULL if line stops
 */
struct line*
follow_line(struct solution *sol, struct line *lin, int *direction)
{
	int j;
	struct line *next=NULL;

	if (*direction == DIRECTION_IN) {
		/* find next line on in this direction */
		for(j=0; j < lin->nin; ++j) {
			if (STATE(lin->in[j]) == LINE_ON) {
				next= lin->in[j];
				/* new direction that continues the flow */
				if (next->ends[0] == lin->ends[0])
					*direction= DIRECTION_OUT;
				else
					*direction= DIRECTION_IN;
				break;
			}
		}
	} else if (*direction == DIRECTION_OUT) {
		/* find next line on in this direction */
		for(j=0; j < lin->nout; ++j) {
			if (STATE(lin->out[j]) == LINE_ON) {
				next= lin->out[j];
				/* new direction that continues the flow */
				if (next->ends[0] == lin->ends[1])
					*direction= DIRECTION_OUT;
				else
					*direction= DIRECTION_IN;
				break;
			}
		}
	} else
		g_debug("illegal direction: %d", *direction);

	return next;
}


/*
 * Return a new solution structure
 */
struct solution*
solve_create_solution_data(struct geometry *geo, struct game *game)
{
	struct solution *sol;
	int i;

	sol= (struct solution *)g_malloc(sizeof(struct solution));
	sol->geo= geo;
	sol->game= game;
	sol->numbers= game->numbers;
	sol->states= (int *)g_malloc(geo->nlines*sizeof(int));
	sol->lin_mask= (int *)g_malloc(geo->nlines*sizeof(int));
	sol->tile_done= (gboolean *)g_malloc(geo->ntiles*sizeof(gboolean));
	sol->vertex_done= (gboolean *)g_malloc(geo->nvertex*sizeof(gboolean));
	sol->nchanges= 0;
	sol->changes= (int *)g_malloc(geo->nlines*sizeof(int));
	/* **TODO** size of changes is overkill (but safe): optimize */
	sol->ntile_changes= 0;
	sol->tile_changes= (int *)g_malloc(geo->ntiles*sizeof(int));
	/* **TODO** size of tile_changes is overkill (but safe): optimize */
	sol->solved= FALSE;
	sol->difficulty= 0.0;
	sol->last_level= -1;

	for(i=0; i < geo->nlines; ++i)
		sol->states[i]= LINE_OFF;
	for(i=0; i < geo->ntiles; ++i) {
		sol->tile_done[i]= FALSE;
	}
	for(i=0; i < geo->nvertex; ++i)	sol->vertex_done[i]= FALSE;
	memset(sol->level_count, 0, SOLVE_NUM_LEVELS * sizeof(int));

	return sol;
}


/*
 * Free solution structure
 */
void
solve_free_solution_data(struct solution *sol)
{
	if (sol == NULL) return;
	g_free(sol->states);
	g_free(sol->lin_mask);
	g_free(sol->tile_done);
	g_free(sol->vertex_done);
	g_free(sol->changes);
	g_free(sol->tile_changes);
	g_free(sol);
}


/*
 * Copy solution
 */
void
solve_copy_solution(struct solution *dest, struct solution *src)
{
	dest->geo= src->geo;
	dest->game= src->game;
	memcpy(dest->states, src->states, src->geo->nlines*sizeof(int));
	dest->numbers= src->numbers;
	memcpy(dest->tile_done, src->tile_done, src->geo->ntiles*sizeof(gboolean));
	memcpy(dest->vertex_done, src->vertex_done, src->geo->nvertex*sizeof(gboolean));
	memcpy(dest->lin_mask, src->lin_mask, src->geo->nlines*sizeof(int));
	dest->nchanges= src->nchanges;
	memcpy(dest->changes, src->changes, src->geo->nlines*sizeof(int));
	dest->ntile_changes= src->ntile_changes;
	memcpy(dest->tile_changes, src->tile_changes, src->geo->ntiles*sizeof(int));
	memcpy(dest->level_count, src->level_count, SOLVE_NUM_LEVELS * sizeof(int));
	dest->solved= src->solved;
	dest->difficulty= src->difficulty;
	dest->last_level= src->last_level;
}


/*
 * Duplicate solution
 */
struct solution *
solve_duplicate_solution(struct solution *src)
{
	struct solution *sol;
	int lines_size;
	int tiles_size;
	int vertex_size;

	g_assert(src != NULL);
	lines_size= src->geo->nlines * sizeof(int);
	tiles_size= src->geo->ntiles * sizeof(gboolean);
	vertex_size= src->geo->nvertex * sizeof(gboolean);
	sol= (struct solution *)g_malloc(sizeof(struct solution));
	sol->geo= src->geo;
	sol->game= src->game;
	sol->numbers= src->numbers;
	sol->states= (int *)g_malloc(lines_size);
	sol->lin_mask= (int *)g_malloc(lines_size);
	sol->tile_done= (gboolean *)g_malloc(tiles_size);
	sol->vertex_done= (gboolean *)g_malloc(vertex_size);
	sol->changes= (int *)g_malloc(lines_size);
	sol->nchanges= src->nchanges;
	sol->tile_changes= (int *)g_malloc(tiles_size);
	sol->ntile_changes= src->ntile_changes;
	sol->solved= src->solved;
	sol->difficulty= src->difficulty;
	sol->last_level= src->last_level;

	memcpy(sol->states, src->states, lines_size);
	memcpy(sol->tile_done, src->tile_done, tiles_size);
	memcpy(sol->vertex_done, src->vertex_done, vertex_size);
	memcpy(sol->lin_mask, src->lin_mask, lines_size);
	memcpy(sol->changes, src->changes, lines_size);
	memcpy(sol->tile_changes, src->tile_changes, tiles_size);
	memcpy(sol->level_count, src->level_count, SOLVE_NUM_LEVELS * sizeof(int));

	return sol;
}


/*
 * Reset solution state
 */
void
solve_reset_solution(struct solution *sol)
{
	memset(sol->states, 0, sol->geo->nlines * sizeof(int));
	memset(sol->lin_mask, 0, sol->geo->nlines * sizeof(int));
	memset(sol->tile_done, 0, sol->geo->ntiles * sizeof(gboolean));
	memset(sol->vertex_done, 0, sol->geo->nvertex * sizeof(gboolean));
	memset(sol->level_count, 0, SOLVE_NUM_LEVELS * sizeof(int));
	sol->nchanges= 0;
	sol->ntile_changes= 0;
	sol->solved= FALSE;
	sol->difficulty= 0.0;
	sol->last_level= -1;
}


/*
 * Set line ON
 */
inline void
solve_set_line_on(struct solution *sol, struct line *lin)
{
	int id=lin->id;

	if (sol->states[id] != LINE_OFF) return;
	sol->states[id]= LINE_ON;
	sol->changes[sol->nchanges]= id;
	++sol->nchanges;
}


/*
 * CROSS line out
 */
inline void
solve_set_line_cross(struct solution *sol, struct line *lin)
{
	int id=lin->id;

	if (sol->states[id] != LINE_OFF) return;
	sol->states[id]= LINE_CROSSED;
	sol->changes[sol->nchanges]= id;
	++sol->nchanges;
}
