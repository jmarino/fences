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



/*
 * Build new game
 */
struct game*
build_new_game(struct geometry *geo, int difficulty)
{
	struct game *game;
	int i, j;
	int nsquares;
	int count;
	int nlines_on;
	int id;
	
	/* create empty game */
	game= create_empty_gamedata(geo);
	
	/* create random loop: result is in 'game' */
	build_new_loop(geo, game);
	
	/* enable random squares */
	nsquares= (int) (geo->nsquares * 0.40);
	for(i=0; i < nsquares; ++i) {
		count= g_random_int_range(0, geo->nsquares - i);
		for(id=0; id < geo->nsquares; ++id) {
			if (game->numbers[id] == -1) {
				--count;
				if (count < 0) break;
			}
		}
		g_debug("id: %d / %d (%d)", id, geo->nsquares, count);
		g_assert(id < geo->nsquares);
		/* count on lines around square 'id' */
		nlines_on= 0;
		for(j=0; j < geo->squares[id].nsides; ++j) {
			if (game->states[geo->squares[id].sides[j]->id] == LINE_ON)
				++nlines_on;
		}
		game->numbers[id]= nlines_on;
	}
	
	/* clear lines of game */
	//for(i=0; i < geo->nlines; ++i)
//		game->states[i]= LINE_OFF;
	
	return game;
}
