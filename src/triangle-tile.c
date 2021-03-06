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

#include "geometry.h"
#include "tiles.h"


/* prefered board dimensions for triangle tile */
#define TRIANGULAR_BOARD_SIZE	100.
#define TRIANGULAR_BOARD_MARGIN	5.
#define TRIANGULAR_GAME_SIZE	(TRIANGULAR_BOARD_SIZE - 2*TRIANGULAR_BOARD_MARGIN)




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
	geo->cross_radius= MIN(geo->tile_width, geo->tile_height)/15.;
	geo->font_scale= 0.7;
}


/*
 * Build triangular tile geometry skeleton: no connections between elements
 * Triangular grid of dim x dim tiles
 */
struct geometry*
build_triangular_tile_skeleton(const struct gameinfo *info)
{
	struct geometry *geo;
	int i, j;
	int ntiles;
	int nvertex;
	int nlines;
	double xpos;
	double ypos;
	double xoffset;
	double yoffset;
	int dimx=info->size;
	int dimy=info->size;
	double height;		// y distance between two points
	double side;
	struct point pts[3];

	/* see how many triangles fit wide */
	side= ((double)TRIANGULAR_GAME_SIZE)/(dimx + 1.0/2.0);
	height= side * sqrt(3.0)/2.0;
	yoffset= TRIANGULAR_GAME_SIZE - (dimy * height);
	yoffset= yoffset/2.0 + TRIANGULAR_BOARD_MARGIN;
	xoffset= TRIANGULAR_BOARD_MARGIN + side/2.0;

	/* dimx until now is number of sides wide,
	   now we turn it into number of triangles*/
	dimx= 2 * dimx;

	/* create new geometry (ntriangles, nvertex, nlines) */
	ntiles= dimx*dimy;
	nvertex= (dimx/2 + 1)*(dimy + 1);
	nlines= dimx/2*(dimy + 1) + (dimx + 1)*dimy;
	geo= geometry_create_new(ntiles, nvertex, nlines, 3);
	geo->board_size= TRIANGULAR_BOARD_SIZE;
	geo->board_margin= TRIANGULAR_BOARD_MARGIN;
	geo->game_size= TRIANGULAR_GAME_SIZE;
	geometry_set_distance_resolution(side/10.0);

	/* iterate through triangles creating skeleton geometry
	   (skeleton geometry: lines hold all the topology info) */
	geo->ntiles= 0;
	geo->nlines= 0;
	geo->nvertex= 0;
	for(j=0; j < dimy; ++j) {
		ypos= yoffset + height*j;
		for(i=0; i < dimx; ++i) {
			xpos= xoffset + (side/2.0)*i;
			if ((i+j)%2 == 0) {			/* upright triangle */
				pts[0].x= xpos;					/* top */
				pts[0].y= ypos;
				pts[1].x= xpos + side/2.0;		/* bot right */
				pts[1].y= ypos + height;
				pts[2].x= xpos - side/2.0;		/* bot left */
				pts[2].y= ypos + height;
			} else {					/* upside down triangle */
				pts[0].x= xpos - side/2.0;		/* top left */
				pts[0].y= ypos;
				pts[1].x= xpos + side/2.0;		/* top right */
				pts[1].y= ypos;
				pts[2].x= xpos;					/* bottom */
				pts[2].y= ypos + height;
			}
			/* add tile to skeleton geometry */
			geometry_add_tile(geo, pts, 3, NULL);
		}
	}

	/* sanity check: see if we got the numbers we expected */
	g_assert(geo->ntiles == ntiles);
	g_assert(geo->nvertex == nvertex);
	g_assert(geo->nlines == nlines);

	return geo;
}


/*
 * Build triangular tile geometry data
 * Triangular grid of dim x dim tiles
 */
struct geometry*
build_triangular_tile_geometry(const struct gameinfo *info)
{
	struct geometry *geo;

	/* build geometry skeleton (no connections) */
	geo= build_triangular_tile_skeleton(info);

	/* finalize geometry data: tie everything together */
	geometry_connect_skeleton(geo);

	/* define sizes of drawing bits */
	triangular_calculate_sizes(geo, info->size);

	return geo;
}
