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
#include "brute-force.h"




/*
 * Allocate stack to record steps taken
 */
static struct stack*
brute_create_step_stack(struct solution *sol, int size)
{
	struct stack *stack;
	int i;

	stack= (struct stack*)g_malloc(sizeof(struct stack));
	stack->step= (struct step*)g_malloc(size * sizeof(struct step));
	stack->ini_states= (int*)g_malloc(sol->geo->nlines * sizeof(int));
	stack->pos= 0;
	stack->size= size;

	/* record initial states in stack */
	for(i=0; i < sol->geo->nlines; ++i) {
		stack->ini_states[i]= sol->states[i];
	}

	return stack;
}


/*
 * Free step stack
 */
static void
brute_free_step_stack(struct stack *stack)
{
	g_free(stack->step);
	g_free(stack->ini_states);
	g_free(stack);
}


/*
 * Check we have a single loop
 * This assumes that the game is in a valid state, i.e.,
 * that solve_check_valid_game has been called previously
 * Return TRUE: single loop found
 * Return FALSE: problem (no loop or isolated loop)
 */
static gboolean
check_single_loop(struct solution *sol, struct line *start)
{
	int i, j;
	int nlines_on=0;
	int direction=DIRECTION_IN;	// OUT would do too, supposed to be a loop
	struct line *lin;
	struct square *sq;
	struct geometry *geo=sol->geo;

	/* check that all numbered squares are satisfied */
	sq= geo->squares;
	for(i=0; i < geo->nsquares ; ++i) {
		/* only numbered squares */
		if (sol->numbers[i] == -1) continue;
		/* count lines ON and compare with number in square */
		nlines_on= 0;
		for(j=0; j < sq[i].nsides; ++j) {
			if (STATE(sq[i].sides[j]) == LINE_ON)
				++nlines_on;
		}
		if (nlines_on != sol->numbers[i]) {
			printf("square unhappy (%d): %d != %d\n", i, nlines_on, sol->numbers[i]);
			return FALSE;
		}
	}

	/* count total lines ON */
	nlines_on= 0;
	for(i=0; i < geo->nlines; ++i) {
		if (sol->states[i] == LINE_ON)
			++nlines_on;
	}

	/* go around loop until we find 'start' or an end (open loop) */
	lin= start;
	while (lin != NULL) {
		lin= follow_line(sol, lin, &direction);
		--nlines_on;
		if (lin == start) // closed loop
			break;
	}
	if (lin == start && nlines_on == 0)
		return TRUE;

	return FALSE;
}


/*
 * Backtrack to previous step
 */
static inline void
backtrack_step(struct solution *sol, struct stack *stack)
{
	/* restore old state */
	sol->states[stack->step[stack->pos].id]= LINE_OFF;

	/* go to previous step in the stack */
	--stack->pos;
}


/*
 * Find next open route and follow it
 */
static gboolean
follow_next_open_route(struct solution *sol, struct stack *stack,
		       struct line *current, int direction)
{
	struct step *step=stack->step + stack->pos;
	struct line **line_list;
	int nlines;
	int route;

	/* which direction are we going */
	if (direction == DIRECTION_IN) {
		nlines= current->nin;
		line_list= current->in;
	} else {	// DIRECTION_OUT
		nlines= current->nout;
		line_list= current->out;
	}

	/* find next open route */
	for(route=0; route < nlines; ++route) {
		if ((step->routes&(1 << route)) != 0) continue;
		if (STATE(line_list[route]) == LINE_CROSSED) {
			/* don't follow an already crossed line */
			step->routes|= (1 << route);
			continue;
		}
		break;
	}
	if (route == nlines) return FALSE;	// tried all routes

	/* follow route */
	current= goto_next_line(current, &direction, route);
	g_assert(current != NULL);

	/* mark current route */
	step->routes|= (1 << route);

	/* setup next step */
	++stack->pos;
	g_assert(stack->pos < stack->size);
	++step;
	step->id= current->id;
	step->direction= direction;
	step->routes= 0;
	/* set next line ON */
	sol->states[current->id]= LINE_ON;

	return TRUE;
}


/*
 * Generate a brute_forte stack for solution given in 'sol'
 * Returns properly initialized stack ready to be used
 * Returns NULL if closed loop found on given solution
 *	NOTE: it does NOT check for closed loops, it returns NULL if it happens
 *	to find a closed loop while looking for a starting point
 */
