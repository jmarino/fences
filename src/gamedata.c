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

#include "gamedata.h"

struct game gamedata;	/* holds current game data */


/* Game area size */
#define FIELD_SIZE		10000


static void
add_dot_connection(int ndot1, int ndot2, int nline, int nsq)
{
	struct dot *d;
	int i;
	
	gamedata.lines[nline].dots[0]= ndot1;
	gamedata.lines[nline].dots[1]= ndot2;
	
	d= gamedata.dots + ndot1;
	for(i=0; i < d->ndots && d->dots[i] != ndot2; ++i);
	if (i == d->ndots) d->dots[d->ndots++]= ndot2;
	for(i=0; i < d->nsquares && d->sq[i] != nsq; ++i);
	if (i == d->nsquares) d->sq[d->nsquares++]= nsq;
	for(i=0; i < d->nlines && d->lin[i] != nline; ++i);
	if (i == d->nlines) d->lin[d->nlines++]= nline;
	
	d= gamedata.dots + ndot2;
	for(i=0; i < d->ndots && d->dots[i] != ndot1; ++i);
	if (i == d->ndots) d->dots[d->ndots++]= ndot1;
	for(i=0; i < d->nsquares && d->sq[i] != nsq; ++i);
	if (i == d->nsquares) d->sq[d->nsquares++]= nsq;
	for(i=0; i < d->nlines && d->lin[i] != nline; ++i);
	if (i == d->nlines) d->lin[d->nlines++]= nline;
}


static void
measure_square_size(void)
{
	struct square *sq;
	int i, j;
	int sqw, sqh, tmp;
	
	/* go around all squares to measure smallest w and h */
	gamedata.sq_width= FIELD_SIZE;
	gamedata.sq_height= FIELD_SIZE;
	sq= gamedata.squares;
	for(i=0; i<gamedata.nsquares; ++i) {
		if (sq->number != -1) {		// square has a number
			sqw= sqh= 0;
			for(j=0; j<4; ++j) {
				tmp= abs(gamedata.dots[sq->dots[j]].x 
						 - gamedata.dots[sq->dots[(j+1)%4]].x);
				if (tmp > sqw) sqw= tmp;
				tmp= abs(gamedata.dots[sq->dots[j]].y 
						 - gamedata.dots[sq->dots[(j+1)%4]].y);
				if (tmp > sqh) sqh= tmp;
			}
			if (sqw < gamedata.sq_width) gamedata.sq_width= sqw;
			if (sqh < gamedata.sq_height) gamedata.sq_height= sqh;
		}
		++sq;
	}
}

/*
 * generate a 7x7 example game by hand
 */
void
generate_example_game()
{
	int i, j;
	const int dim=7;		// num of squares per side
	int ypos;
	struct dot *dot;
	struct square *sq;
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
	gamedata.nsquares= dim*dim;
	gamedata.squares= (struct square*)
		malloc(gamedata.nsquares*sizeof(struct square));
	if (gamedata.squares == NULL) printf("Mem error: squares\n");
	gamedata.ndots= (dim + 1)*(dim + 1);
	gamedata.dots= (struct dot*)
		malloc(gamedata.ndots*sizeof(struct dot));
	if (gamedata.dots == NULL) printf("Mem error: dots\n");
	gamedata.nlines= 2*dim*(dim + 1);
	gamedata.lines= (struct line*)
		malloc(gamedata.nlines*sizeof(struct line));
	if (gamedata.lines == NULL) printf("Mem error: lines\n");

	/* initialize dots */
	dot= gamedata.dots;
	for(j=0; j < dim + 1; ++j) {
		ypos= ((float)FIELD_SIZE)/dim*j;
		for(i=0; i < dim + 1; ++i) {
			dot->ndots= dot->nsquares= dot->nlines= 0;
			dot->x= ((float)FIELD_SIZE)/dim*i;
			dot->y= ypos;
			++dot;
		}
	}
	
	/* initialize squares */
	sq= gamedata.squares;
	nsq= 0;
	for(j=0; j<dim; ++j) {
		for(i=0; i<dim; ++i) {
			// set number inside square
			sq->number= squaredata[j*dim+i];
			
			// set dots on corner of square
			sq->dots[0]= j*(dim+1)+i;			// top left
			sq->dots[1]= j*(dim+1)+i+1;			// top right
			sq->dots[2]= (j+1)*(dim+1)+i+1;		// bot right
			sq->dots[3]= (j+1)*(dim+1)+i;		// bot left
			
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
			
			++sq;
			++nsq;
		}
	}

	measure_square_size();
	gamedata.lines[0].state= LINE_ON;
	gamedata.lines[12].state= LINE_ON;
	gamedata.lines[15].state= LINE_CROSSED;
}
