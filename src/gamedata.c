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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>

#include "gamedata.h"
#include "tile_cache.h"


/* holds info about board */
struct board board;


/* Game area size */
//#define FIELD_SIZE		10000


/*
 * Add two dots to end of line (and also add line to the dots)
 */
static void
add_dot_connection(int ndot1, int ndot2, int nline, int nsq)
{
	struct dot *d;
	struct line *lin;
	int i;
	
	// add dots at ends of line (**TODO** think about repeated assignments)
	lin= board.game->lines + nline;
	lin->dots[0]= ndot1;
	lin->dots[1]= ndot2;
	
	// Add square touching line (if needed)
	if (lin->nsquares == 0) {	// no squares assigned yet
		lin->sq[0]= nsq;
		lin->nsquares= 1;
	} else if (lin->nsquares == 1 && lin->sq[0] != nsq) {	
		// one was already assigned -> add if different
		lin->sq[1]= nsq;
		lin->nsquares= 2;
	}
	
	d= board.game->dots + ndot1;
	for(i=0; i < d->ndots && d->dots[i] != ndot2; ++i);
	if (i == d->ndots) d->dots[d->ndots++]= ndot2;
	for(i=0; i < d->nsquares && d->sq[i] != nsq; ++i);
	if (i == d->nsquares) d->sq[d->nsquares++]= nsq;
	for(i=0; i < d->nlines && d->lin[i] != nline; ++i);
	if (i == d->nlines) d->lin[d->nlines++]= nline;
	
	d= board.game->dots + ndot2;
	for(i=0; i < d->ndots && d->dots[i] != ndot1; ++i);
	if (i == d->ndots) d->dots[d->ndots++]= ndot1;
	for(i=0; i < d->nsquares && d->sq[i] != nsq; ++i);
	if (i == d->nsquares) d->sq[d->nsquares++]= nsq;
	for(i=0; i < d->nlines && d->lin[i] != nline; ++i);
	if (i == d->nlines) d->lin[d->nlines++]= nline;
}


/*
 * Measure smallest width and height of a square with a number in it
 * Used to decide what font size to use
 */
static void
measure_square_size(struct game *game)
{
	struct square *sq;
	int i, j;
	int sqw, sqh, tmp;
	
	/* go around all squares to measure smallest w and h */
	game->sq_width= board.board_size;
	game->sq_height= board.board_size;
	sq= game->squares;
	for(i=0; i<game->nsquares; ++i) {
		if (sq->number != -1) {		// square has a number
			sqw= sqh= 0;
			for(j=0; j<4; ++j) {
				tmp= abs(game->dots[sq->dots[j]].x 
					 - game->dots[sq->dots[(j+1)%4]].x);
				if (tmp > sqw) sqw= tmp;
				tmp= abs(game->dots[sq->dots[j]].y 
					 - game->dots[sq->dots[(j+1)%4]].y);
				if (tmp > sqh) sqh= tmp;
			}
			if (sqw < game->sq_width) game->sq_width= sqw;
			if (sqh < game->sq_height) game->sq_height= sqh;
		}
		++sq;
	}
}


/*
 * Define area of influence for each line (4 points)
 */
static void
fill_line_data(struct game *game)
{
	int i;
	struct line *lin;
	struct dot *d1;
	struct square *sq;
	
	lin= game->lines;
	for(i=0; i<game->nlines; ++i) {
		lin->id= i;
		d1= game->dots + lin->dots[0];
		lin->inf[0].x= d1->x;
		lin->inf[0].y= d1->y;
		sq= game->squares + lin->sq[0];
		lin->inf[1].x= sq->center.x;
		lin->inf[1].y= sq->center.y;
		d1= game->dots + lin->dots[1];
		lin->inf[2].x= d1->x;
		lin->inf[2].y= d1->y;
		if (lin->nsquares == 2) {
			sq= game->squares + lin->sq[1];
			lin->inf[3].x= sq->center.x;
			lin->inf[3].y= sq->center.y;
		} else {	// edge line, must manufacture 4th point
			/* make up point across line: 
			 along line joining center of square to center of line */
			lin->inf[3].x= lin->inf[0].x + lin->inf[2].x - lin->inf[1].x;
			lin->inf[3].y= lin->inf[0].y + lin->inf[2].y - lin->inf[1].y;
		}
		/* define box that contains line as [x,y];[w,h]
		 inf[0].xy & inf[2].xy are both ends of the line */
		lin->inf_box[0].x= 
			(lin->inf[0].x < lin->inf[2].x) ? lin->inf[0].x : lin->inf[2].x;
		lin->inf_box[1].x= lin->inf[2].x - lin->inf[0].x;
		if (lin->inf_box[1].x < 0) lin->inf_box[1].x= -lin->inf_box[1].x;
		lin->inf_box[0].y= 
			(lin->inf[0].y < lin->inf[2].y) ? lin->inf[0].y : lin->inf[2].y;
		lin->inf_box[1].y= lin->inf[2].y - lin->inf[0].y;
		if (lin->inf_box[1].y < 0) lin->inf_box[1].y= -lin->inf_box[1].y;
		/* pad inf_box a bit just in case */
		lin->inf_box[0].x-= 200;
		lin->inf_box[0].y-= 200;
		lin->inf_box[1].x+= 400;
		lin->inf_box[1].y+= 400;
		++lin;
	}
}


