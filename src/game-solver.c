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
#include "solve-combinations.h"


/* number of solution levels */
#define MAX_LEVEL	7


/*
 * Description:
 * We're given a game state and a geometry
 * Find solution
 */




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
 * Check if line touches square
 */
static inline gboolean
line_touches_square(struct line *lin, struct square *sq)
{
	if (lin->nsquares == 1) {
		if (lin->sq[0] == sq) return TRUE;
	} else {
		if (lin->sq[0] == sq || 
		    lin->sq[1] == sq) return TRUE;
	}
	return FALSE;
}


/*
 * Is vertex a cornered with respect to one square
 * I.e. the vertex has no exits outside this square
 */
static gboolean
is_vertex_cornered(struct solution *sol, struct square *sq, 
		   struct vertex *vertex)
{
	int i;
	
	/* check lines coming out of vertex that don't belong to square */
	for(i=0; i < vertex->nlines; ++i) {
		if (line_touches_square(vertex->lines[i], sq)) 
			continue;
		/* if line ON or OFF, vertex not cornered */
		if (STATE(vertex->lines[i]) != LINE_CROSSED)
			return FALSE;
	}
	return TRUE;
}


/*
 * Check game data for inconsistencies
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
	
	/* check all vertices, look for no exit lines */
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
		++vertex;
	}
	
	return TRUE;
}


/*
 * Go through all squares and cross sides of squares with a 0
 * returns number of lines crossed out
 */
int
solve_handle_zero_squares(struct solution *sol)
{
	int i, j;
	int count=0;
	struct square *sq;
	struct geometry *geo=sol->geo;
	
	for(i=0; i < geo->nsquares; ++i) {
		if (sol->numbers[i] != 0) continue; // only care about 0 squares
		/* cross sides of square */
		sol->sq_mask[i]= FALSE;	// mark square as handled
		sq= geo->squares + i;
		for(j=0; j < sq->nsides; ++j)
			CROSS_LINE(sq->sides[j]);
	}
	return count;
}


/*
 * Iterate over all squares to find numbered squares with enough crossed
 * sides that their solution is trivial
 */
