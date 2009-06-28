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

#include "gamedata.h"
#include "game-solver.h"
#include "build-loop.h"
#include "brute-force.h"


#define SQUARE_HIDDEN		0
#define SQUARE_VISIBLE		1
#define SQUARE_FIXED		2
#define SQUARE_TEMPORARY	3


/*
 * structure that contains new game data
 */
struct newgame {
	int *loop;				// line states defining the game loop
	int *all_numbers;		// numbers inside squares
	int *sq_mask;			// squares with visible number
	int nvisible;			// number of squares with visible number
	int nhidden;			// number of squares not made visible (hidden)

	/* current status of new game */
	struct game *game;
	/* solution data structure */
	struct solution *sol;
	int max_level;
};


/*
 * Pick random square out of hidden ones and mark it as
 * visible temporarily: SQUARE_TEMPORARY
 * Returns ID of chosen square
 */
static int
pick_random_hidden_square(struct newgame *newgame)
{
	int count;
	int index=-1;

	count= g_random_int_range(0, newgame->nhidden);
	while(count >= 0) {
		++index;
		if (newgame->sq_mask[index] == SQUARE_HIDDEN)
			--count;
	}
	newgame->sq_mask[index]= SQUARE_TEMPORARY;
	newgame->game->numbers[index]= newgame->all_numbers[index];
	--newgame->nhidden;
	++newgame->nvisible;

	return index;
}


/*
 * Eliminate useless numbers from game
 */
static void
newgame_trim_game(struct newgame *newgame, struct geometry *geo)
{
	int prev_difficulty=newgame->sol->difficulty;
	int index;
	int i, j;

	/* if difficulty is not good enough, go over the current game and
	   delete squares */
	if (newgame->sol->difficulty < 6.0) {
	    index= -1;
		for(i=0; i < newgame->nvisible; ++i) {
			for(j=index + 1; j < geo->nsquares; ++j)
				if (newgame->sq_mask[j] == SQUARE_VISIBLE)
					break;
			g_assert(j < geo->nsquares);
			index= j;
			newgame->sq_mask[index]= SQUARE_HIDDEN;
			newgame->game->numbers[index]= -1;
			solve_reset_solution(newgame->sol);
			solve_game_solution(newgame->sol, newgame->max_level);
			if (newgame->sol->solved && newgame->sol->difficulty > prev_difficulty) {
				--newgame->nvisible;
				++newgame->nhidden;
				printf("**eliminated one number\n");
				prev_difficulty= newgame->sol->difficulty;
			} else {
				/* restore square */
				newgame->sq_mask[index]= SQUARE_VISIBLE;
				newgame->game->numbers[index]=
					newgame->all_numbers[index];
			}
		}
	}
}


/*
 * Build new game
 */
