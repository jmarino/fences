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


/* defined in gamedata.c */
extern struct board board;

/*
 * Connect sides of square
 */
static void
connect_lines_to_square(struct square *sq)
{
	int i, i2;
	struct line *lin;
	
	/* iterate over all sides of square */
	for(i=0; i < sq->nsides; ++i) {
		i2= (i+1) % sq->nsides;
		lin= sq->sides[i];
		
		/* multiple assignations may happen (squares sharing a line) */
		lin->ends[0]= sq->vertex[i];
		lin->ends[1]= sq->vertex[i2];
		
		/* add current square to list of squares touching line */
		if (lin->nsquares == 0) {	// no squares assigned yet
			lin->sq[0]= sq;
			lin->nsquares= 1;
		} else if (lin->nsquares == 1 && lin->sq[0] != sq) {	
			// one was already assigned -> add if different
			lin->sq[1]= sq;
			lin->nsquares= 2;
		}
	}
}


/*
 * Build square tile game data
 * Square grid of dim x dim dimensions
 */
struct game*
build_square_board(const int dim)
{
	struct game *game;
	int i, j, k;
	double ypos;
	struct dot *dot;
	struct square *sq;
	struct line *lin;
	int id;
	
	/* generate a 7x7 squares in a grid */
	game= (struct game*) g_malloc(sizeof(struct game));
	game->nsquares= dim*dim;
	game->squares= (struct square*)
		g_malloc(game->nsquares*sizeof(struct square));
	game->ndots= (dim + 1)*(dim + 1);
	game->dots= (struct dot*)
		g_malloc(game->ndots*sizeof(struct dot));
	game->nlines= 2*dim*(dim + 1);
	game->lines= (struct line*)
		g_malloc(game->nlines*sizeof(struct line));

	/* initialize dots */
	dot= game->dots;
	for(j=0; j < dim + 1; ++j) {
		ypos= ((double)board.game_size)/dim*j + board.board_margin;
		for(i=0; i < dim + 1; ++i) {
			dot->id= j*(dim + 1) + i;
			dot->nlines= 0;		// value will be set in 'join_lines'
			dot->lines= NULL;
			dot->pos.x= ((double)board.game_size)/dim*i + board.board_margin;
			dot->pos.y= ypos;
			++dot;
		}
	}
	
	/* initialize lines */
	lin= game->lines;
	for (id= 0; id < game->nlines; ++id) {
		lin->id= id;
		lin->nsquares= 0;
		lin->fx_status= 0;
		lin->fx_frame= 0;
		++lin;
	}
	
	/* initialize squares */
	sq= game->squares;
	id= 0;
	for(j=0; j<dim; ++j) {
		for(i=0; i<dim; ++i) {
			sq->id= id;
			/* set number inside square */
			sq->number= -1;
			
			/* set vertices */
			sq->nvertex= 4;
			sq->vertex= (struct dot **)g_malloc(4 * sizeof(void*));
			sq->vertex[0]= game->dots + j*(dim+1)+i;	// top left
			sq->vertex[1]= game->dots + j*(dim+1)+i+1;	// top right
			sq->vertex[2]= game->dots + (j+1)*(dim+1)+i+1;	// bot right
			sq->vertex[3]= game->dots + (j+1)*(dim+1)+i;	// bot left
			
			/* calculate position of center of square */
			sq->center.x= sq->vertex[0]->pos.x;
			sq->center.y= sq->vertex[0]->pos.y;
			for(k=1; k < sq->nvertex; ++k) {
				sq->center.x+= sq->vertex[k]->pos.x;
				sq->center.y+= sq->vertex[k]->pos.y;
			}
			sq->center.x= sq->center.x/(double)sq->nvertex;
			sq->center.y= sq->center.y/(double)sq->nvertex;
			
			// set lines on edges of square
			sq->nsides= 4;
			sq->sides= (struct line **)g_malloc(4 * sizeof(void *));
			sq->sides[0]= game->lines + j*(dim+dim+1)+i;
			sq->sides[1]= game->lines + j*(dim+dim+1)+dim+i+1;
			sq->sides[2]= game->lines + (j+1)*(dim+dim+1)+i;
			sq->sides[3]= game->lines + j*(dim+dim+1)+dim+i;
			
			/* connect lines to square and vertices */
			connect_lines_to_square(sq);
			
			/* ini FX status */
			sq->fx_status= 0;
			sq->fx_frame= 0;
			
			++sq;
			++id;
		}
	}
	
	/* interconnect all the lines */
	build_line_network(game);
	
	/* define area of influence of each line */
	define_line_infarea(game);

	return game;
}


/*
 * generate a 7x7 example game by hand
 */
struct game*
generate_example_game(void)
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
	
	/* create empty square board */
	game= build_square_board(dim);
	
	/* set number inside square */
	for(i=0; i < game->nsquares; ++i) {
		game->squares[i].number= squaredata[i];
	}
	
	/* measure smallest square with a number (used to determine font size) */
	find_smallest_numbered_square(game);
	
	// test line states
	game->lines[0].state= LINE_ON;
	game->lines[12].state= LINE_ON;
	game->lines[15].state= LINE_CROSSED;
	game->lines[0].fx_status= 1;
	
	/* artificial test for FX animation */
	for(i=0; i < 7; ++i) {
		game->lines[i].state= LINE_ON;
		game->lines[i].fx_status= 1;
		game->lines[i].fx_frame= i;
	}
	
	return game;
}
