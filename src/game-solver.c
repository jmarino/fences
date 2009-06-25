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
 * Go through all squares and cross sides of squares with a 0
 * Update sol->nchanges to number of lines crossed out
 */
void
solve_zero_squares(struct solution *sol)
{
	int i, j;
	struct square *sq;
	struct geometry *geo=sol->geo;

	sol->nchanges= sol->nsq_changes= 0;
	for(i=0; i < geo->nsquares; ++i) {
		/* only care about unhandled 0 squares */
		if (sol->numbers[i] != 0 || sol->sq_handled[i]) continue;
		/* cross sides of square */
		sol->sq_handled[i]= TRUE;	// mark square as handled
		sol->sq_changes[sol->nsq_changes]= i;
		++sol->nsq_changes;
		sq= geo->squares + i;
		for(j=0; j < sq->nsides; ++j)
			if (STATE(sq->sides[j]) == LINE_OFF)
				CROSS_LINE(sq->sides[j]);
	}
}


/*
 * Iterate over all unhandled squares to find:
 *  - Numbered squares with enough crossed sides that a solution is trivial.
 *  - Any square with all lines either ON or CROSSED -> mark it handled.
 */
void
solve_trivial_squares(struct solution *sol)
{
	int i, j;
	int lines_on;
	int lines_crossed;
	struct square *sq;
	struct geometry *geo=sol->geo;

	sol->nchanges= sol->nsq_changes= 0;
	for(i=0; i < geo->nsquares; ++i) {
		/* only unhandled squares */
		if (sol->sq_handled[i])
			continue;

		/* count lines ON and CROSSED around square */
		sq= geo->squares + i;
		lines_on= 0;
		lines_crossed= 0;
		for(j=0; j < sq->nsides; ++j) {
			if (STATE(sq->sides[j]) == LINE_CROSSED)
				++lines_crossed;
			else if (STATE(sq->sides[j]) == LINE_ON)
				++lines_on;
		}
		/* square has all lines either ON or CROSSED -> handled */
		if (lines_on + lines_crossed == sq->nsides) {
			sol->sq_handled[i]= TRUE;
			continue;
		}
		/* enough lines crossed? -> set ON the OFF ones */
		if ( sq->nsides - lines_crossed == NUMBER(sq) ) {
			sol->sq_handled[i]= TRUE;
			sol->sq_changes[sol->nsq_changes]= i;
			++sol->nsq_changes;
			for(j=0; j < sq->nsides; ++j) {
				if (STATE(sq->sides[j]) == LINE_OFF)
					SET_LINE(sq->sides[j]);
			}
		}
		/* only allow one trivial square to be set at a time */
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
	int lines_on;
	int lines_off;
	int pos=0;
	struct vertex *vertex;
	struct geometry *geo=sol->geo;

	sol->nchanges= sol->nsq_changes= 0;
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
		/* only allow one trivial vertex to be set at a time */
		if (sol->nchanges > 0) break;
		++vertex;
	}
}


/*
 * Find squares with a number == nsides - 1
 * Check around all its vertices to see if it neighbors another square
 * with number == nsides
 * If another found: determine if they're side by side or diagonally and set
 * lines accordingly
 */
