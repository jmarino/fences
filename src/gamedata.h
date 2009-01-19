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

/* Line states */
#define LINE_OFF		0
#define LINE_ON			1
#define LINE_CROSSED		2

struct point {
	int x, y;
};

struct dot {
	int id;			// id of dot
	struct point pos;	// x,y coordinates of dot
	int nlines;		// number of lines touching dot
	struct line **lines;	// lines touching dot
};

struct square {
	int id;			// id of square
	int number;		// Number inside the square
	int nvertex;		// number of vertices of 'square'
	struct dot **vertex;	// vertices of 'square'
	int nsides;		// number of sides of 'square'
	struct line **sides;	// lines around 'square'
	struct point center;	// coords of center of square
	int fx_status;		// is it being animated? which animation?
	int fx_frame;		// frame in FX animation
};

struct line {
	int id;			// id of line (line number)
	int state;		// State of line
	struct dot *ends[2];	// coords of ends of line
	int nsquares;		// Number of squares touching this line (either 1 or 2)
	struct square *sq[2];	// squares at each side of line
	int nin;		// number of lines in
	int nout;		// number of lines out
	struct line **in;	// lines in
	struct line **out;	// lines out
	struct point inf[4]; 	// coords of 4 points defining area of influence
	struct point inf_box[2];// [x,y] & [w,h] of box that contains line
	int fx_status;		// is it being animated? which animation?
	int fx_frame;		// frame in FX animation
};

struct game {
	int ndots;		// Number of dots
	struct dot *dots;	// List of dots
	int nsquares;		// Number of squares
	struct square *squares;	// List of squares
	int nlines;
	struct line *lines;
	int sq_width;
	int sq_height;
};



struct tile_cache {
	int ntiles_side;	// number of tiles per side
	int ntiles;		// Total num of tiles (ntiles_side^2)
	int tile_size;		// size of boxes that tile gameboard
	GSList **tiles;		// array of lists (board tile -> lines in it)
};

struct board {
	struct game *game;
	int board_size;		// Size of gameboard field (game + margin)
	int board_margin;	// margin around actual game area
	int game_size;		// Size of game area (board_size-2*board_margin)
	double width_pxscale;	// Width board-to-pixel scale
	double height_pxscale;	// Height board-to-pixel scale
	struct tile_cache *tile_cache; // tile to line cache	
};


/*
 * Functions
 */

void initialize_board(void);

#endif