int
solve_handle_trivial_squares(struct solution *sol)
{
	int i, j;
	int count=0;
	int lines_crossed;
	struct square *sq;
	struct geometry *geo=sol->geo;
	
	for(i=0; i < geo->nsquares; ++i) {
		if (sol->numbers[i] == -1 || sol->sq_mask[i] == FALSE) 
			continue; // only numbered squares
		
		/* count lines ON and CROSSED around square */
		sq= geo->squares + i;
		lines_crossed= 0;
		for(j=0; j < sq->nsides; ++j) {
			if (STATE(sq->sides[j]) == LINE_CROSSED)
				++lines_crossed;
		}
		/* enough lines crossed -> set ON the OFF ones */
		if ( sq->nsides - lines_crossed == NUMBER(sq) ) {
			sol->sq_mask[i]= FALSE;
			for(j=0; j < sq->nsides; ++j) {
				if (STATE(sq->sides[j]) == LINE_OFF)
					SET_LINE(sq->sides[j]);
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
int
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
		}
		++vertex;
	}
	return count;
}


/*
 * Find squares with a number == nsides - 1
 * Check around all its vertices to see if it neighbors another square 
 * with number == nsides
 * If another found: determine if they're side by side or diagonally and set
 * lines accordingly
 */
int
solve_handle_maxnumber_squares(struct solution *sol)
{
	int i, j, k, k2;
	struct square *sq, *sq2=NULL;
	int pos1, pos2;
	struct vertex *vertex;
	struct line *lin;
	struct geometry *geo=sol->geo;
	int count=0;
	
	/* iterate over all squares */
	for(i=0; i < geo->nsquares; ++i) {
		sq= geo->squares + i;
		/* ignore squares without number or number < nsides -1 */
		if (sol->sq_mask[i] == FALSE || sol->numbers[i] != sq->nsides - 1) 
			continue;
		
		/* inspect vertices of square */
		for(j=0; j < sq->nvertex; ++j) {
			vertex= sq->vertex[j];
			/* vertex is in a corner -> enable lines touching it */
			if (vertex->nlines == 2) {
				if (STATE(vertex->lines[0]) != LINE_ON)
					SET_LINE(vertex->lines[0]);
				if (STATE(vertex->lines[1]) != LINE_ON)
					SET_LINE(vertex->lines[1]);
				continue;
			}
			/* at this point vertex touches at least 2 squares (not in a corner) */
			/* find neighbor square with MAX_NUMBER */
			for(k=0; k < vertex->nsquares; ++k) {
				sq2= vertex->sq[k];
				if (sq2 == sq) continue; // ignore current square
				if ( MAX_NUMBER(sq2) )
					break;
			}
			if (k == vertex->nsquares) // no max neighbor 
				continue;
			
			/* find if the two MAX squares share a line */
			lin= find_shared_side(sq, sq2, &pos1, &pos2);
			if (lin != NULL) {	// shared side: side by side squares
				if (STATE(lin) != LINE_ON)
					SET_LINE(lin);
				/* set all lines around squares except the ones touching lin */
				for(k=2; k < sq->nsides - 1; ++k) {
					k2= (pos1 + k) % sq->nsides;
					if (STATE(sq->sides[k2]) != LINE_ON)
						SET_LINE(sq->sides[k2]);
				}
				for(k=2; k < sq2->nsides - 1; ++k) {
					k2= (pos2 + k) % sq2->nsides;
					if (STATE(sq->sides[k2]) != LINE_ON)
						SET_LINE(sq2->sides[k2]); 
				}
			} else {	// no shared side: diagonally opposed squares
				/* set all lines around squares that don't touch vertex */
				for(k=0; k < sq->nsides; ++k) {
					if (sq->sides[k]->ends[0] == vertex ||
					    sq->sides[k]->ends[1] == vertex) continue;
					if (STATE(sq->sides[k]) != LINE_ON)
						SET_LINE(sq->sides[k]);
				}
				for(k=0; k < sq2->nsides; ++k) {
					if (sq2->sides[k]->ends[0] == vertex ||
					    sq2->sides[k]->ends[1] == vertex) continue;
					if (STATE(sq2->sides[k]) != LINE_ON)
						SET_LINE(sq2->sides[k]);
				}
			}
		}
	}

	return count;
}


/*
 * Applies to squares with a number == nsides - 1
 * If a square with number == (nsides - 1) has an incoming line in a vertex,
 * the line must go around the square and exit through a vertex one line away
 * from the current vertex. This translates into:
 *  - set ON all lines around square that don't touch this vertex
 *  - line must go in to square, can't create a corner. Thus, cross any line
 *    going out from vertex that does not go into square
 */
int
solve_handle_maxnumber_incoming_line(struct solution *sol)
{
	int i, j, k;
	struct square *sq;
	struct vertex *vertex;
	int nlines_on;
	struct line *lin=NULL;
	struct geometry *geo=sol->geo;
	int count=0;
	
	/* iterate over all squares */
	for(i=0; i < geo->nsquares; ++i) {
		sq= geo->squares + i;
		/* ignore squares without number or number < nsides -1 */
		if (sol->numbers[i] != sq->nsides - 1 || sol->sq_mask[i] == FALSE) 
			continue;
		
		/* inspect vertices of square */
		for(j=0; j < sq->nvertex; ++j) {
			vertex= sq->vertex[j];
			/* check that vertex has one ON line */
			nlines_on= 0;
			for(k=0; k < vertex->nlines; ++k) {
				if (STATE(vertex->lines[k]) == LINE_ON) {
					lin= vertex->lines[k];
					++nlines_on;
				}
			}
			if (nlines_on != 1) continue;
			/* make sure ON line is not part of square */
			if (line_touches_square(lin, sq)) continue;

			/* cross lines going out from vertex that don't
			 * touch square */
			for(k=0; k < vertex->nlines; ++k) {
				if (STATE(vertex->lines[k]) == LINE_OFF &&
				    !line_touches_square(vertex->lines[k], sq)) {
					CROSS_LINE(vertex->lines[k]);
				}
			}
			
			/* set lines in square not touching vertex ON */
			for(k=0; k < sq->nsides; ++k) {
				if (sq->sides[k]->ends[0] == vertex ||
				    sq->sides[k]->ends[1] == vertex) continue;
				if (STATE(sq->sides[k]) != LINE_ON)
					SET_LINE(sq->sides[k]);
			}
			break;
		}
	}

	return count;
}


/*
 * Find squares with (number == nsides - 1) || (number == 1)
 * If one corner has no exit:
 * 	- (nsides - 1) set both lines
 * 	- (1) cross both lines
 */
int
solve_handle_corner(struct solution *sol)
{
	int i, j, k;
	struct square *sq;
	struct vertex *vertex;
	struct geometry *geo=sol->geo;
	int count=0;
	
	/* iterate over all squares */
	for(i=0; i < geo->nsquares; ++i) {
		sq= geo->squares + i;
		/* ignore squares without number or number < nsides -1 */
		if (sol->sq_mask[i] == FALSE ||
		    (sol->numbers[i] != sq->nsides - 1 && sol->numbers[i] != 1)) 
			continue;
		
		/* inspect vertices of square */
		for(j=0; j < sq->nvertex; ++j) {
			vertex= sq->vertex[j];
			
			/* check vertex is a no-exit corner */
			if (is_vertex_cornered(sol, sq, vertex) == FALSE)
				continue;
			/* ON or CROSS corner lines */
			for(k=0; k < vertex->nlines; ++k) {
				if (line_touches_square(vertex->lines[k], sq) == FALSE) 
					continue;
				if (sol->numbers[i] == 1) {
					if (STATE(vertex->lines[k]) != LINE_CROSSED)
						CROSS_LINE(vertex->lines[k]);
				} else {
					if (STATE(vertex->lines[k]) != LINE_ON)
						SET_LINE(vertex->lines[k]);
				}
			}
		}
	}

	return count;
}


/*
 * Find numbered squares with (number - lines_on) == 1
 * If a vertex has one incoming line and only available lines are part of the
 * square, cross all the rest lines of square
 */
int
solve_handle_squares_net_1(struct solution *sol)
{
	int i, j, k;
	struct square *sq;
	int nlines_on;
	int num_exits;
	struct line *lin=NULL;
	struct vertex *vertex;
	struct geometry *geo=sol->geo;
	int count=0;
	
	/* iterate over all squares */
	for(i=0; i < geo->nsquares; ++i) {
		sq= geo->squares + i;
		/* ignore squares without number or number < nsides -1 */
		if (sol->sq_mask[i] == FALSE) 
			continue;
		
		/* count lines ON around square */
		nlines_on= 0;
		for(j=0; j < sq->nsides; ++j) {
			if (STATE(sq->sides[j]) == LINE_ON) 
				++nlines_on;
		}
		if (NUMBER(sq) - nlines_on != 1) continue;
		
		/* inspect vertices of square */
		for(j=0; j < sq->nvertex; ++j) {
			vertex= sq->vertex[j];
			/* check we have one incoming line ON */
			nlines_on= 0;
			num_exits= 0;
			for(k=0; k < vertex->nlines; ++k) {
				if (STATE(vertex->lines[k]) == LINE_ON) {
					lin= vertex->lines[k];
					++nlines_on;
					continue;
				}
				/* OFF line not on square */
				if (STATE(vertex->lines[k]) == LINE_OFF && 
				    line_touches_square(vertex->lines[k], sq) == FALSE) {
					++num_exits;
				}
			}
			if (nlines_on != 1 || line_touches_square(lin, sq) == TRUE ||
			    num_exits > 0)
				continue;
			
			/* cross all lines away from this vertex */
			for(k=0; k < sq->nsides; ++k) {
				if (sq->sides[k]->ends[0] == vertex || 
				    sq->sides[k]->ends[1] == vertex) continue;
				if (STATE(sq->sides[k]) == LINE_OFF) 
					CROSS_LINE(sq->sides[k]);
			}
		}
	}

	return count;
}


/*
 * Cross out trivial lines.
 * Find vertices with 2 lines ON, vertex busy -> cross any OFF line left.
 * Find vertices with only 0 ON and 1 OFF, no exit -> cross it.
 * Repeat process until no more lines are crossed out.
 * Find squares with enough lines ON, cross out any OFF lines around it.
 * **NOTE: this function should only modify 'sol->states' field, so it can be
 * used with solve strategies that do temporary changes.
 */
int
solve_cross_lines(struct solution *sol)
{
	int i, j;
	int num_on;
	int num_off;
	int pos=0;
	struct vertex *vertex;
	struct square *sq;
	struct geometry *geo=sol->geo;
	int count=0;
	int old_count=-1;
	
	while (old_count != count) {
		old_count= count;
		/* go through all vertices */
		vertex= geo->vertex;
		for(i=0; i < geo->nvertex; ++i) {
			num_on= num_off= 0;
			/* count lines on and off */
			for(j=0; j < vertex->nlines; ++j) {
				if (STATE(vertex->lines[j]) == LINE_ON)
					++num_on;
				else if (STATE(vertex->lines[j]) == LINE_OFF) {
					pos= j;
					++num_off;
				}
			}
			/* check count */
			if (num_on == 2) {	/* vertex busy */
				for(j=0; j < vertex->nlines; ++j) {
					if (STATE(vertex->lines[j]) == LINE_OFF)
						CROSS_LINE(vertex->lines[j]);
				}
			} else if (num_on == 0 && num_off == 1) { /* no exit */
				CROSS_LINE(vertex->lines[pos]);
			}
			++vertex;
		}
	}
	
	/* go through all squares */
	for(i=0; i < geo->nsquares; ++i) {
		/* only numbered unhandled squares */
		if (sol->sq_mask[i] == FALSE) continue;
			
		/* count lines ON around square */
		sq= geo->squares + i;
		num_on= 0;
		for(j=0; j < sq->nsides; ++j) {
			if (STATE(sq->sides[j]) == LINE_ON)
				++num_on;
		}
		if (num_on != NUMBER(sq)) continue; // not ready, continue
		/* square is complete, cross any OFF left */
		for(j=0; j < sq->nsides; ++j) {
			if (STATE(sq->sides[j]) == LINE_OFF)
				CROSS_LINE(sq->sides[j]);
		}
	}
	
	return count;
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
 * Find two ends of a partial loop. If only separated by a line, cross it
 * Returns length of partial loop
 */
int
solve_handle_loop_bottleneck(struct solution *sol)
{
	int i, j;
	int count=0;
	struct line *end1;
	struct line *end2;
	struct line *next;
	struct vertex *vertex;
	int dir1, dir2;		// direction each end is going
	int stuck;
	int lines_left;		// how many ON lines to still left to check
	struct geometry *geo=sol->geo;
	int length=0;
	
	/* initialize line mask */
	lines_left= 0;
	for(i=0; i < geo->nlines; ++i) {
		if (sol->states[i] == LINE_ON) {
			sol->lin_mask[i]= TRUE;
			++lines_left;
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
		length= 0;
		/* follow along end1 and end2 until they end or they meet */
		while(stuck != 3) {
			/* move end1 */
			if ((stuck & 1) == 0) {
				next= follow_line(sol, end1, &dir1);
				if (next == end2) break;
				if (next != NULL) {
					end1= next;
					sol->lin_mask[next->id]= FALSE;
					++length;
				} else stuck|= 1;
			}
			/* move end2 */
			if ((stuck & 2) == 0) {
				next= follow_line(sol, end2, &dir2);
				if (next == end1) break;
				if (next != NULL) {
					end2= next;
					sol->lin_mask[next->id]= FALSE;
					++length;
				} else stuck|= 2;
			}
		}
		/* while quitted unexpectedly -> we have a closed loop */
		if (stuck != 3) {
			return 0;
		}
		
		/* check if ends are within a line away */
		if (dir1 == DIRECTION_IN) vertex= end1->ends[0];
		else vertex= end1->ends[1];
		if (dir2 == DIRECTION_IN) {
			for(j=0; j < end2->nin; ++j) {
				if (end2->in[j]->ends[0] == vertex ||
				    end2->in[j]->ends[1] == vertex) break;
			}
			if (j < end2->nin && STATE(end2->in[j]) != LINE_CROSSED) {
				printf("bottleneck loop found: end1 %d\n", end2->in[j]->id);
				CROSS_LINE(end2->in[j]);
				return length;
			}
		} else {
			for(j=0; j < end2->nout; ++j) {
				if (end2->out[j]->ends[0] == vertex ||
				    end2->out[j]->ends[1] == vertex) break;
			}
			if (j < end2->nout && STATE(end2->out[j]) != LINE_CROSSED) {
				printf("bottleneck loop found: end2 %d\n", end2->out[j]->id);
				CROSS_LINE(end2->out[j]);
				return length;
			}
		}
	}

	return 0;
}


/*
 *
 */
static gboolean
solve_check_solution(struct solution *sol)
{
	int i;
	struct geometry *geo=sol->geo;
	gboolean sol_good=TRUE;
	
	/* check we have one and only one loop
	 * last run inside 'handle_loop_bootleneck' set lin_mask
	 * if the whole loop was followed, all lines will have lin_mask FALSE */
	for(i=0; i < geo->nlines; ++i) {
		if (sol->states[i] == LINE_ON) {
			if (sol->lin_mask[i]) {
				sol_good= FALSE;
				break;
			}
		}
	}
	
	/* check that all squares are happy */
	for(i=0; i < geo->nsquares; ++i) {
		if (sol->sq_mask[i]) {
			sol_good= FALSE;
			break;
		}
	}
	return sol_good;
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
	sol->sq_mask= (gboolean *)g_malloc(geo->nsquares*sizeof(gboolean));
	
	for(i=0; i < geo->nlines; ++i)
		sol->states[i]= LINE_OFF;
	for(i=0; i < geo->nsquares; ++i) {
		if (game->numbers[i] != -1)
			sol->sq_mask[i]= TRUE;
		else
			sol->sq_mask[i]= FALSE;
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
	g_free(sol->sq_mask);
	g_free(sol);
}


/*
 * Calculate difficulty of game from level_count
 */
static double
calculate_difficulty(int *level_count)
{
	int i;
	//double level_range[MAX_LEVEL]={0.25, 0.5, 0.75, 1.25, 1.5, 1.75, 4.0};
	double level_range[MAX_LEVEL]={0.25, 0.25, 1.5, 2.0, 0.5, 2.5, 3.0};
	double score;
	double total_score;
	int total=0;
	
	/* count total */
	for(i=0; i < MAX_LEVEL; ++i)
		total+= level_count[i];
	
	/* calculate difficulty */
	//printf("-------------\n");
	total_score= 0.;
	for(i=0; i < MAX_LEVEL; ++i) {
		if (i == 0 || i == 1) {
			score= level_count[i]/( (double)total/2.0 );
		} else if (i == 2) {
			if (level_count[0] > 0)
				score= level_count[2]/((double)level_count[0]/10.);
			else {
				if (level_count[2] > 0)	score= 1.;
				else score= 0.;
			}
		} else if (i == 6) {
			if (level_count[i - 1] > 0)
				score= level_count[i]/(double)level_count[i - 1]*4;
			else {
				if (level_count[i] > 0)	score= 1.;
				else score= 0.;
			}
		} else {
			if (level_count[i - 1] > 0)
				score= level_count[i]/(double)level_count[i - 1];
			else {
				if (level_count[i] > 0)	score= 1.;
				else score= 0.;
			}
		}
		
		/* cap score contribution from this level */
		if (score > 1.0) 
			score= 1.0;
		
		//printf("..level %d: %lf\n", i, score*level_range[i]);
		total_score+= score * level_range[i];
	}
	return total_score;
}


/*
 * Solve game
 */
struct solution*
solve_game(struct geometry *geo, struct game *game, double *final_score)
{
	struct solution *sol;
	int count;
	int level=-1;
	int level_count[MAX_LEVEL]={0, 0, 0, 0, 0, 0, 0};
	int last_level= -1;
	
	/* init solution structure */
	sol= solve_create_solution_data(geo, game);
	
	/* These two tests only run once at the very start */
	count= solve_handle_zero_squares(sol);
	printf("zero: count %d\n", count);
	
	count= solve_handle_maxnumber_squares(sol);
	printf("maxnumber: count %d\n", count);

	while(level < MAX_LEVEL) {
		/* */
		if (level == -1) {
			(void)solve_cross_lines(sol);
			count= 0;
		} else if (level == 0) {
			count= solve_handle_trivial_vertex(sol);
		} else if (level == 1) {
			count= solve_handle_trivial_squares(sol);
		} else if (level == 2) {
			count= solve_handle_corner(sol);
		} else if (level == 3) {
			count= solve_handle_maxnumber_incoming_line(sol);
		} else if (level == 4) {
			count= solve_handle_loop_bottleneck(sol) != 0;
		} else if (level == 5) {
			count= solve_handle_squares_net_1(sol);
		} else if (level == 6) {
			count= solve_try_combinations(sol);
		}
		
		printf("level %d: count %d\n", level, count);
				
		if (count == 0) {
			++level;
		} else {
			/* ignore two bottlenecks in a row */
			if (level == 4 && last_level == 4) 
				count= 0;
			
			g_assert(level >= 0);
			level_count[level]+= count;
			last_level= level;
			level= -1;
		}
	}
	
	int i;
	printf("-------------\n");
	for(i=0; i < MAX_LEVEL; ++i) {
		printf("level %d: %d\n", i, level_count[i]);
	}
	printf("-------------\n");
	
	*final_score= calculate_difficulty(level_count);
	
	/* check if we have a valid solution */
	if (solve_check_solution(sol)) {
		printf("Solution good! (%lf)\n", *final_score);
	} else {
		*final_score+= 10;
		printf("Solution BAD! (%lf)\n", *final_score);
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
	double final_score;
	
	int count=0;
	static int level=-3;
	static int level_count[MAX_LEVEL]={0, 0, 0, 0, 0, 0, 0};
	static int last_level= -1;

	if (first) {
		/* init solution structure */
		sol= solve_create_solution_data(geo, game);
		first= FALSE;
	}	

	while(level < MAX_LEVEL) {
		if (level == -3) {
			count= solve_handle_zero_squares(sol);
			count= 0;
		} else if (level == -2) {
			count= solve_handle_maxnumber_squares(sol);
			count= 0;
		} else if (level == -1) {
			/* cross all possible lines */
			(void)solve_cross_lines(sol);
			count= 0;
		} else if (level == 0) {
			count= solve_handle_trivial_vertex(sol);
		} else if (level == 1) {
			count= solve_handle_trivial_squares(sol);
		} else if (level == 2) {
			count= solve_handle_corner(sol);
		} else if (level == 3) {
			count= solve_handle_maxnumber_incoming_line(sol);
		} else if (level == 4) {
			count= solve_handle_loop_bottleneck(sol) != 0;
		} else if (level == 5) {
			count= solve_handle_squares_net_1(sol);
		} else if (level == 6) {
			count= solve_try_combinations(sol);
		}
		
		printf("level %d: %d\t", level, count);
		
		if (count == 0) {
			++level;
		} else {
			g_assert(level >= 0);
			/* ignore two bottlenecks in a row */
			if (level == 4 && last_level == 4) 
				count= 0;
			
			level_count[level]+= count;
			last_level= level;
			level= -1;
		}
		break;
	}
	
	final_score= calculate_difficulty(level_count);
	
	printf("["); 
	for(i=0; i < MAX_LEVEL; ++i) {
		printf("(%d):%d", i, level_count[i]);
		if (i < MAX_LEVEL - 1) printf("\t");
	}
	printf("] --> %5.2lf\n", final_score);

	

	/* record solution */
	for(i=0; i < geo->nlines; ++i)
		game->states[i]= sol->states[i];

	if (solve_check_solution(sol)) {
		printf("**Solution found! (%lf)\n", final_score);
		printf("-------------\n");
		for(i=0; i < MAX_LEVEL; ++i)
			printf("level %d: %d\n", i, level_count[i]);
		printf("-------------\n");
	}
	
	//solve_free_solution_data(sol);
}
