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
 * Simplifies to: n*(n-1)*...*(n-k+1) / k!
 * Example: 4 side tile with number 2 --> k= 2, n= 4
 */
static int
number_combinations(int n, int k)
{
	int i;
	int fac;
	int result;

	/* n * (n-1) * ... * (n-k+1) */
	result= n;
	for(i=n-1; i > n - k; --i) result*= i;

	/* k! */
	fac= k;
	for(i=k-1; i > 1; --i) fac*= i;
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
set_combination(struct solution *sol, struct tile *tile, int n, int k, int comb)
{
	int i, j;
	int start;
	int nline=0;
	int spaces=0;
	int lines_mask=0;

	start= comb % n;
	spaces= comb/n;

	/* find first available line */
	while(STATE(tile->sides[nline]) != LINE_OFF)
		nline= (nline + 1) % tile->nsides;

	/* jump start lines */
	for(i=0; i < start; ++i) {
		while(STATE(tile->sides[nline]) != LINE_OFF)
			nline= (nline + 1) % tile->nsides;
		nline= (nline + 1) % tile->nsides;
	}

	/* set k lines on */
	for(i=0; i < k; ++i) {
		/* find next available line and set it */
		while(STATE(tile->sides[nline]) != LINE_OFF)
			nline= (nline + 1) % tile->nsides;
		sol->states[tile->sides[nline]->id]= LINE_ON;
		lines_mask|= 1 << nline;
		nline= (nline + 1) % tile->nsides;
		while(STATE(tile->sides[nline]) != LINE_OFF)
			nline= (nline + 1) % tile->nsides;

		/* jump spaces */
		for(j=0; j < spaces; ++j) {
			while(STATE(tile->sides[nline]) != LINE_OFF)
				nline= (nline + 1) % tile->nsides;
			nline= (nline + 1) % tile->nsides;
		}
	}
	return lines_mask;
}


/*
 * Solve combination with look-ahead level 0
 * Try only one iteration of trivial vertex and trivial tile
 * Returns TRUE if solution found is valid
 */
static gboolean
combination_solve0(struct solution *sol)
{
	(void)solve_cross_lines(sol);
	(void)solve_trivial_vertex(sol);
	(void)solve_trivial_tiles(sol);

	return solve_check_valid_game(sol);
}


/*
 * Solve combination with look-ahead level 1
 * Try a couple of iterations of trivial vertex and trivial tile
 * Returns TRUE if solution found is valid
 */
static gboolean
combination_solve1(struct solution *sol)
{
	gboolean valid=TRUE;
	int count=1;

	while(valid && count > 0) {
		(void)solve_cross_lines(sol);
		solve_trivial_vertex(sol);
		count= sol->nchanges;
		solve_trivial_tiles(sol);
		count+= sol->nchanges;
		valid= solve_check_valid_game(sol);
	}
	return valid;
}


/*
 * Solve combination with look-ahead level 2
 * Try something close to regular solution (checking validity at each step)
 * Only go 5 iterations forward.
 * Returns TRUE if solution found is valid
 */
static gboolean
combination_solve2(struct solution *sol)
{
	gboolean valid=TRUE;
	int level=0;
	int iter=0;

	while(valid && level <= 5 && iter < 5) {
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
		}

		/* if nothing found go to next level */
		if (sol->nchanges == 0) {
			++level;
		} else {
			valid= solve_check_valid_game(sol);
			level= 0;
			++iter;
		}
	}

	return valid;
}


/*
 * Test all possible combinations in one tile
 * Level: how far ahead to look after trying a combination
 * Returns number of lines succesfully modified after trying all combinations
 * NOTE: line masks are sizeof(int) bits which limits max number of lines
 *	but no safety checks exist to ensure num of lines < sizeof(int)
 */
static int
test_tile_combinations(struct solution *sol, struct solution *sol_bak,
			 int tile_num, int level)
{
	struct tile *tile;
	int nlines_off=0;
	int nlines_todo=0;
	int lines_mask;		// lines always ON in all valid combinations
	int bad_lines;		// lines always ON in all invalid combinations
	int all_lines=0;	// lines attempted
	int tmp_mask;
	int ncomb;
	int count=0;
	int i;
	gboolean valid=TRUE;

	tile= sol->geo->tiles + tile_num;
	/* count lines ON and OFF in tile */
	for(i=0; i < tile->nsides; ++i) {
		if (STATE(tile->sides[i]) == LINE_OFF)
			++nlines_off;
		else if (STATE(tile->sides[i]) == LINE_ON)
			++nlines_todo;
	}

	/* get number of possible combinations */
	nlines_todo= sol->numbers[tile_num] - nlines_todo;
	ncomb= number_combinations(nlines_off, nlines_todo);

	/* try every different combination */
	lines_mask= ~0;		// 0xFFFF
	bad_lines= ~0;
	for(i=0; i < ncomb; ++i) {
		/* enable lines for this combination */
		/* lines_mask only keeps lines that are always on */
		tmp_mask= set_combination(sol, tile, nlines_off, nlines_todo, i);
		all_lines|= tmp_mask;

		/* try to solve a bit (limited by look-ahead level) */
		switch(level) {
			case 0: valid= combination_solve0(sol);
			break;
			case 1: valid= combination_solve1(sol);
			break;
			case 2: valid= combination_solve2(sol);
			break;
			default:
				g_debug("wrong combinations look-ahead level (%d)", level);
		}

		/* if solution found is valid, track line states */
		if (valid) {
			lines_mask&= tmp_mask;
			bad_lines&= ~(tmp_mask); // cross current combination out
		} else {
			bad_lines&= tmp_mask;	// mask with bad combination
		}

		/* restore initial solution state */
		solve_copy_solution(sol, sol_bak);
	}
	/* normalize mask of lines that are always ON in all invalid cases */
	bad_lines&= all_lines;

	/* after trying all combinations see if a line was always on */
	i= 0;
	while(lines_mask != 0 || bad_lines != 0) {
		/* set lines in mask */
		if (lines_mask&1) {
			SET_LINE(tile->sides[i]);
		} else if (bad_lines&1)
			CROSS_LINE(tile->sides[i]);
		lines_mask>>= 1;
		bad_lines>>= 1;
		++i;
	}

	/* a line has been set */
	return count;
}


/*
 * Try all possible combinations of lines around a numbered tile
 * For each try, test the validity of the game
 * Discard permutations that produce an invalid game.
 * Any line that is ON for all valid permutations, is set to ON
 * Level: how far ahead to look after trying a combination
 */
int
solve_try_combinations(struct solution *sol, int level)
{
	int i;
	struct solution *sol_bak;	// backup solution
	int count=0;
	struct geometry *geo=sol->geo;

	/* make a backup of current solution state */
	sol_bak= solve_duplicate_solution(sol);

	/* iterate over all tiles */
	for(i=0; i < geo->ntiles; ++i) {
		/* ignore handled tiles or tiles with no number */
		if (sol->tile_done[i] || sol->numbers[i] == -1)
			continue;

		/* Test all combinations for tile and see if all valid ones
		 have a line always ON or OFF. */
		count= test_tile_combinations(sol, sol_bak, i, level);

		/* TRUE -> a line has been set, we're done */
		if (count > 0)
			break;
	}

	/* free solution backup */
	solve_free_solution_data(sol_bak);

	return count;
}
