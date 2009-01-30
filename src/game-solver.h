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

#ifndef __INCLUDED_GAME_SOLVER_H__
#define __INCLUDED_GAME_SOLVER_H__



/*
 * Contains detail about game solution
 */
struct solution {
	struct geometry *geo;	// geometry of game
	struct game *game;	// game state (mostly useful for square numbers)
	int *states;		// line states (so we don't touch game->states)
	int *numbers;		// points to game->numbers
	gboolean *sq_mask;
	int *lin_mask;
};


/* 
 * Functions
 */
struct line* follow_line(struct solution *sol, struct line *lin, int *direction);
int solve_handle_zero_squares(struct solution *sol);
int solve_handle_maxnumber_squares(struct solution *sol);
int solve_handle_busy_vertex(struct solution *sol);
int solve_handle_trivial_squares(struct solution *sol);
int solve_handle_trivial_vertex(struct solution *sol);
int solve_handle_loop_bottleneck(struct solution *sol);
void solve_free_solution_data(struct solution *sol);
struct solution* solve_create_solution_data(struct geometry *geo, struct game *game);


#endif
