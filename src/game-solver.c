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

/*
 * Description:
 * We're given a game state and a geometry
 * Find solution
 */


/*
 * Convenience macros (they assume that 'struct solution *sol' is defined)
 */
#define SET_LINE(lin)	sol->states[(lin)->id]= LINE_ON
#define UNSET_LINE(lin)	sol->states[(lin)->id]= LINE_OFF
#define CROSS_LINE(lin)	sol->states[(lin)->id]= LINE_CROSSED
#define STATE(lin)	sol->states[(lin)->id]
#define NUMBER(sq)	sol->numbers[(sq)->id]
#define MAX_NUMBER(sq)	sol->numbers[(sq)->id] == ((sq)->nsides - 1)


#define DIRECTION_IN	0
#define DIRECTION_OUT	1


struct solution {
	struct geometry *geo;	// geometry of game
	struct game *game;	// game state (mostly useful for square numbers)
	int *states;		// line states (so we don't touch game->states)
	int *numbers;		// points to game->numbers
	gboolean *sq_mask;
	int *lin_mask;
};

struct step {
	int dir;
	int taken;
};



/*
 * Find line that touches both given squares
 * NULL: if squares don't share any line
 */
static struct line*
find_shared_side(struct square *sq1, struct square *sq2, int *i, int *j)
{
	for(*i=0; *i < sq1->nsides; ++(*i)) {
		for(*j=0; *j < sq1->nsides; ++(*j)) {
			if (sq1->sides[*i] == sq2->sides[*j]) {
				return sq1->sides[*i];
			}
		}
	}
	*i= *j= 0;
	return NULL;
}


/*
 * Go through all squares and cross sides of squares with a 0
 * returns number of lines crossed out
 */
static int
solve_handle_zero_squares(struct solution *sol)
{
	int i, j;
	int count=0;
	struct geometry *geo=sol->geo;
	
	for(i=0; i < geo->nsquares; ++i) {
		if (sol->numbers[i] == 0) {
			sol->sq_mask[i]= FALSE;
			for(j=0; j < geo->squares[i].nsides; ++j) {
				sol->states[ geo->squares[i].sides[j]->id ]= 
					LINE_CROSSED;
				++count;
			}
		}
	}
	return count;
}


/*
 * Iterate over all squares to find numbered squares that have enough crossed
 * sides that can be solved
 */
static int
solve_handle_trivial_squares(struct solution *sol)
{
	int i, j;
	int count=0;
	int lines_on;
	int lines_crossed;
	struct square *sq;
	struct geometry *geo=sol->geo;
	
	for(i=0; i < geo->nsquares; ++i) {
		if (sol->numbers[i] == -1) continue; // only numbered squares
		
		sq= geo->squares + i;
		lines_on= lines_crossed= 0;
		for(j=0; j < sq->nsides; ++j) {
			if (STATE(sq->sides[j]) == LINE_ON)
				++lines_on;
			if (STATE(sq->sides[j]) == LINE_CROSSED)
				++lines_crossed;
		}
		/* if lines ON == square number cross the rest */
		if ( lines_on == NUMBER(sq)) {
			for(j=0; j < sq->nsides; ++j) {
				if (STATE(sq->sides[j]) == LINE_OFF)
					CROSS_LINE(sq->sides[j]);
			}
		} else {
			/* enough lines crossed -> set any that's OFF */
			if ( sq->nsides - lines_crossed == NUMBER(sq) ) {
				for(j=0; j < sq->nsides; ++j) {
					if (STATE(sq->sides[j]) == LINE_OFF)
						SET_LINE(sq->sides[j]);
				}
			}
		}
		
	}
	return count;
}


/*
 * Iterate over all vertex to find vertices with one line ON and only one other
 * possible line available, i.e., (nlines - 2) CROSSED and 1 OFF
 * Then set remaining line
 */
static int
solve_handle_trivial_vertex(struct solution *sol)
{
	int i, j;
	int count=0;
	int lines_on;
	int lines_off;
	int pos=0;
	struct vertex *vertex;
	struct geometry *geo=sol->geo;
	
	vertex= geo->vertex;
	for(i=0; i < geo->nvertex; ++i) {
		lines_on= lines_off= 0;
		/* count lines on and off */
		for(j=0; j < vertex->nlines; ++j) {
			if (STATE(vertex->lines[j]) == LINE_ON)
				++lines_on;
			else if (STATE(vertex->lines[j]) == LINE_OFF) {
				pos= j;
				++lines_off;
			}
		}
		if (lines_on == 1 && lines_off == 1) {
			/* vertex with one incoming and only one exit */
			SET_LINE(vertex->lines[pos]);
		} else if (lines_on == 0  && lines_off == 1) {
			/* vertex with all but one crossed out */
			CROSS_LINE(vertex->lines[pos]);
		}
		++vertex;
	}
	return count;
}


/*
 * Find squares with a number == nsides
 * Check around all its vertices to see if it neighbors another square 
 * with number == nsides
 * If another found: determine if they're side by side or diagonally and set
 * lines accordingly
 */