void
solve_maxnumber_squares(struct solution *sol)
{
	int i, j, k, k2;
	struct square *sq, *sq2=NULL;
	int pos1, pos2;
	struct vertex *vertex;
	struct line *lin;
	struct geometry *geo=sol->geo;
	int cache;

	sol->nchanges= sol->nsq_changes= 0;
	/* iterate over all squares */
	for(i=0; i < geo->nsquares; ++i) {
		sq= geo->squares + i;
		/* ignore handled squares or with number != sides - 1 */
		if (sol->sq_handled[i] || sol->numbers[i] != sq->nsides - 1)
			continue;

		/* inspect vertices of square */
		for(j=0; j < sq->nvertex; ++j) {
			vertex= sq->vertex[j];
			cache= sol->nchanges;
			/* vertex is in a corner -> enable lines touching it */
			if (vertex->nlines == 2) {
				if (STATE(vertex->lines[0]) != LINE_ON)
					SET_LINE(vertex->lines[0]);
				if (STATE(vertex->lines[1]) != LINE_ON)
					SET_LINE(vertex->lines[1]);
				/* any changes? -> record square */
				if (sol->nchanges > cache) {
					sol->sq_changes[sol->nsq_changes]= i;
					++sol->nsq_changes;
				}
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
				/* cross any lines from vertex that are not part of sq or sq2 */
				for(k=0; k < vertex->nlines; ++k) {
					if (line_touches_square(vertex->lines[k], sq) ||
					    line_touches_square(vertex->lines[k], sq2))
						continue;
					if (STATE(vertex->lines[k]) != LINE_CROSSED)
						CROSS_LINE(vertex->lines[k]);

				}
				/* lines changed -> record square */
				if (sol->nchanges > cache) {
					sol->sq_changes[sol->nsq_changes]= i;
					++sol->nsq_changes;
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
				/* lines changed -> record square */
				if (sol->nchanges > cache) {
					sol->sq_changes[sol->nsq_changes]= i;
					++sol->nsq_changes;
				}
			}
		}
	}
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
void
solve_maxnumber_incoming_line(struct solution *sol)
{
	int i, j, k;
	struct square *sq;
	struct vertex *vertex;
	int nlines_on;
	struct line *lin=NULL;
	struct geometry *geo=sol->geo;
	int cache;

	sol->nchanges= sol->nsq_changes= 0;
	/* iterate over all squares */
	for(i=0; i < geo->nsquares; ++i) {
		sq= geo->squares + i;
		/* ignore squares without number or number < nsides -1 */
		if (sol->numbers[i] != sq->nsides - 1 || sol->sq_handled[i])
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

			cache= sol->nchanges;
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
			/* lines changed? record square */
			if (sol->nchanges > cache) {
				sol->sq_changes[sol->nsq_changes]= i;
				++sol->nsq_changes;
			}
			break;
		}
	}
}


/*
 * A vertex of a square with number == nsides - 1
 * One vertex of square doesn't have any lines ON and all other lines of
 * square are ON.
 * Vertex has only one possible output away from square.
 *  - Turn exit line ON
 */
void
solve_maxnumber_exit_line(struct solution *sol)
{
	int i, j;
	int pos=0;
	int pos2=0;
	struct square *sq;
	struct vertex *vertex;
	int nlines_off;
	struct geometry *geo=sol->geo;

	sol->nchanges= sol->nsq_changes= 0;
	/* iterate over all squares */
	for(i=0; i < geo->nsquares; ++i) {
		sq= geo->squares + i;
		/* ignore handled squares, without number or number < nsides -1 */
		if (sol->numbers[i] != sq->nsides - 1 || sol->sq_handled[i])
			continue;

		/* count lines OFF on square */
		nlines_off= 0;
		for(j=0; j < sq->nsides; ++j) {
			if (STATE(sq->sides[j]) == LINE_OFF) {
				++nlines_off;
				pos2= pos;	// keep track of 2nd to last OFF line
				pos= j;		// hold pos of last OFF line
			}
		}
		/* must have 2 lines OFF */
		if (nlines_off != 2) continue;

		/* find shared vertex between pos and pos2 lines */
		if (sq->sides[pos]->ends[0] == sq->sides[pos2]->ends[0] ||
			sq->sides[pos]->ends[0] == sq->sides[pos2]->ends[1]) {
			vertex= sq->sides[pos]->ends[0];
		} else if (sq->sides[pos]->ends[1] == sq->sides[pos2]->ends[0] ||
			sq->sides[pos]->ends[1] == sq->sides[pos2]->ends[1]) {
			vertex= sq->sides[pos]->ends[1];
		} else {
			continue;	// no shared vertex, not a candidate
		}

		/* count lines OFF going out from vertex and not part of square */
		nlines_off= 0;
		for(j=0; j < vertex->nlines; ++j) {
			/* if any line is ON, stop right here */
			if (STATE(vertex->lines[j]) == LINE_ON)
				break;
			if (STATE(vertex->lines[j]) == LINE_OFF &&
				!line_touches_square(vertex->lines[j], sq)) {
				++nlines_off;
				pos= j;
			}
		}
		if (j < vertex->nlines || nlines_off != 1) continue;

		/* only one line OFF -> set it ON */
		SET_LINE(vertex->lines[pos]);
		sol->sq_changes[sol->nsq_changes]= i;
		++sol->nsq_changes;
		break;
	}
}


/*
 * Find squares with (number == nsides - 1) || (number == 1)
 * If one corner has no exit:
 *	- (nsides - 1) set both lines
 *	- (1) cross both lines
 */
void
solve_corner(struct solution *sol)
{
	int i, j, k;
	struct square *sq;
	struct vertex *vertex;
	struct geometry *geo=sol->geo;
	int cache;

	sol->nchanges= sol->nsq_changes= 0;
	/* iterate over all squares */
	for(i=0; i < geo->nsquares; ++i) {
		sq= geo->squares + i;
		/* ignore handled squares, unnumbered squares or number != nsides -1 */
		if (sol->sq_handled[i] ||
		    (sol->numbers[i] != sq->nsides - 1 && sol->numbers[i] != 1))
			continue;

		cache= sol->nchanges;
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
		/* any line changes? record square */
		if (sol->nchanges > cache) {
			sol->sq_changes[sol->nsq_changes]= i;
			++sol->nsq_changes;
		}
	}
}


/*
 * Find numbered squares with (number - lines_on) == 1
 * If a vertex has one incoming line and only available lines are part of the
 * square, cross all the rest lines of square
 */
void
solve_squares_net_1(struct solution *sol)
{
	int i, j, k;
	struct square *sq;
	int nlines_on;
	int num_exits;
	struct line *lin=NULL;
	struct vertex *vertex;
	struct geometry *geo=sol->geo;
	int cache;

	sol->nchanges= sol->nsq_changes= 0;
	/* iterate over all squares */
	for(i=0; i < geo->nsquares; ++i) {
		sq= geo->squares + i;
		/* ignore handled squares and unnumbered squares */
		if (sol->sq_handled[i] || sol->numbers[i] == -1)
			continue;

		/* count lines ON around square */
		nlines_on= 0;
		for(j=0; j < sq->nsides; ++j) {
			if (STATE(sq->sides[j]) == LINE_ON)
				++nlines_on;
		}
		if (NUMBER(sq) - nlines_on != 1) continue;

		cache= sol->nchanges;
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
		/* any line changes? record square */
		if (sol->nchanges > cache) {
			sol->sq_changes[sol->nsq_changes]= i;
			++sol->nsq_changes;
		}
	}
}


/*
 * Cross out trivial lines.
 * Find vertices with 2 lines ON, vertex busy -> cross any OFF line left.
 * Find vertices with all but one crossed (0 ON and 1 OFF) -> cross it.
 * Repeat process until no more lines are crossed out.
 * Find squares with enough lines ON, cross out any OFF lines around it.
 * **NOTE: this function should only modify 'sol->states' field, so it can be
 * used with solve strategies that do temporary changes.
 */
void
solve_cross_lines(struct solution *sol)
{
	int i, j;
	int num_on;
	int num_off;
	int pos=0;
	struct vertex *vertex;
	struct square *sq;
	struct geometry *geo=sol->geo;
	int old_count=-1;
	int cache;

	sol->nchanges= sol->nsq_changes= 0;
	/* go through all squares */
	for(i=0; i < geo->nsquares; ++i) {
		/* only unhandled squares */
		if (sol->sq_handled[i]) continue;

		/* count lines ON around square */
		sq= geo->squares + i;
		num_on= 0;
		for(j=0; j < sq->nsides; ++j) {
			if (STATE(sq->sides[j]) == LINE_ON)
				++num_on;
		}
		if (sol->numbers[i] == -1) { // unnumbered squares
			/* square has less than nsides-1 ON, ignore */
			if (num_on != sq->nsides - 1) continue;
		} else {	// numbered squares
			/* square not finished, ignore */
			if (num_on != NUMBER(sq)) continue;
		}
		cache= sol->nchanges;
		/* square is complete, cross any OFF left */
		for(j=0; j < sq->nsides; ++j) {
			if (STATE(sq->sides[j]) == LINE_OFF)
				CROSS_LINE(sq->sides[j]);
		}
		/* any line changes? record square */
		if (sol->nchanges > cache) {
			sol->sq_changes[sol->nsq_changes]= i;
			++sol->nsq_changes;
		}
	}

	while (old_count != sol->nchanges) {
		old_count= sol->nchanges;
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

	sol->nchanges= sol->nsq_changes= 0;
	/* check if all numbered squares have been handled */
	for(i=0; i < geo->nsquares; ++i) {
		if (sol->numbers[i] != -1 && sol->sq_handled[i] == FALSE) {
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
			/* avoid creating artificial solutions */
			if (length == nlines_on && all_handled) {
				return;
			}
			CROSS_LINE(next);

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

	/* check that all numbered squares are happy */
	for(i=0; i < geo->nsquares; ++i) {
		if (sol->numbers[i] != -1 && sol->sq_handled[i] == FALSE)
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
			solve_trivial_squares(sol);
		} else if (level == 2) {
			solve_bottleneck(sol);
		} else if (level == 3) {
			solve_corner(sol);
		} else if (level == 4) {
			solve_maxnumber_incoming_line(sol);
			if (sol->nchanges == 0) solve_maxnumber_exit_line(sol);
		} else if (level == 5) {
			solve_squares_net_1(sol);
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
	solve_zero_squares(sol);
	printf("zero: changes %d\n", sol->nchanges);
	solve_maxnumber_squares(sol);
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
		solve_zero_squares(sol);
		++level;
	} else if (level == -1) {
		solve_maxnumber_squares(sol);
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
	solve_zero_squares(sol);
	solve_maxnumber_squares(sol);

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
