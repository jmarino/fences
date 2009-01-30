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



/* contains data to record each step taken (useful to backtrack) */
/* WARNING: 'int routes' limits number of possible line connections to 32 (or 64)
 * (seems reasonable) */
struct step {
	int id;
	int new_state;
	int old_state;
	int direction;
	int routes;	/* **NOTE** this is highly arch dependant */
};

/* contains stack of steps */
struct stack {
	struct step *step;
	int pos;
	int size;
};





/*
 * Allocate stack to record steps taken
 */
static struct stack*
brute_create_step_stack(int size)
{
	struct stack *stack;
	
	stack= (struct stack*)g_malloc(sizeof(struct stack));
	stack->step= (struct step*)g_malloc(size * sizeof(struct step));
	stack->pos= 0;
	stack->size= size;
	
	return stack;
}


/*
 * Free stack
 */
static void
brute_free_step_stack(struct stack *stack)
{
	g_free(stack->step);
	g_free(stack);
}


/*
 * Check game data for some insconsistencies
 * Check all numbered squares and vertices
 * Return TRUE: all fine
 * Return FALSE: found problem
 */
static gboolean
brute_force_check_valid(struct solution *sol)
{
	int i, j;
	int lines_on;
	struct square *sq;
	struct vertex *vertex;
	struct geometry *geo=sol->geo;
	
	/* check all numbered squares for lines > (squaren num) */
	for(i=0; i < geo->nsquares; ++i) {
		/* only numbered squares */
		if (sol->numbers[i] == -1) continue;
		
		/* count lines ON around square */
		sq= geo->squares + i;
		lines_on= 0;
		for(j=0; j < sq->nsides; ++j) {
			if (STATE(sq->sides[j]) == LINE_ON)
				++lines_on;
			if (lines_on > sol->numbers[i]) return FALSE;
		}
		/* if (lines ON) == (square number) -> square is done cross the rest */
		
	}
	
	/* check all vertices for one with more than 2 lines */
	for(i=0; i < geo->nvertex; ++i) {
		vertex= geo->vertex + i;
		lines_on= 0;
		for(j=0; j < vertex->nlines; ++j) {
			if (STATE(vertex->lines[j]) == LINE_ON)
				++lines_on;
			if (lines_on > 2) return FALSE;
		}
	}
	 
	return TRUE;
}


/*
 * Check for loops
 * Return TRUE: solution found
 * Return FALSE: problem (partial loop, or isolated loop)
 */
static gboolean
brute_force_check_loop(struct solution *sol, int id)
{
	int i;
	int nlines=0;
	int direction;
	struct line *start;
	struct line *lin;
	struct geometry *geo=sol->geo;
	
	/* reset line mask */
	for(i=0; i < geo->nlines; ++i) {
		if (sol->states[i] == LINE_ON) {
			sol->lin_mask[i]= TRUE;
			++nlines;
		} else sol->lin_mask[i]= FALSE;
	}
	
	while(nlines > 0) {
		/* find an ON line that we haven't visited yet */
		for(i=0; i < geo->nlines; ++i) {
			if (sol->lin_mask[i]) break;
		}
		g_assert(i < geo->nlines);
		lin= start= geo->lines + i;
		direction= DIRECTION_IN;
		while (lin != NULL) {
			sol->lin_mask[lin->id]= FALSE;
			--nlines;
			lin= follow_line(sol, lin, &direction);
			if (lin == start) // closed loop
				break;
		}
		if (lin == start) {	// closed loop
			if (nlines > 0) return FALSE;	// loop + some lines -> bad
			return TRUE;
		}
	}
	return FALSE;
}


/*
 * Brute force solve
 */
