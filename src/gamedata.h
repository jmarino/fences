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
#include "tiles.h"


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
 * Holds game data (tile numbers and lines that are on)
 */
struct game {
	int *states;		// Line states
	int *numbers;		// Number in tiles
	int nlines_on;		// Number of lines currently on
	int *solution;		// Solved game
	int solution_nlines_on;	// Number of lines on in solution
};


/*
 * contains info about a line change
 */
struct line_change {
	int id;			// id of line that changes
	int old_state;		// old state of line
	int new_state;		// new state of line
};


/* stores history data (private declaration, see history.c) */
struct history;


/*
 * Click mesh: listing lines found inside each mesh tile
 */
struct click_mesh {
	int ntiles_side;	// number of mesh tiles per side
	int ntiles;		// Total num of mesh tiles (nmesh_side^2)
	double tile_size;	// size of boxes that tile gameboard
	GSList **tiles;		// array of lists (mesh tile -> lines in it)
};

struct board {
	struct gameinfo gameinfo;		// info about game (tile type, size, ...)
	struct geometry *geo;	// geometry info of lines, tiles & vertices
	struct game *game;	// game data (line states and tile numbers)
	double width_pxscale;	// Width board-to-pixel scale
	double height_pxscale;	// Height board-to-pixel scale
	struct click_mesh *click_mesh; // click mesh
	struct history *history;		// history data
	gpointer drawarea;	// widget where board is drawn
	gpointer window;	// main gtk window
	int game_state;		// hold current state of game
};


/* gamedata.c */
struct game* create_empty_gamedata(struct geometry *geo);
void free_gamedata(struct game *game);
void game_set_line(int id, int state);
struct board* initialize_board(void);
void gamedata_clear_game(struct board *board);
struct geometry *build_board_geometry(struct gameinfo *gameinfo);
void gamedata_destroy_current_game(struct board *board);
void gamedata_create_new_game(struct board *board, struct gameinfo *info);
struct geometry *build_geometry_tile(struct gameinfo *gameinfo);
struct geometry *build_tile_skeleton(struct gameinfo *gameinfo);

/* click-mesh.c */
void click_mesh_destroy(struct click_mesh *click_mesh);
struct click_mesh* click_mesh_setup(const struct geometry *geo);

/* mesh-tools.c */
gboolean is_area_inside_box(struct point *area, struct point *box);
gboolean is_point_inside_area(struct point *point, struct point *area);

/* build-game.c */
struct game* build_new_game(struct geometry *geo, double difficulty);

/* build-loop.c */
void build_new_loop(struct geometry *geo, struct game *game, gboolean trace);


#endif
