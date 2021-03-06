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
#include "brute-force.h"

#include <stdio.h>

#define TILE_HIDDEN		0
#define TILE_VISIBLE		1
#define TILE_FIXED		2
#define TILE_TEMPORARY	3


/*
 * structure that contains new game data
 */
struct newgame {
	int *loop;				// line states defining the game loop
	int *all_numbers;		// numbers inside tiles
	int *tile_mask;			// tiles with visible number
	int nvisible;			// number of tiles with visible number
	int nhidden;			// number of tiles not made visible (hidden)

	/* current status of new game */
	struct game *game;
	/* solution data structure */
	struct solution *sol;
	int max_level;
};


/*
 * Pick random tile out of hidden ones and mark it as
 * visible temporarily: TILE_TEMPORARY
 * Returns ID of chosen tile
 */
static int
pick_random_hidden_tile(struct newgame *newgame)
{
	int count;
	int index=-1;

	count= g_random_int_range(0, newgame->nhidden);
	while(count >= 0) {
		++index;
		if (newgame->tile_mask[index] == TILE_HIDDEN)
			--count;
	}
	newgame->tile_mask[index]= TILE_TEMPORARY;
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
	   delete tiles */
	if (newgame->sol->difficulty < 6.0) {
	    index= -1;
		for(i=0; i < newgame->nvisible; ++i) {
			for(j=index + 1; j < geo->ntiles; ++j)
				if (newgame->tile_mask[j] == TILE_VISIBLE)
					break;
			g_assert(j < geo->ntiles);
			index= j;
			newgame->tile_mask[index]= TILE_HIDDEN;
			newgame->game->numbers[index]= -1;
			solve_reset_solution(newgame->sol);
			solve_game_solution(newgame->sol, newgame->max_level);
			if (newgame->sol->solved && newgame->sol->difficulty > prev_difficulty) {
				--newgame->nvisible;
				++newgame->nhidden;
				printf("**eliminated one number\n");
				prev_difficulty= newgame->sol->difficulty;
			} else {
				/* restore tile */
				newgame->tile_mask[index]= TILE_VISIBLE;
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
	struct tile *tile;
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

	/* count number of lines touching all tiles */
	newgame->all_numbers= (int*)g_malloc(geo->ntiles * sizeof(int));
	newgame->tile_mask= (int*)g_malloc(geo->ntiles * sizeof(int));
	for(i=0; i < geo->ntiles; ++i) {
		newgame->all_numbers[i]= 0;
		newgame->tile_mask[i]= TILE_HIDDEN;
		tile= geo->tiles + i;
		for(j=0; j < tile->nsides; ++j) {
			if (newgame->loop[tile->sides[j]->id] == LINE_ON)
				++(newgame->all_numbers[i]);
		}
	}
	newgame->nvisible= 0;
	newgame->nhidden= geo->ntiles;
	/* maximum solution level to allow with aimed difficulty */
	/* **TODO** fix this hack */
	newgame->max_level= 5;

	/* populate solution structure */
	newgame->sol= solve_create_solution_data(geo, newgame->game);
	sol= newgame->sol;

	/* reset game states and previous solution states */
	memset(sol->states, 0, geo->nlines * sizeof(int));
	memset(sol->lin_mask, 0, geo->nlines * sizeof(int));
	memset(sol->tile_done, 0, geo->ntiles * sizeof(gboolean));

	/* main loop */
	while(1) {
		if (newgame->nhidden == 0) {
			printf("nhidden = 0. Stop.\n");
			break;
		}
		/* pick a random tile to show */
	    index= pick_random_hidden_tile(newgame);
		tile= geo->tiles + index;

		/* solve current game */
		solve_zero_tiles(sol);
		if (sol->nchanges == 0)
			solve_maxnumber_tiles(sol);
		if (sol->nchanges == 0) {
			/* run just 1 step of solution */
			solution_loop(sol, 1, newgame->max_level);
		}
		/* something was done */
		if (sol->nchanges > 0) {
			newgame->tile_mask[index]= TILE_VISIBLE;
			/* mark also any other tiles that may have been involved */
			for(i=0; i < sol->ntile_changes; ++i) {
				if (sol->tile_changes[i] == index) continue;
				newgame->tile_mask[sol->tile_changes[i]]= TILE_VISIBLE;
			}
			/* clear all temporary tiles */
			for(i=0; i < geo->ntiles; ++i) {
				if (newgame->tile_mask[i] == TILE_TEMPORARY) {
					newgame->tile_mask[i]= TILE_HIDDEN;
					newgame->game->numbers[i]= -1;
					++newgame->nhidden;
					--newgame->nvisible;
				}
			}
			/* restart solution and solve as far as possible */
			solve_reset_solution(sol);

			solve_zero_tiles(sol);
			solve_maxnumber_tiles(sol);

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

	/* trim any not needed tiles */
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
	memcpy(game->solution, newgame->loop, geo->nlines * sizeof(int));
	game->nlines_on= 0;
	game->solution_nlines_on= 0;
	for(i=0; i < geo->nlines; ++i) {
		game->states[i]= LINE_OFF;
		if (game->solution[i] == LINE_ON)
			++game->solution_nlines_on;
	}
	/* free solution */
	solve_free_solution_data(sol);

	/* free new game structure */
	g_free(newgame->loop);
	g_free(newgame->all_numbers);
	g_free(newgame->tile_mask);
	//g_free(newgame->zero_pos);
	g_free(newgame);

	return game;
}
