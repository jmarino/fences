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

#include "gamedata.h"
#include "square-tile.h"
#include "penrose-tile.h"
#include "tile_cache.h"


/* holds info about board */
struct board board;



/*
 * Measure smallest width and height of a square with a number in it
 * Used to decide what font size to use
 */
void
find_smallest_numbered_square(struct geometry *geo, struct game *game)
{
	struct square *sq;
	int i, j, j2;
	double sqw, sqh, tmp;
	
	/* go around all squares to measure smallest w and h */
	geo->sq_width= board.board_size;
	geo->sq_height= board.board_size;
	sq= geo->squares;
	for(i=0; i<geo->nsquares; ++i) {
		if (game->numbers[sq->id] != -1) {		// square has a number
			sqw= sqh= 0.;
			for(j=0; j < sq->nvertex; ++j) {
				j2= (j + 1) % sq->nvertex;
				tmp= fabs(sq->vertex[j]->pos.x - 
					 sq->vertex[j2]->pos.x);
				if (tmp > sqw) sqw= tmp;
				tmp= fabs(sq->vertex[j]->pos.y - 
					 sq->vertex[j2]->pos.y);
				if (tmp > sqh) sqh= tmp;
			}
			if (sqw < geo->sq_width) geo->sq_width= sqw;
			if (sqh < geo->sq_height) geo->sq_height= sqh;
		}
		++sq;
	}
}


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
	game->numbers= (int*)g_malloc(geo->nsquares*sizeof(int));
	for(i=0; i < geo->nlines; ++i)
		game->states[i]= LINE_OFF;
	for(i=0; i < geo->nsquares; ++i)
		game->numbers[i]= -1;
	
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
	g_free(game);
}


/*
 * generate a 7x7 example game by hand
 */
static struct game*
generate_example_game(struct geometry *geo)
{
	int i;
	const int dim=7;		// num of squares per side
	struct game *game;
	int squaredata[dim*dim]={
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
	/* Create empty game data that fits geo */
	game= create_empty_gamedata(geo);
	
	/* set number inside square */
	for(i=0; i < geo->nsquares; ++i) {
		//game->numbers[i]= squaredata[i];
		game->numbers[i]= hard[i];
	}
	
	/* measure smallest square with a number (used to determine font size) */
	find_smallest_numbered_square(geo, game);
	
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
void
initialize_board(void)
{
	/* Setup coordinate size of board */
	board.board_size= 1;//11000;
	board.board_margin= 0.05;//500;
	board.game_size= board.board_size - 2*board.board_margin; //10000
	board.tile_cache= NULL;

	/* make up example game */
	board.geo= build_square_board(7);
	board.game= generate_example_game(board.geo);
	//board.geo= build_square_board(17);
	//board.geo= build_penrose_board();
	//board.game= create_empty_gamedata(board.geo);

	/* generate tile cache for lines */
	setup_tile_cache();
}
