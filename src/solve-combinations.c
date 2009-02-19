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
 * Get number of combinations
 * How many combinations (of size k) in a set of size n
 * Formula: n! / ( k! (n - k)! )
 * Example: square with number 2 --> k= 2, n= 4
 */
static int
number_combinations(int n, int k)
{
	int i;
	int fac;
	int result;
	
	/* n! */
	result= n;
	for(i=n-1; i > 1; --i) result*= i;
	
	/* k! */
	fac= k;
	for(i=k-1; i > 1; --i) fac*= i;
	if (fac == 0) fac= 1;
	result/= fac;
	
	/* (n - k)! */
	fac= n - k;
	for(i=n - k - 1; i > 1; --i) fac*= i;
	if (fac == 0) fac= 1;
	
	return result/fac;
}


/*
 * Set combination number 'comb' 
 * The combinations are of 'k' size chosen out of 'n' elements
 * Returns a mask indicating which lines are on
 * Example: k= 2; n= 4
 *	x x - -		comb=0 ; start=0; spaces=0;
 *	- x x -		comb=1 ; start=1; spaces=0;
 *	- - x x		comb=2 ; start=2; spaces=0;
 *	x - - x		comb=3 ; start=3; spaces=0;
 *	x - x -		comb=4 ; start=0; spaces=1;
 *	- x - x		comb=5 ; start=1; spaces=1;
 */
static int
set_combination(struct solution *sol, struct square *sq, int n, int k, int comb)
{
	int i, j;
	int start;
	int nline=0;
	int spaces=0;
	int lines_mask=0;
	
	start= comb % n;
	spaces= comb/n;

	/* find first available line */
	while(STATE(sq->sides[nline]) != LINE_OFF) 
		nline= (nline + 1) % sq->nsides;
	
	/* jump start lines */
	for(i=0; i < start; ++i) {
		while(STATE(sq->sides[nline]) != LINE_OFF) 
			nline= (nline + 1) % sq->nsides;
		nline= (nline + 1) % sq->nsides;
	}
	
	/* set k lines on */
	for(i=0; i < k; ++i) {
		/* find next available line and set it */
		while(STATE(sq->sides[nline]) != LINE_OFF) 
			nline= (nline + 1) % sq->nsides;
		sol->states[sq->sides[nline]->id]= LINE_ON;
		lines_mask|= 1 << nline;
		nline= (nline + 1) % sq->nsides;
		while(STATE(sq->sides[nline]) != LINE_OFF) 
			nline= (nline + 1) % sq->nsides;
		
		/* jump spaces */
		for(j=0; j < spaces; ++j) {
			while(STATE(sq->sides[nline]) != LINE_OFF) 
				nline= (nline + 1) % sq->nsides;
			nline= (nline + 1) % sq->nsides;
		}
	}
	return lines_mask;
}


/*
 * Try all possible combinations of lines around a numbered square
 * For each try, test the validity of the game
 * Discard permutations that produce an invalid game.
 * Any line that is ON for all valid permutations, is set to ON
 */
int
solve_try_combinations(struct solution *sol)
{
	int i, j;
	int *good_states;	// initial game states (back-up)
	int count=0;
	int nlines_off;
	int nlines_todo;
	int lines_mask;		// mask of lines that are on (**HACK** 32 max lines)
	int tmp_mask;
	int ncomb;
	struct square *sq;
	struct geometry *geo=sol->geo;
	
	/* store copy of the game state */
	good_states= (int*)g_malloc(geo->nlines * sizeof(int));
	for(i=0; i < geo->nlines; ++i) {
		good_states[i]= sol->states[i];
	}
	
	/* iterate over all squares */
	for(i=0; i < geo->nsquares; ++i) {
		/* ignore handled squares or squares with no number */
		if (sol->sq_mask[i] == FALSE || sol->numbers[i] == -1)
			continue;
		
		sq= geo->squares + i;
		/* count lines ON and OFF in square */
		nlines_todo= nlines_off= 0;
		for(j=0; j < sq->nsides; ++j) {
			if (STATE(sq->sides[j]) == LINE_OFF)
				++nlines_off;
			else if (STATE(sq->sides[j]) == LINE_ON)
				++nlines_todo;
		}
		
		/* get number of possible combinations */
		nlines_todo= sol->numbers[i] - nlines_todo;
		ncomb= number_combinations(nlines_off, nlines_todo);
		
		/* try every different combination */
		lines_mask= !0;
		for(j=0; j < ncomb; ++j) {
			/* enable lines for this combination */
			/* lines_mask only keeps lines that are always on */
			tmp_mask= set_combination(sol, sq, nlines_off, nlines_todo, j);
			
			/* cross all possible lines */
			(void)solve_cross_lines(sol);
			
			/* check validity of combination */
			if (solve_check_valid_game(sol)) {
				lines_mask&= tmp_mask;
			}
			
			/* restore initial state */
			memcpy(sol->states, good_states, geo->nlines*sizeof(int));
		}
		
		/* after trying all combinations see if a line was always on */
		j= 0;
		while(lines_mask != 0) {
			/* set lines in mask */
			if (lines_mask&1) 
				SET_LINE(sq->sides[j]);
			lines_mask>>= 1;
			++j;
		}
		/* a line has been set, cross lines and update good_states */
		if (j > 0) {
			(void)solve_cross_lines(sol);
			memcpy(good_states, sol->states, geo->nlines*sizeof(int));
		}
	}
	
	g_free(good_states);

	return count;
}