struct game*
build_new_game(struct geometry *geo, double difficulty)
{
	int i, j;
	int index;
	struct newgame *newgame;
	struct square *sq;
	gboolean found=FALSE;
	struct solution *sol;

	/* create new game structure */
	newgame= (struct newgame*)g_malloc(sizeof(struct newgame));

	/* create empty game */
	newgame->game= create_empty_gamedata(geo);

	/* create random loop: result is in 'game' */
	build_new_loop(geo, newgame->game, FALSE);

	/* **TODO** build_new_loop has to do extra work to put loop in game, now
	   we have to do more work to take loop out of game, maybe we could
	   optimize this and save some work (returning loop directly) */
	newgame->loop= (int*)g_malloc(geo->nlines * sizeof(int));
	memcpy(newgame->loop, newgame->game->states, geo->nlines * sizeof(int));
	memset(newgame->game->states, 0, geo->nlines * sizeof(int));

	/* count number of lines touching all squares */
	newgame->all_numbers= (int*)g_malloc(geo->nsquares * sizeof(int));
	newgame->sq_mask= (int*)g_malloc(geo->nsquares * sizeof(int));
	for(i=0; i < geo->nsquares; ++i) {
		newgame->all_numbers[i]= 0;
		newgame->sq_mask[i]= SQUARE_HIDDEN;
		sq= geo->squares + i;
		for(j=0; j < sq->nsides; ++j) {
			if (newgame->loop[sq->sides[j]->id] == LINE_ON)
				++(newgame->all_numbers[i]);
		}
	}
	newgame->nvisible= 0;
	newgame->nhidden= geo->nsquares;
	/* maximum solution level to allow with aimed difficulty */
	/* **TODO** fix this hack */
	newgame->max_level= 5;

	/* populate solution structure */
	newgame->sol= solve_create_solution_data(geo, newgame->game);
	sol= newgame->sol;

	/* reset game states and previous solution states */
	memset(sol->states, 0, geo->nlines * sizeof(int));
	memset(sol->lin_mask, 0, geo->nlines * sizeof(int));
	memset(sol->sq_handled, 0, geo->nsquares * sizeof(gboolean));

	/* main loop */
	while(1) {
		if (newgame->nhidden == 0) {
			printf("nhidden = 0. Stop.\n");
			break;
		}
		/* pick a random square to show */
	    index= pick_random_hidden_square(newgame);
		sq= geo->squares + index;

		/* solve current game */
		solve_zero_squares(sol);
		if (sol->nchanges == 0)
			solve_maxnumber_squares(sol);
		if (sol->nchanges == 0) {
			/* run just 1 step of solution */
			solution_loop(sol, 1, newgame->max_level);
		}
		/* something was done */
		if (sol->nchanges > 0) {
			newgame->sq_mask[index]= SQUARE_VISIBLE;
			/* mark also any other squares that may have been involved */
			for(i=0; i < sol->nsq_changes; ++i) {
				if (sol->sq_changes[i] == index) continue;
				newgame->sq_mask[sol->sq_changes[i]]= SQUARE_VISIBLE;
			}
			/* clear all temporary squares */
			for(i=0; i < geo->nsquares; ++i) {
				if (newgame->sq_mask[i] == SQUARE_TEMPORARY) {
					newgame->sq_mask[i]= SQUARE_HIDDEN;
					newgame->game->numbers[i]= -1;
					++newgame->nhidden;
					--newgame->nvisible;
				}
			}
			/* restart solution and solve as far as possible */
			solve_reset_solution(sol);

			solve_zero_squares(sol);
			solve_maxnumber_squares(sol);

			/* run solution loop with no limits */
			solution_loop(sol, -1, newgame->max_level);

			/* check if we have a full solution */
			for (i=0; i < geo->nlines; ++i) {
				if ((newgame->loop[i] == LINE_ON
					 && sol->states[i] != LINE_ON)
					|| (newgame->loop[i] == LINE_OFF
						&& sol->states[i] == LINE_ON)) {
					break;
				}
			}
			if (i == geo->nlines) {
				found= TRUE;
				break;
			}

		}
	}

	/* reset and solve to get a meassure of difficulty */
	solve_reset_solution(sol);
	solve_game_solution(sol, -1);

	/* trim any not needed squares */
	newgame_trim_game(newgame, geo);

	/* reset and solve to get a meassure of difficulty */
	solve_reset_solution(sol);
	solve_game_solution(sol, -1);

	if (found) {
		printf("YAY. Found a solution! (%6.4lf)\n", sol->difficulty);
	}

	struct game *game;

	/* put new game in game structure */
	game= newgame->game;
	memcpy(game->states, sol->states, geo->nlines * sizeof(int));
	memcpy(game->states, newgame->loop, geo->nlines * sizeof(int));
	/*memset(game->states, 0, geo->nlines * sizeof(int));
	for(i=0; i < geo->nsquares; ++i)
		if (newgame->sq_mask[i] != SQUARE_HIDDEN)
			game->numbers[i]= newgame->all_numbers[i];
		else
			game->numbers[i]= -1;
	*/
	/* free solution */
	solve_free_solution_data(sol);

	/* free new game structure */
	g_free(newgame->loop);
	g_free(newgame->all_numbers);
	g_free(newgame->sq_mask);
	//g_free(newgame->zero_pos);
	g_free(newgame);

	return game;
}