/*
 * generate a 7x7 example game by hand
 */
void
generate_example_game(struct game *game)
{
	int i, j;
	const int dim=7;		// num of squares per side
	int ypos;
	struct dot *dot;
	struct square *sq;
	struct line *lin;
	int nsq;
	int squaredata[dim*dim]={
		-1,-1,-1,-1,-1,-1,-1,
		-1,-1,-1, 3, 2,-1, 2,
		-1, 3,-1, 1,-1,-1,-1,
		-1, 3, 0,-1, 3,-1,-1,
		-1, 3, 2, 2, 1, 2,-1,
		-1,-1, 2, 2,-1, 1,-1,
		-1,-1,-1, 2, 2, 2,-1};
	
	/* generate a 7x7 squares in a grid */
	game->nsquares= dim*dim;
	game->squares= (struct square*)
		malloc(game->nsquares*sizeof(struct square));
	if (game->squares == NULL) printf("Mem error: squares\n");
	game->ndots= (dim + 1)*(dim + 1);
	game->dots= (struct dot*)
		malloc(game->ndots*sizeof(struct dot));
	if (game->dots == NULL) printf("Mem error: dots\n");
	game->nlines= 2*dim*(dim + 1);
	game->lines= (struct line*)
		malloc(game->nlines*sizeof(struct line));
	if (game->lines == NULL) printf("Mem error: lines\n");

	/* initialize dots */
	dot= game->dots;
	for(j=0; j < dim + 1; ++j) {
		ypos= ((float)board.game_size)/dim*j + board.board_margin;
		for(i=0; i < dim + 1; ++i) {
			dot->id= j*(dim + 1) + i;
			dot->ndots= dot->nsquares= dot->nlines= 0;
			dot->x= ((float)board.game_size)/dim*i + board.board_margin;
			dot->y= ypos;
			++dot;
		}
	}
	
	/* initialize lines */
	lin= game->lines;
	for(i=0; i < game->nlines; ++i) {
		lin->nsquares= 0;
		++lin;
	}
	
	/* initialize squares */
	sq= game->squares;
	nsq= 0;
	for(j=0; j<dim; ++j) {
		for(i=0; i<dim; ++i) {
			sq->id= j*dim + i;
			// set number inside square
			sq->number= squaredata[sq->id];
			
			// set dots on corner of square
			sq->dots[0]= j*(dim+1)+i;	// top left
			sq->dots[1]= j*(dim+1)+i+1;	// top right
			sq->dots[2]= (j+1)*(dim+1)+i+1;	// bot right
			sq->dots[3]= (j+1)*(dim+1)+i;	// bot left
			
			// set lines on edges of square
			sq->lines[0]= j*(dim+dim+1)+i;
			sq->lines[1]= j*(dim+dim+1)+dim+i+1;
			sq->lines[2]= (j+1)*(dim+dim+1)+i;
			sq->lines[3]= j*(dim+dim+1)+dim+i;
			
			// connect dots
			add_dot_connection(sq->dots[0], sq->dots[1], sq->lines[0], nsq);
			add_dot_connection(sq->dots[1], sq->dots[2], sq->lines[1], nsq);
			add_dot_connection(sq->dots[2], sq->dots[3], sq->lines[2], nsq);
			add_dot_connection(sq->dots[3], sq->dots[0], sq->lines[3], nsq);
			
			// calculate position of center of square
			int k;
			sq->center.x= sq->center.y= 0;
			for(k=0; k<4; ++k) {
				dot= game->dots + sq->dots[k];
				sq->center.x+= dot->x;
				sq->center.y+= dot->y;
			}
			sq->center.x/= 4;
			sq->center.y/= 4;
			
			++sq;
			++nsq;
		}
	}

	// debug: print dots asociated to lines
	lin= game->lines;
	for(i=0; i < game->nlines; ++i) {
		//printf("lin %d:  %2d,%2d\n", i, lin->dots[0], lin->dots[1]);
		++lin;
	}

	fill_line_data(game);
	measure_square_size(game);
	
	// test line states
	game->lines[0].state= LINE_ON;
	game->lines[12].state= LINE_ON;
	game->lines[15].state= LINE_CROSSED;
}


/*
 * Initialize board
 */
void
initialize_board(void)
{
	/* Setup coordinate size of board */
	board.board_size= 11000;
	board.board_margin= 500;
	board.game_size= board.board_size - 2*board.board_margin; //10000
	board.tile_cache= NULL;

	/* generate game info */
	board.game= (struct game*) g_malloc(sizeof(struct game));
	/****TODO** check mem error */

	/* make up example game */
	generate_example_game(board.game);

	/* generate tile cache for lines */
	setup_tile_cache();
}
