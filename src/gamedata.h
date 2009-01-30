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
	double board_size;	// Size of gameboard field (game + margin)
	double board_margin;	// margin around actual game area
	double game_size;		// Size of game area (board_size-2*board_margin)
	double width_pxscale;	// Width board-to-pixel scale
	double height_pxscale;	// Height board-to-pixel scale
	struct tile_cache *tile_cache; // tile to line cache	
};


/*
 * Functions
 */
void find_smallest_numbered_square(struct geometry *geo, struct game *game);
struct game* create_empty_gamedata(struct geometry *geo);
void free_gamedata(struct game *game);
void initialize_board(void);


#endif