static int
solve_handle_maxnumber_squares(struct solution *sol)
{
	int i, j, k;
	struct square *sq, *sq2=NULL;
	int pos1, pos2;
	struct vertex *vertex;
	struct line *lin;
	struct geometry *geo=sol->geo;
	
	/* iterate over all squares */
	for(i=0; i < geo->nsquares; ++i) {
		sq= geo->squares + i;
		/* ignore squares without number or number < nsides -1 */
		if (sol->numbers[i] != sq->nsides - 1) continue;
		
		/* inspect vertices of square */
		for(j=0; j < sq->nvertex; ++j) {
			vertex= sq->vertex[j];
			/* vertex is in a corner -> enable lines touching it */
			if (vertex->nlines == 2) {
				SET_LINE(vertex->lines[0]);
				SET_LINE(vertex->lines[1]);
				continue;
			}
			/* at this point vertex touches at least 2 squares (not in a corner) */
			for(k=0; k < vertex->nsquares; ++k) {
				sq2= vertex->sq[k];
				if (sq2 == sq) continue; // ignore current square
				if ( MAX_NUMBER(sq2) )
					break;
			}
			if (k == vertex->nsquares) // no max neighbor 
				continue;
			
			/* find if the two squares share a line */
			lin= find_shared_side(sq, sq2, &pos1, &pos2);
			if (lin != NULL) {	// side by side squares
				SET_LINE(lin);
				/* set all lines around squares except the ones touching lin */
				for(k=2; k < sq->nsides - 1; ++k)
					SET_LINE(sq->sides[ (pos1 + k) % sq->nsides ]);
				for(k=2; k < sq2->nsides - 1; ++k)
					SET_LINE(sq2->sides[ (pos2 + k) % sq2->nsides ]); 
			} else {		// diagonally opposed squares
				/* set all lines around squares that don't touch vertex */
				for(k=0; k < sq->nsides; ++k) {
					if (sq->sides[k]->ends[0] == vertex ||
					    sq->sides[k]->ends[1] == vertex) continue;
					SET_LINE(sq->sides[k]);
				}
				for(k=0; k < sq2->nsides; ++k) {
					if (sq2->sides[k]->ends[0] == vertex ||
					    sq2->sides[k]->ends[1] == vertex) continue;
					SET_LINE(sq2->sides[k]);
				}
			}
		}
		
		 
	}

	return 0;
}


/*
 * Finds vertices with 2 ON lines and crosses out the rest
 */
static int
solve_handle_busy_vertex(struct solution *sol)
{
	int i, j;
	int num_on;
	struct geometry *geo=sol->geo;
	
	
	for(i=0; i < geo->nvertex; ++i) {
		num_on= 0;
		for(j=0; j < geo->vertex[i].nlines; ++j) {
			if (STATE(geo->vertex[i].lines[j]) == LINE_ON)
			    ++num_on;
		}
		if (num_on < 2) continue;
		if (num_on > 2) 	/* sanity check */
			g_debug("vertex %d has %d (>2) lines ON", i, num_on);
		for(j=0; j < geo->vertex[i].nlines; ++j) {
			if (STATE(geo->vertex[i].lines[j]) == LINE_OFF)
				CROSS_LINE(geo->vertex[i].lines[j]);
		}
	}

	return 0;
}


/*
 * Free solution structure
 */
static void
solve_free_solution(struct solution *sol)
{
	g_free(sol->states);
	g_free(sol->lin_mask);
	g_free(sol->sq_mask);
	g_free(sol);
}


/*
 * Solve game
 */
struct solution*
solve_game(struct geometry *geo, struct game *game)
{
	struct solution *sol;
	int i;
	
	/* init solution structure */
	sol= (struct solution *)g_malloc(sizeof(struct solution));
	sol->geo= geo;
	sol->game= game;
	sol->numbers= game->numbers;
	sol->states= (int *)g_malloc(geo->nlines*sizeof(int));
	sol->lin_mask= (int *)g_malloc(geo->nlines*sizeof(int));
	sol->sq_mask= (gboolean *)g_malloc(geo->nsquares*sizeof(gboolean));
	
	for(i=0; i < geo->nlines; ++i)
		sol->states[i]= LINE_OFF;
	for(i=0; i < geo->nsquares; ++i)
		sol->sq_mask[i]= TRUE;
	
	/* These two tests only run once at the very start */
	solve_handle_zero_squares(sol);
	solve_handle_maxnumber_squares(sol);
	
	for(i=0; i < 10; ++i) {
		solve_handle_busy_vertex(sol);
		solve_handle_trivial_squares(sol);
		solve_handle_trivial_vertex(sol);
	}
	
	return sol;
}


void 
test_solve_game(struct geometry *geo, struct game *game)
{
	int *squares;
	int i;
	struct solution *sol;
	
	//game->numbers[0]= 3;
	//game->numbers[35]= 3;
	sol= solve_game(geo, game);
	
	for(i=0; i < geo->nlines; ++i)
		game->states[i]= sol->states[i];
	
	solve_free_solution(sol);
}
