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
 * Numbered square:
 *  - Number of lines on around square <= number
 *  - Must have enough lines OFF to possibly satisy square number
 * Non-numbered square:
 *  - **TODO** number ON must be < number of sides in square
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
	struct square *sq;
	struct vertex *vertex;
	
	/* check all squares */
	sq= sol->geo->squares;
	for(i=0; i < sol->geo->nsquares ; ++i) {
		/* only numbered squares */
		if (sol->numbers[i] == -1) continue;
		/* count lines on and compare with number in square */
		num_on= num_off= 0;
		for(j=0; j < sq[i].nsides; ++j) {
			if (STATE(sq[i].sides[j]) == LINE_ON)
				++num_on;
			else if (STATE(sq[i].sides[j]) == LINE_OFF)
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
	sol->sq_handled= (gboolean *)g_malloc(geo->nsquares*sizeof(gboolean));
	
	for(i=0; i < geo->nlines; ++i)
		sol->states[i]= LINE_OFF;
	for(i=0; i < geo->nsquares; ++i) {
		sol->sq_handled[i]= FALSE;
	}
	
	return sol;
}


/*
 * Free solution structure
 */
void
solve_free_solution_data(struct solution *sol)
{
	g_free(sol->states);
	g_free(sol->lin_mask);
	g_free(sol->sq_handled);
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
	memcpy(dest->sq_handled, src->sq_handled, src->geo->nsquares*sizeof(gboolean));
	memcpy(dest->lin_mask, src->lin_mask, src->geo->nlines*sizeof(int));
}


/*
 * Copy solution
 */
struct solution *
solve_duplicate_solution(struct solution *src)
{
	struct solution *sol;
	int lines_size;
	int squares_size;
	
	g_assert(src != NULL);
	lines_size= src->geo->nlines * sizeof(int);
	squares_size= src->geo->nsquares * sizeof(gboolean);
	sol= (struct solution *)g_malloc(sizeof(struct solution));
	sol->geo= src->geo;
	sol->game= src->game;
	sol->numbers= src->numbers;
	sol->states= (int *)g_malloc(lines_size);
	sol->lin_mask= (int *)g_malloc(lines_size);
	sol->sq_handled= (gboolean *)g_malloc(squares_size);
	
	memcpy(sol->states, src->states, lines_size);
	memcpy(sol->sq_handled, src->sq_handled, squares_size);
	memcpy(sol->lin_mask, src->lin_mask, lines_size);
	
	return sol;
}
