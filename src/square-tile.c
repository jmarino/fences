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

#include "geometry.h"


/* prefered board dimensions for square tile */
#define SQUARE_BOARD_SIZE	100.
#define SQUARE_BOARD_MARGIN	5.
#define SQUARE_GAME_SIZE	(SQUARE_BOARD_SIZE - 2*SQUARE_BOARD_MARGIN)



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
 * Calculate sizes of drawing details
 */
static void
square_calculate_sizes(struct geometry *geo, int dim)
{
	geo->on_line_width= geo->game_size/dim/15.0;
	geo->off_line_width= geo->board_size/1000.;
	if (geo->on_line_width < 2*geo->off_line_width) 
		geo->on_line_width= 2*geo->off_line_width;
	geo->font_scale= 1.;
}


/*
 * Build square tile geometry data
 * Square grid of dim x dim dimensions
 */
struct geometry*
build_square_board(const int dim)
{
	struct geometry *geo;
	int i, j, k;
	double ypos;
	struct vertex *ver;
	struct square *sq;
	int id;
	
	/* create new geometry (nsquares, nvertex, nlines) */
	geo= geometry_create_new(dim*dim, (dim + 1)*(dim + 1), 2*dim*(dim + 1), 4);
	geo->board_size= SQUARE_BOARD_SIZE;
	geo->board_margin= SQUARE_BOARD_MARGIN;
	geo->game_size= SQUARE_GAME_SIZE;
	
	/* initialize vertices */
	ver= geo->vertex;
	for(j=0; j < dim + 1; ++j) {
		ypos= ((double)SQUARE_GAME_SIZE)/dim*j + SQUARE_BOARD_MARGIN;
		for(i=0; i < dim + 1; ++i) {
			ver->id= j*(dim + 1) + i;
			ver->nlines= 0;		// value will be set in 'join_lines'
			ver->lines= NULL;
			ver->nsquares= 0;
			ver->sq= NULL;
			ver->pos.x= ((double)SQUARE_GAME_SIZE)/dim*i + SQUARE_BOARD_MARGIN;
			ver->pos.y= ypos;
			++ver;
		}
	}
	
	/* initialize lines */
	geometry_initialize_lines(geo);
	
	/* initialize squares */
	sq= geo->squares;
	id= 0;
	for(j=0; j<dim; ++j) {
		for(i=0; i<dim; ++i) {
			sq->id= id;
			/* set vertices */
			sq->nvertex= 4;
			sq->vertex= (struct vertex **)g_malloc(4 * sizeof(void*));
			sq->vertex[0]= geo->vertex + j*(dim+1)+i;	// top left
			sq->vertex[1]= geo->vertex + j*(dim+1)+i+1;	// top right
			sq->vertex[2]= geo->vertex + (j+1)*(dim+1)+i+1;	// bot right
			sq->vertex[3]= geo->vertex + (j+1)*(dim+1)+i;	// bot left
			
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
			sq->sides[0]= geo->lines + j*(dim+dim+1)+i;
			sq->sides[1]= geo->lines + j*(dim+dim+1)+dim+i+1;
			sq->sides[2]= geo->lines + (j+1)*(dim+dim+1)+i;
			sq->sides[3]= geo->lines + j*(dim+dim+1)+dim+i;
			
			/* connect lines to square and vertices */
			connect_lines_to_square(sq);
			
			/* ini FX status */
			sq->fx_status= 0;
			sq->fx_frame= 0;
			
			++sq;
			++id;
		}
	}

	/* finalize geometry data: tie everything together */
	geometry_connect_elements(geo);
	
	/* define sizes of drawing bits */
	square_calculate_sizes(geo, dim);
	
	return geo;
}
