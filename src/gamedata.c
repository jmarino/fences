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
#include <math.h>
#include <string.h>

#include "gamedata.h"
#include "tiles.h"
#include "history.h"


/* holds info about board */
struct board board;


/*
 * Pointers to functions that create all different geometry tiles
 */
typedef struct geometry* (*build_geometry_func_type)(const struct gameinfo *info);
static build_geometry_func_type build_geometry_func[NUMBER_TILE_TYPE]={
	build_square_tile_geometry,
	build_penrose_tile_geometry,
	build_triangular_tile_geometry,
	build_qbert_tile_geometry,
	build_hex_tile_geometry,
	build_snub_tile_geometry,
	build_cairo_tile_geometry,
	build_cartwheel_tile_geometry,
	build_trihex_tile_geometry
};
static build_geometry_func_type build_skeleton_func[NUMBER_TILE_TYPE]={
	build_square_tile_skeleton,
	build_penrose_tile_skeleton,
	build_triangular_tile_skeleton,
	build_qbert_tile_skeleton,
	build_hex_tile_skeleton,
	build_snub_tile_skeleton,
	build_cairo_tile_skeleton,
	build_cartwheel_tile_skeleton,
	build_trihex_tile_skeleton
};



/*
 * Create a new empty game structure that fits geometry 'geo'
 */
struct game*
create_empty_gamedata(struct geometry *geo)
{
	struct game *game;
	int i;

	/* allocate and initialize */
	game= (struct game*)g_malloc(sizeof(struct game));
	game->states= (int*)g_malloc(geo->nlines*sizeof(int));
	game->numbers= (int*)g_malloc(geo->ntiles*sizeof(int));
	game->solution= (int*)g_malloc(geo->nlines*sizeof(int));
	for(i=0; i < geo->nlines; ++i)
		game->states[i]= LINE_OFF;
	for(i=0; i < geo->ntiles; ++i)
		game->numbers[i]= -1;
	game->nlines_on= 0;
	game->solution_nlines_on= 0;

	return game;
}


/*
 * Free game structure
 */
void
free_gamedata(struct game *game)
{
	g_free(game->states);
	g_free(game->numbers);
	g_free(game->solution);
	g_free(game);
}


/*
 * Set line (id) to given state
 */
inline void
game_set_line(int id, int state)
{
	board.game->states[id]= state;
}


/*
 * generate a 7x7 example game by hand
 */
