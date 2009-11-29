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

#ifndef __INCLUDED_GEOMETRY_H__
#define __INCLUDED_GEOMETRY_H__

#include "avl-tree.h"


/* Display states */
enum {
	DISPLAY_NORMAL,
	DISPLAY_ERROR,
	DISPLAY_HANDLED,
	NUM_DISPLAY_STATES
};



/*
 * Holds coordinates of a point
 */
struct point {
	double x, y;
};


/*
 * Coords defining a clip rectangle
 */
struct clipbox {
	double x, y;	// coords of top left corner
	double w, h;	// width, height of box
};


/*
 * Holds info about a vertex
 */
struct vertex {
	int id;			// id of vertex
	struct point pos;	// x,y coordinates of dot
	int nlines;		// number of lines touching dot
	struct line **lines;	// lines touching dot
	int ntiles;		// number of tiles vertex touches
	struct tile **tiles;		// tiles vertex touches
	int display_state;			// display state
};


/*
 * Holds info about a tile
 */
struct tile {
	int id;			// id of tile
	int nvertex;			// number of vertices of 'tile'
	struct vertex **vertex;	// vertices of tile
	int nsides;				// number of sides of tile
	struct line **sides;	// lines around tile
	struct point center;	// coords of center of tile
	int fx_status;		// is it being animated? which animation?
	int fx_frame;		// frame in FX animation
	int display_state;			// display state
};


/*
 * Holds info about a line
 */
struct line {
	int id;			// id of line (line number)
	struct vertex *ends[2];	// coords of ends of line
	int ntiles;				// Number of tiles touching this line (either 1 or 2)
	struct tile *tiles[2];		// tiles at each side of line
	int nin;		// number of lines in
	int nout;		// number of lines out
	struct line **in;	// lines in
	struct line **out;	// lines out
	struct point inf[4];	// coords of 4 points defining area of influence
	struct clipbox clip;	// clip box that contains line
	int fx_status;		// is it being animated? which animation?
	int fx_frame;		// frame in FX animation
};


/*
 * Describes game geometry (how lines, tiles and dots connect to each other)
 */
struct geometry {
	int nvertex;		// Number of dots
	struct vertex *vertex;	// List of dots
	int ntiles;				// Number of tiles
	struct tile *tiles;		// List of tiles
	int nlines;
	struct line *lines;
	double tile_width;
	double tile_height;
	double on_line_width;		// width of ON line
	double off_line_width;		// width of OFF line
	double cross_line_width;	// width of CROSSED line
	double cross_radius;		// cross size
	double font_size;		// font size to use for tile numbers
	double font_scale;		// additional font scale factor for the particular type of tile
	struct point *numpos;		// position hints for numbers
	char *numbers;			// numbers in tiles
	int max_numlines;		// maximum number of lines around a tile
	double board_size;		// size of board
	double board_margin;		// size of margin around game area
	double game_size;		// size of game area (board_size-2*board_margin)
	struct avl_node *vertex_root;	// AVL tree to track vertices
	struct avl_node *line_root;		// AVL tree to track lines
	struct clipbox clip;		// current clip area
};



/* geometry.c */
void geometry_add_tile(struct geometry *geo, struct point *pts, int npts,
					   struct point *center);
void geometry_set_distance_resolution(double distance);
void geometry_connect_skeleton(struct geometry *geo);
struct geometry* geometry_create_new(int ntiles, int nvertex, int nlines,
									 int max_numlines);
void geometry_destroy(struct geometry *geo);
void geometry_set_clip(struct geometry *geo, struct clipbox *clip);
void geometry_update_clip(struct geometry *geo, struct clipbox *clip);


/* geometry-legacy.c */
void geometry_initialize_lines(struct geometry *geo);
void geometry_connect_elements(struct geometry *geo);


#endif
