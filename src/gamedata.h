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

#ifndef __INCLUDED_GAMEDATA_H__
#define __INCLUDED_GAMEDATA_H__

#include "geometry.h"


/* Line states */
#define LINE_OFF		0
#define LINE_ON			1
#define LINE_CROSSED		2


/* Possible connecting directions for a line */
#define DIRECTION_IN	0
#define DIRECTION_OUT	1


/* Possible game states */
enum {
	GAMESTATE_NEW,			/* new game: no moves yet*/
	GAMESTATE_ONGOING,		/* game in progress */
	GAMESTATE_FINISHED,		/* game finished */
	GAMESTATE_NOGAME		/* board empty, no game going on */
};


/*
 * Holds game data (square numbers and lines that are on)
 */
struct game {
	int *states;		// Line states
	int *numbers;		// Number in squares
};


/*
 * Cache listing lines found inside each cache tile
 */
struct tile_cache {
	int ntiles_side;	// number of tiles per side
	int ntiles;		// Total num of tiles (ntiles_side^2)
	double tile_size;	// size of boxes that tile gameboard
	GSList **tiles;		// array of lists (board tile -> lines in it)
};

struct board {
	struct geometry *geo;	// geometry info of lines, squares & vertices
	struct game *game;	// game data (line states and square numbers)
	double width_pxscale;	// Width board-to-pixel scale
	double height_pxscale;	// Height board-to-pixel scale
	struct tile_cache *tile_cache; // tile to line cache
	GList *history;
	gpointer drawarea;	// widget where board is drawn
	gpointer window;	// main gtk window
	int game_state;		// hold current state of game
};


/*
 * Functions
 */
void find_smallest_numbered_square(struct geometry *geo, struct game *game);
struct game* create_empty_gamedata(struct geometry *geo);
void free_gamedata(struct game *game);
void game_set_line(int id, int state);
void initialize_board(void);
void gamedata_clear_game(struct board *board);


#endif