static struct game*
generate_example_game(struct geometry *geo)
{
	int i;
	const int dim=7;		// num of tiles per side
	struct game *game;
	int tiledata[7*7]={
		-1,-1,-1,-1,-1,-1,-1,
		-1,-1,-1, 3, 2,-1, 2,
		-1, 3,-1, 1,-1,-1,-1,
		-1, 3, 0,-1, 3,-1,-1,
		-1, 3, 2, 2, 1, 2,-1,
		-1,-1, 2, 2,-1, 1,-1,
		-1,-1,-1, 2, 2, 2,-1};
	int hard[7*7]={
		 3,-1, 2,-1,-1,-1,-1,
		-1,-1, 2, 1,-1,-1, 3,
		-1,-1, 3,-1, 2,-1,-1,
		 2,-1, 0, 1, 2,-1, 3,
		 3,-1, 0, 2,-1, 1,-1,
		-1,-1, 3,-1,-1, 3,-1,
		-1, 2,-1,-1,-1, 1, 3};
	int hard7[7*7]={		// Really hard game
		-1, 3, 2, 1,-1, 2,-1,
		-1,-1, 2,-1, 1, 3, 2,
		-1,-1, 2,-1, 3, 2,-1,
		-1,-1,-1,-1, 1,-1,-1,
		-1, 3,-1, 3, 2,-1, 3,
		-1, 3, 1,-1,-1, 2,-1,
		 2,-1,-1,-1, 2,-1, 3};
	int hard7_2[7*7]={		// Very hard (slightly less than hard7)
		-1,-1,-1, 3,-1, 1,-1,
		 1, 2,-1, 1, 2,-1,-1,
		 3, 2, 2, 3,-1, 2,-1,
		 2,-1, 3, 2,-1,-1,-1,
		-1,-1,-1,-1, 2,-1, 2,
		-1,-1, 3, 2,-1, 3,-1,
		-1,-1,-1, 2,-1, 2, 2};
	int hard7_3[7*7]={		// Very hard (level 6)
		 3,-1,-1,-1, 1,-1,-1,
		-1, 2, 2, 3,-1,-1, 2,
		 2,-1,-1,-1,-1, 2,-1,
		 1,-1,-1,-1,-1, 2, 2,
		 2,-1, 2,-1, 2,-1, 3,
		-1, 3,-1, 2,-1, 3, 2,
		-1, 2, 3,-1, 2,-1, 2};
	int hard7_4[7*7]={		// Very hard (level 6)
		 3,-1,-1, 2,-1, 2, 3,
		-1, 2,-1, 1,-1,-1,-1,
		 3,-1, 2, 1, 2, 2, 0,
		-1,-1,-1,-1,-1, 3,-1,
		 2, 2,-1, 2,-1, 0,-1,
		-1, 2, 1, 2, 3,-1, 3,
		 3,-1,-1, 2,-1,-1,-1};
	int hard7_5[7*7]={		// Hard (level 5)
		-1,-1,-1,-1,-1,-1,-1,
		 3,-1,-1, 2,-1,-1, 2,
		-1, 2,-1, 2,-1,-1, 2,
		 2, 3, 3,-1, 3,-1, 2,
		-1, 1, 2,-1,-1, 2, 2,
		-1,-1,-1, 1, 3,-1, 2,
		 3,-1, 3, 2,-1,-1,-1};
	int wiki[6*6]={
		-1,-1,-1,-1,0,-1,
		 3, 3,-1,-1, 1,-1,
		-1,-1, 1, 2,-1,-1,
		-1,-1, 2, 0,-1,-1,
		-1, 1,-1,-1, 1, 1,
		-1, 2,-1,-1,-1,-1};
	int easy15[15*15]={
		-1,-1,-1,-1,-1, 2,-1,-1, 3,-1, 3,-1,-1,-1, 3,
		 2, 1,-1, 2, 3, 2,-1,-1, 3, 1, 2,-1, 1,-1, 2,
		 3,-1,-1,-1, 2,-1, 1,-1, 2, 2, 1, 1, 3,-1, 3,
		 1,-1,-1, 3, 0,-1, 3, 3, 1,-1,-1,-1, 2, 1, 3,
		 2, 3, 2, 3,-1,-1, 2,-1,-1, 1,-1, 3, 2,-1,-1,
		 2, 1, 2, 2,-1,-1, 2,-1,-1, 2,-1,-1, 0,-1,-1,
		 2,-1, 3,-1,-1,-1, 1,-1, 3, 3, 2,-1,-1,-1,-1,
		 3,-1,-1,-1, 3, 2, 3,-1,-1,-1, 2,-1,-1, 1,-1,
		-1,-1,-1,-1,-1,-1, 2, 1,-1,-1, 3,-1,-1,-1, 1,
		-1, 2, 1,-1,-1,-1, 1,-1,-1,-1, 3,-1, 2, 2,-1,
		-1, 2,-1, 3, 2, 3, 3, 2, 3,-1, 3,-1, 2, 1, 3,
		 3, 1, 2,-1,-1,-1, 1,-1, 3, 1,-1, 2, 3,-1, 3,
		-1, 2,-1,-1, 3, 2, 2, 1,-1, 2, 3,-1,-1, 1, 2,
		 1,-1,-1,-1,-1,-1, 2,-1,-1,-1,-1,-1,-1,-1, 2,
		-1,-1,-1,-1,-1,-1, 3, 3, 2,-1, 3,-1, 1,-1, 2};
	int hard15[15*15]={
		 3,-1,-1,-1, 1,-1,-1,-1,-1,-1, 2,-1, 3,-1,-1,
		-1, 2, 1, 3,-1, 2,-1,-1,-1, 1, 1,-1,-1, 1, 3,
		 3,-1,-1,-1, 2, 2, 2, 1,-1, 1, 1,-1,-1, 3, 1,
		-1,-1,-1,-1,-1,-1,-1,-1, 2, 2,-1,-1,-1,-1, 2,
		-1, 3,-1, 2, 1,-1,-1,-1, 2, 2,-1, 2,-1, 1,-1,
		-1,-1, 3, 3, 2,-1,-1, 3,-1, 2,-1, 1,-1, 1, 3,
		-1,-1,-1, 1,-1,-1,-1,-1, 1,-1,-1, 2,-1, 1, 2,
		-1,-1, 2,-1,-1, 3, 2, 1,-1, 2, 1, 2,-1,-1,-1,
		-1, 3,-1, 3,-1,-1,-1,-1, 3,-1,-1,-1,-1, 3,-1,
		-1, 1,-1,-1, 2,-1, 2,-1,-1,-1,-1,-1, 1,-1, 2,
		-1, 2,-1, 1, 2, 1,-1,-1,-1,-1, 3,-1, 2,-1,-1,
		-1, 1,-1,-1,-1, 3, 3,-1, 3,-1, 2, 1, 2, 3, 1,
		-1,-1,-1, 2,-1,-1,-1,-1,-1, 3,-1,-1, 1,-1, 3,
		 2, 3,-1, 3, 1,-1,-1, 1,-1, 1,-1,-1,-1,-1, 2,
		-1,-1, 1,-1, 3, 1,-1,-1,-1, 2,-1, 1,-1,-1,-1};


	/* Create empty game data that fits geo */
	game= create_empty_gamedata(geo);

	/* set number inside tile */
	for(i=0; i < geo->ntiles; ++i) {
		//game->numbers[i]= tiledata[i];
		game->numbers[i]= hard7[i];
		//game->numbers[i]= hard[i];
		//game->numbers[i]= easy15[i];
		//game->numbers[i]= hard15[i];
		//game->numbers[i]= wiki[i];
	}

	// test line states
	game->states[0]= LINE_ON;
	game->states[12]= LINE_ON;
	game->states[15]= LINE_CROSSED;

	/* artificial test for FX animation */
	/*for(i=0; i < 7; ++i) {
		game->lines[i].state= LINE_ON;
		game->lines[i].fx_status= 1;
		game->lines[i].fx_frame= i;
	}*/

	return game;
}


