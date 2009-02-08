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

#include "gamedata.h"
#include "game-solver.h"
#include "build-loop.h"
#include "brute-force.h"


#define SQUARE_HIDDEN	0
#define SQUARE_VISIBLE	1
#define SQUARE_FIXED	2


/*
 * Build new game
 */
struct game*
build_new_game(struct geometry *geo, double difficulty)
{
	struct game *game;
	int i, j;
	int count;
	int *loop;
	int *numbers;		// number of lines touching each square
	int *num_mask;		// is square number visible or disabled?
	int nvisible;		// number of visible squares
	int nfixed;		// number of squares fixed (can't be hidden)
	double score;
	int sq_id;
	struct solution *sol;
	double max_diff=0.;
	
	/* create empty game */
	game= create_empty_gamedata(geo);
	
	/* create random loop: result is in 'game' */
	build_new_loop(geo, game);
	loop= (int*)g_malloc(geo->nlines * sizeof(gboolean));
	for(i=0; i < geo->nlines; ++i)
		loop[i]= game->states[i];
	
	/* count number of lines touching all squares */
	numbers= (int*)g_malloc(geo->nsquares * sizeof(int));
	num_mask= (int*)g_malloc(geo->nsquares * sizeof(int));
	for(i=0; i < geo->nsquares; ++i) {
		/* count on lines around square 'id' */
		numbers[i]= 0;
		num_mask[i]= SQUARE_VISIBLE;	// all visible at the start
		for(j=0; j < geo->squares[i].nsides; ++j) {
			if (game->states[geo->squares[i].sides[j]->id] == LINE_ON)
				++(numbers[i]);
		}
		game->numbers[i]= numbers[i];
	}
	nvisible= geo->nsquares;
	nfixed= 0;
	
	/* HACK */
	difficulty= 2.5;
	
	while(nvisible - nfixed > 0) {
		/* select random square to hide */
		count= g_random_int_range(0, nvisible - nfixed);
		for(sq_id=0; sq_id < geo->nsquares; ++sq_id) {
			if (num_mask[sq_id] == SQUARE_VISIBLE)
				--count;
			if (count < 0) break;
		}
		g_assert(sq_id < geo->nsquares);
		
		/* hide square */
		game->numbers[sq_id]= -1;
		
		/* Solve current game as it is */
		sol= solve_game(geo, game, &score);
	
		/* check solution */
		for(i=0; i < geo->nlines; ++i) {
			if (sol->states[i] == LINE_ON) {
				if (loop[i] != LINE_ON)
					break;
			} else {
				if (loop[i] == LINE_ON)
					break;
			}
		}
		solve_free_solution_data(sol);
		
		/* wrong solution or wrong difficulty */
		if (i < geo->nlines || score > difficulty) {
			/* restore hidden square and fix it */
			game->numbers[sq_id]= numbers[sq_id];
			num_mask[sq_id]= SQUARE_FIXED;
			++nfixed;
			if (nfixed == nvisible) break;
		} else {
			/* mark square as hidden, unfix squares and continue */
			num_mask[sq_id]= SQUARE_HIDDEN;
			--nvisible;
			nfixed= 0;
			for (j=0; j < geo->nsquares; ++j) {
				if (num_mask[j] == SQUARE_FIXED) 
					num_mask[j]= SQUARE_VISIBLE;
			}
			max_diff= score;
		}
		
		printf("new game (%d - %d): score %lf\n", nvisible, nfixed, score);
	}

	/* Set correct difficulty */
	score= max_diff;
	printf("new game: score %lf\n", score);
	
	/* clear lines of game */
	//for(i=0; i < geo->nlines; ++i)
//		game->states[i]= LINE_OFF;
	
	g_free(loop);
	g_free(numbers);
	g_free(num_mask);
	
	return game;
}
