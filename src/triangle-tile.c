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
#include "tiles.h"


/* prefered board dimensions for triangle tile */
#define TRIANGULAR_BOARD_SIZE	100.
#define TRIANGULAR_BOARD_MARGIN	5.
#define TRIANGULAR_GAME_SIZE	(TRIANGULAR_BOARD_SIZE - 2*TRIANGULAR_BOARD_MARGIN)



/*
 * Connect sides around triangle
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
triangular_calculate_sizes(struct geometry *geo, int dim)
{
	geo->on_line_width= geo->game_size/dim/15.0;
	geo->off_line_width= geo->board_size/1000.;
	if (geo->on_line_width < 2*geo->off_line_width)
		geo->on_line_width= 2*geo->off_line_width;
	geo->cross_line_width= geo->off_line_width*2;
	geo->cross_radius= MIN(geo->sq_width, geo->sq_height)/15.;
	geo->font_scale= 0.7;
}


/*
 * Build triangular tile geometry data
 * Triangular grid of dim x dim squares
 */
struct geometry*
build_triangular_tile_geometry(const struct gameinfo *info)
{
	struct geometry *geo;
	int i, j, k;
	double ypos;
	double xpos0;
	struct vertex *ver;
	struct square *sq;
	int id;
	int dimx=info->size;
	int dimy=info->size;
	double height;		// y distance between two points
	double offsety;
	double side;
	int pos;

	/* see how many triangles fit wide */
	side= ((double)TRIANGULAR_GAME_SIZE)/(dimx + 1.0/2.0);
	height= side * sqrt(3.0)/2.0;
	offsety= TRIANGULAR_GAME_SIZE - (dimy * height);
	offsety/= 2.0;

	/* dimx until now is number of sides wide,
	   now we turn it into number of triangles*/
	dimx= 2 * dimx;

	/* create new geometry (ntriangles, nvertex, nlines) */
	geo= geometry_create_new(dimx*dimy,			/* ntiles */
							 (dimx/2 + 1)*(dimy + 1), 	/* nvertex */
							 dimx/2*(dimy + 1)
							 + (dimx + 1)*dimy,			/* nlines */
							 3);
	geo->board_size= TRIANGULAR_BOARD_SIZE;
	geo->board_margin= TRIANGULAR_BOARD_MARGIN;
	geo->game_size= TRIANGULAR_GAME_SIZE;

	/* initialize vertices */
	ver= geo->vertex;
	for(j=0; j < dimy + 1; ++j) {
		ypos= height*j + TRIANGULAR_BOARD_MARGIN + offsety;
		xpos0= TRIANGULAR_BOARD_MARGIN;
		if (j % 2 == 0) xpos0+= side / 2.0;
		for(i=0; i < dimx/2 + 1; ++i) {
			ver->id= j*(dimy + 1) + i;
			ver->nlines= 0;		// value will be set in 'join_lines'
			ver->lines= NULL;
			ver->nsquares= 0;
			ver->sq= NULL;
			ver->pos.x= xpos0 + side*i;
			ver->pos.y= ypos;
			++ver;
		}
	}

	/* initialize lines */
	geometry_initialize_lines(geo);

	/* initialize triangles */
	sq= geo->squares;
	id= 0;
	for(j=0; j < dimy; ++j) {
		for(i=0; i < dimx; ++i) {
			sq->id= id;
			/* set vertices */
			sq->nvertex= 3;
			sq->vertex= (struct vertex **)g_malloc(3 * sizeof(void*));
			/* two cases: upright triangle, upside-down triangle */
			pos= j*(dimx/2 + 1) + i/2;		// top (or top left) vertex
			if ((i + j) % 2 == 0) {		/* upright triangle */
				sq->vertex[0]= geo->vertex + pos + (j%2);// top
				sq->vertex[1]= geo->vertex + pos + dimx/2 + 2;	// bot right
				sq->vertex[2]= geo->vertex + pos + dimx/2 + 1;	// bot left
			} else {					/* upside-down triangle */
				sq->vertex[0]= geo->vertex + pos;// top left
				sq->vertex[1]= geo->vertex + pos + 1;	// top right
				sq->vertex[2]= geo->vertex + pos + dimx/2 + 2 - (j%2);	// bottom
			}

			/* calculate position of center of triangle */
			sq->center.x= sq->vertex[0]->pos.x;
			sq->center.y= sq->vertex[0]->pos.y;
			for(k=1; k < sq->nvertex; ++k) {
				sq->center.x+= sq->vertex[k]->pos.x;
				sq->center.y+= sq->vertex[k]->pos.y;
			}
			sq->center.x= sq->center.x/(double)sq->nvertex;
			sq->center.y= sq->center.y/(double)sq->nvertex;

			// set lines on edges of square
			sq->nsides= 3;
			sq->sides= (struct line **)g_malloc(3 * sizeof(void *));
			/* two cases: upright triangle, upside-down triangle */
			if ((i + j) % 2 == 0) {		/* upright triangle */
				sq->sides[0]= geo->lines + dimx/2 + j*(dimx+1+dimx/2)+i+1; // right
				sq->sides[1]= geo->lines + (j+1)*(dimx+1+dimx/2)+i/2;// bot
				sq->sides[2]= geo->lines + dimx/2 + j*(dimx+1+dimx/2)+ i; // left
			} else {					/* upside-down triangle */
				sq->sides[0]= geo->lines + j*(dimx+1+dimx/2)+i/2;//top
				sq->sides[1]= geo->lines + j*(dimx+1+dimx/2)+dimx/2+i+1;//right
				sq->sides[2]= geo->lines + j*(dimx+1+dimx/2)+dimx/2+i;//left
			}

			/* connect lines to square and vertices */
			connect_lines_to_square(sq);

			/* ini FX status */
			sq->fx_status= 0;
			sq->fx_frame= 0;

			++sq;
			++id;
		}
	}

	/* DEBUG: print out details about a few triangles */
	/*g_message("nvertex:%d, nsquares:%d, nlines:%d", geo->nvertex,
			  geo->nsquares, geo->nlines);
	for(i=0; i < 8; ++i) {
		sq= geo->squares + i;
		printf("Triangle: %d (nvertex:%d, nsides:%d)\n", sq->id, sq->nvertex,
			sq->nsides);
		printf("**Vertices:\n");
		for(j=0; j< sq->nvertex; ++j)
			printf("**** %d\n", sq->vertex[j]->id);
		printf("**Sides:\n");
		for(j=0; j< sq->nsides; ++j)
			printf("**** %d\n", sq->sides[j]->id);
			}*/

	/* finalize geometry data: tie everything together */
	geometry_connect_elements(geo);

	/* define sizes of drawing bits */
	triangular_calculate_sizes(geo, dimy);

	return geo;
}