static int
brute_force(struct solution *sol)
{
	struct geometry *geo=sol->geo;
	struct stack *stack;
	struct step *step;
	struct line *current;
	struct line *lin;
	struct line **list;
	int nlist;
	int start_line=0;
	int count=0;
	int i;
	int direction;
	int num_solutions=0;
	int step_valid;
	
	/* count how many lines are already on */
	for(i=0; i < geo->nlines; ++i) {
		if (sol->states[i] == LINE_ON)
			++count;
	}
	if (count == 0) {
		g_error("we need at least one line ON to start");
	}
	
	/* create stack to record steps taken */
	stack= brute_create_step_stack(geo->nlines - count + 1);
	
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

	/* make sure we reach an open end before we start the brute force */
	lin= geo->lines + start_line;
	current= NULL;
	while(lin != NULL && current != geo->lines + start_line) {
		current= lin;
		lin= follow_line(sol, lin, &direction);
	}
	if (lin != NULL) {	// there's a closed loop
		g_error("attempted to start brute force on a closed loop (line %d)", 
			start_line);
	}
	
	/* setup first step info */
	start_line= current->id;
	step= stack->step;
	step->id= start_line;
	step->old_state= LINE_ON;
	step->new_state= LINE_ON;
	step->direction= direction;
	step->routes= 0;
	current= geo->lines + start_line;
	
	/* main loop */
	while(1) {
		/*
		 * check if current solution has any problems 
		 */
		step_valid= brute_force_check_valid(sol);
		if (step_valid) {
			step_valid= brute_force_check_loop(sol, current->id);
			if (step_valid) {
				/* found a solution, record it and backtrack */
				++num_solutions;
				
				/* restore old state */
				sol->states[current->id]= step->old_state;
			
				/* go to previous step in the stack */
				--stack->pos;
				if (stack->pos < 0) break;	// give up
				step= stack->step + stack->pos;
				current= geo->lines + step->id;
				direction= step->direction;
				step->routes<<= 1;
				step->routes|= 1;
				continue;
			}
		}
		/* found problem -> backtrack */
		if (!step_valid) {
			/* restore old state */
			sol->states[current->id]= step->old_state;
			
			/* go to previous step in the stack */
			--stack->pos;
			if (stack->pos < 0) break;	// give up
			step= stack->step + stack->pos;
			current= geo->lines + step->id;
			direction= step->direction;
			continue;
		}
		
		/*
		 * choose next line to try 
		 */
		if (step->direction == DIRECTION_IN) {
			nlist= current->nin;
			list= current->in;
		} else {	// DIRECTION_OUT
			nlist= current->nout;
			list= current->out;
		}
		
		/* find next open route */
		for(i=0; i < nlist; ++i) {
			if ((step->routes&(1 << i)) != 0) continue;
			if (STATE(list[i]) == LINE_CROSSED) {
				/* don't follow an already crossed line */
				step->routes|= (1 << i);
				continue;
			}
			break;
		}
		if (i == nlist) {	// tried all routes
			/* restore old state */
			sol->states[current->id]= step->old_state;
			
			/* go to previous step in the stack */
			--stack->pos;
			if (stack->pos < 0) break;	// give up
			step= stack->step + stack->pos;
			current= geo->lines + step->id;
			direction= step->direction;
			continue;
		}
		/* follow route */
		current= goto_next_line(current, &direction, i);
		g_assert(current != NULL);
		
		/* mark current route */
		step->routes|= (1 << i);
		
		/* setup next step */
		++stack->pos;
		g_assert(stack->pos < stack->size);
		step= stack->step + stack->pos;
		step->id= current->id;
		step->old_state= sol->states[current->id];
		step->new_state= LINE_ON;
		step->direction= direction;
		step->routes= 0;
		/* set next line ON */
		sol->states[current->id]= LINE_ON;
	}
	brute_free_step_stack(stack);
	
	return num_solutions;
}


int
brute_force_solve(struct geometry *geo, struct game *game)
{
	struct solution *sol;
	int nsolution;
	int i;
	
	/* create empty solution structure */
	sol= solve_create_solution_data(geo, game);
	
	/* Run a set of trivial steps to get a head start */
	(void)solve_handle_zero_squares(sol);
	(void)solve_handle_maxnumber_squares(sol);
	(void)solve_handle_busy_vertex(sol);
	(void)solve_handle_trivial_squares(sol);
	(void)solve_handle_trivial_vertex(sol);
	(void)solve_handle_busy_vertex(sol);
	
	/* start brute force approach */
	nsolution= brute_force(sol);

	printf("brute force: %d solutions\n", nsolution);
	
	for(i=0; i < geo->nlines; ++i)
		game->states[i]= sol->states[i];
	
	
	return 0;
}