/*
 * Initialize board
 */
struct board *
initialize_board(void)
{
	/* Setup coordinate size of board */
	//board.board_size= 1;//11000;
	//board.board_margin= 0.05;//500;
	//board.game_size= board.board_size - 2*board.board_margin; //10000

	board.gameinfo.type= TILE_TYPE_PENROSE;
	//board.gameinfo.type= TILE_TYPE_SQUARE;
	board.gameinfo.size= 2;
	//board.gameinfo.size= 7;
	board.gameinfo.diff_index= 3;

	board.click_mesh= NULL;
	board.history= history_create();
	board.drawarea= NULL;
	board.window= NULL;
	board.game_state= GAMESTATE_NOGAME;

	/* build geometry data from gameinfo */
	board.geo= build_geometry_tile(&board.gameinfo);

	/* generate click mesh for lines */
	board.click_mesh= click_mesh_setup(board.geo);

	/* empty gamedata */
	board.game= create_empty_gamedata(board.geo);
	//board.game= generate_example_game(board.geo);
	//printf("nlines: %d\nntiles: %d\n", board.geo->nlines, board.geo->ntiles);

	return &board;
}


/*
 * Clear game
 */
void
gamedata_clear_game(struct board *board)
{
	/* clear line states */
	memset(board->game->states, 0, board->geo->nlines*sizeof(int));
	/* clear history */
	history_clear(board->history);
	board->game_state= GAMESTATE_NEW;
}


/*
 * Build new geometry of type determined by gameinfo
 */
struct geometry *
build_geometry_tile(struct gameinfo *gameinfo)
{
	g_assert(gameinfo->type >= 0 &&  gameinfo->type < NUMBER_TILE_TYPE);
	return build_geometry_func[gameinfo->type](gameinfo);
}


/*
 * Build tile skeleton of type determined by gameinfo
 */
struct geometry *
build_tile_skeleton(struct gameinfo *gameinfo)
{
	g_assert(gameinfo->type >= 0 &&  gameinfo->type < NUMBER_TILE_TYPE);
	return build_geometry_func[gameinfo->type](gameinfo);
}


/*
 * Destroy current game
 */
void
gamedata_destroy_current_game(struct board *board)
{
	geometry_destroy(board->geo);
	board->geo= NULL;
	free_gamedata(board->game);
	board->game= NULL;
	click_mesh_destroy(board->click_mesh);
	board->click_mesh= NULL;
	history_clear(board->history);
	board->game_state= GAMESTATE_NOGAME;
}


/*
 * Create new game according to given gameinfo
 */
void
gamedata_create_new_game(struct board *board, struct gameinfo *info)
{
	memcpy(&board->gameinfo, info, sizeof(struct gameinfo));

	/* build geometry data from gameinfo */
	board->geo= build_geometry_tile(info);

	/* generate click mesh for lines */
	board->click_mesh= click_mesh_setup(board->geo);

	/* build new game */
	board->game= build_new_game(board->geo, 4.0);
}
