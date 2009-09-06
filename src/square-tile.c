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


/* prefered board dimensions for square tile */
#define SQUARE_BOARD_SIZE	100.
#define SQUARE_BOARD_MARGIN	5.
#define SQUARE_GAME_SIZE	(SQUARE_BOARD_SIZE - 2*SQUARE_BOARD_MARGIN)



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
	geo->cross_line_width= geo->off_line_width*2;
	geo->cross_radius= MIN(geo->tile_width, geo->tile_height)/15.;
	geo->font_scale= 1.;
}


/*
 * Build square tile geometry data
 * Square grid of dim x dim dimensions
 */
struct geometry*
build_square_tile_geometry(const struct gameinfo *info)
{
	struct geometry *geo;
	int i, j;
	double xpos;
	double ypos;
	int dim=info->size;		/** HACK ** FIXME: allow rectangular games */
	double side;
	int ntiles;
	int nvertex;
	int nlines;
	struct point pts[4];

	/* calculate length of tile side */
	side= ((double)SQUARE_GAME_SIZE)/dim;

	/* create new geometry (ntiles, nvertex, nlines) */
	ntiles= dim*dim;
	nvertex= (dim + 1)*(dim + 1);
	nlines= 2*dim*(dim + 1);
	geo= geometry_create_new(ntiles, nvertex, nlines, 4);
	geo->board_size= SQUARE_BOARD_SIZE;
	geo->board_margin= SQUARE_BOARD_MARGIN;
	geo->game_size= SQUARE_GAME_SIZE;
	geometry_set_distance_resolution(side/10.0);

	/* iterate through tiles creating skeleton geometry
	   (skeleton geometry: lines hold all the topology info) */
	geo->ntiles= 0;
	geo->nlines= 0;
	geo->nvertex= 0;
	for(j=0; j < dim; ++j) {
		ypos= SQUARE_BOARD_MARGIN + side*j;
		for(i=0; i < dim; ++i) {
			xpos= SQUARE_BOARD_MARGIN + side*i;
			/* xpos,ypos -> coord of top left vertex */
			pts[0].x= xpos;					/* top left*/
			pts[0].y= ypos;
			pts[1].x= xpos + side;			/* top right */
			pts[1].y= ypos;
			pts[2].x= xpos + side;			/* bot right */
			pts[2].y= ypos + side;
			pts[3].x= xpos;					/* bot left */
			pts[3].y= ypos + side;

			/* add tile to skeleton geometry */
			geometry_add_tile(geo, pts, 4, NULL);
		}
	}

	/* sanity check: see if we got the numbers we expected */
	g_assert(geo->ntiles == ntiles);
	g_assert(geo->nvertex == nvertex);
	g_assert(geo->nlines == nlines);

	/* finalize geometry data: tie everything together */
	geometry_connect_skeleton(geo);

	/* define sizes of drawing bits */
	square_calculate_sizes(geo, dim);

	return geo;
}