struct stack*
brute_init_step_stack(struct solution *sol)
{
	struct geometry *geo=sol->geo;
	struct stack *stack;
	struct step *step;
	struct line *current;
	struct line *lin;
	int start_line=0;
	int count=0;
	int i;
	int direction;

	/* count how many lines are already on */
	for(i=0; i < geo->nlines; ++i) {
		if (sol->states[i] == LINE_ON)
			++count;
	}
	if (count == 0) {
		g_debug("zero lines ON. We need at least one line ON to start");
		return NULL;
	}

	/* create stack to record steps taken */
	stack= brute_create_step_stack(sol, geo->nlines - count + 1);

	/* find random line where to start */
	count= g_random_int_range(0, count);
	for(i=0; i < geo->nlines; ++i) {
		if (sol->states[i] == LINE_ON)
			--count;
		if (count < 0) {
			start_line= i;
			break;
		}
	}

	/* choose a direction randomly */
	if (g_random_int_range(0, 2) == 0) direction= DIRECTION_IN;
	else direction= DIRECTION_OUT;

	/* make sure we reach an open end */
	lin= geo->lines + start_line;
	do {
		current= lin;
		lin= follow_line(sol, lin, &direction);
	} while(lin != NULL && lin != geo->lines + start_line);

	if (lin != NULL) {
		/* there's a closed loop in given solution (no good) */
		brute_free_step_stack(stack);
		return NULL;
	}

	/* setup first step info */
	step= stack->step;
	step->id= current->id;
	step->direction= direction;
	step->routes= 0;

	return stack;
}


/*
 * Try brute force approach on given solution and stack
 * Return TRUE: solution found (solution in sol
 */
gboolean
brute_force_solve(struct solution *sol, struct stack *stack, gboolean trace_mode)
{
	struct geometry *geo=sol->geo;
	struct step *step;
	struct line *current;
	struct line *lin;
	struct line *next;
	int i;
	int direction;
	gboolean done=FALSE;
	int niter=0;

	while(stack->pos >= 0) {
		step= stack->step + stack->pos;
		current= geo->lines + step->id;
		direction= step->direction;
		++niter;

		/* restore initial not-ON states (reset crossed lines) */
		for(i=0; i < geo->nlines; ++i) {
			if (sol->states[i] != LINE_ON)
				sol->states[i]= stack->ini_states[i];
		}

		/* cross all crossable lines */
		(void)solve_cross_lines(sol);

		/* trace mode stop */
		if (trace_mode) {
			if (done) break;
			done= TRUE;
		}

		/* check validity of game */
		if (solve_check_valid_game(sol) == FALSE) {
			backtrack_step(sol, stack);
			continue;
		}

		/* see if line connects to another one (possible loop) */
		lin= follow_line(sol, current, &direction);
		if (lin != NULL) {	// line connects ahead
			/* follow last line added until loop or end */
			next= lin;
			while (next != NULL) {
				lin= next;
				next= follow_line(sol, lin, &direction);
				if (next == current) // closed loop found
					break;
			}
			/* closed loop!!! */
			if (next == current) {
				/* we may have a solution, check */
				if (check_single_loop(sol, current)) {
					/* found a solution, we're done */
					g_message("brute_force: took %d iterations", niter);
					return TRUE;
				} else {
					/* false alarm, backtrack */
					backtrack_step(sol, stack);
					continue;
				}
			} else {
				/* open loop, set open end to current */
				current= lin;
				// direction is already set
				/* let program continue normally */
			}
		}

		/*
		 * choose next line to try
		 */
		if (follow_next_open_route(sol, stack, current, direction) == FALSE) {
			/* no more open routes here */
			backtrack_step(sol, stack);
			continue;
		}
	}

	return FALSE;
}


/*
 * Test brute force
 */
int
brute_force_test(struct geometry *geo, struct game *game)
{
	static struct solution *sol;
	int i;
	static gboolean first=TRUE;
	static struct stack *stack=NULL;
	double score;

	/* Solve as much as we can */
	if (first) {
		sol= solve_game(geo, game, &score);
		first= FALSE;

		/* setup initial stack */
		stack= brute_init_step_stack(sol);
	}

	if (stack != NULL) {
		/* start brute force approach */
		brute_force_solve(sol, stack, FALSE);	// TRUE= one step at a time
	}


	/* copy solution on game state */
	for(i=0; i < geo->nlines; ++i)
		game->states[i]= sol->states[i];

	return 0;
}
