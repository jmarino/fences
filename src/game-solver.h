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
 * Convenience macros
 * They assume that 'struct solution *sol' is defined
 */
#define STATE(lin)			sol->states[(lin)->id]
#define NUMBER(tile)		sol->numbers[(tile)->id]
#define MAX_NUMBER(tile)	sol->numbers[(tile)->id] == ((tile)->nsides - 1)


/* number of solution levels */
#define SOLVE_MAX_LEVEL		8
#define SOLVE_NUM_LEVELS	(SOLVE_MAX_LEVEL + 1)

/* structure to keep track of number of lines ON and CROSS */
struct num_lines {
	guint8 on;
	guint8 cross;
};


/*
 * Contains detail about game solution
 */
struct solution {
	struct geometry *geo;	// geometry of game
	struct game *game;	// game state (mostly useful for tile numbers)
	int *states;		// line states (so we don't touch game->states)
	int *numbers;		// points to game->numbers
	gboolean *tile_done;	// has tile been handled?
	gboolean *vertex_done;	// has vertex been handled?
	int num_tile_done;		// number of tiles marked as done
	int num_vertex_done;	// number of vertices marked as done
	int *lin_mask;
	int nchanges;			// number of line changes in last solution step
	int *changes;			// ID of lines changed in last solution step
	int ntile_changes;		// number of tiles involved in last solution step
	int *tile_changes;		// ID of tiles that caused changes in last step
	int level_count[SOLVE_NUM_LEVELS];	// solution score by level
	gboolean solved;		// is solution complete
	double difficulty;		// difficulty of solution
	int last_level;			// level used in last step of solution
	struct num_lines *tile_count;		// keep track of lines ON & CROSS around tile
	struct num_lines *vertex_count;	// keep track of lines ON & CROSS around vertex
	guint8 *steps;			// keep track of solution steps (levels used)
	int iter;				// number of iterations (solution steps) taken
};


/*
 * Functions
 */
/* solve-tools.c */
gboolean solve_check_valid_game(struct solution *sol);
struct line* goto_next_line(struct line *lin, int *direction, int which);
struct line* follow_line(struct solution *sol, struct line *lin, int *direction);
struct solution* solve_create_solution_data(struct geometry *geo, struct game *game);
void solve_free_solution_data(struct solution *sol);
void solve_copy_solution(struct solution *dest, struct solution *src);
struct solution *solve_duplicate_solution(struct solution *src);
void solve_reset_solution(struct solution *sol);
inline void solve_set_line_on(struct solution *sol, struct line *lin);
inline void solve_set_line_cross(struct solution *sol, struct line *lin);

/* solve-combinations.c */
void solve_try_combinations(struct solution *sol, int level);

/* game-solver.c */
void solve_zero_tiles(struct solution *sol);
void solve_maxnumber_tiles(struct solution *sol);
void solve_maxnumber_incoming_line(struct solution *sol);
void solve_maxnumber_exit_line(struct solution *sol);
void solve_corner(struct solution *sol);
void solve_tiles_net_1(struct solution *sol);
void solve_trivial_tiles(struct solution *sol);
void solve_trivial_vertex(struct solution *sol);
void solve_bottleneck(struct solution *sol);
void solve_cross_lines(struct solution *sol);
void solution_loop(struct solution *sol, int max_iter, int max_level);
struct solution* solve_game(struct geometry *geo, struct game *game, double *score);
void solve_game_solution(struct solution *sol, int max_level);

#endif
