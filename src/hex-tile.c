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


/* prefered board dimensions for penrose tile */
#define HEX_BOARD_SIZE	100.
#define HEX_BOARD_MARGIN	5.
#define HEX_GAME_SIZE	(HEX_BOARD_SIZE - 2*HEX_BOARD_MARGIN)



/*
 * Calculate sizes of drawing details
 */
static void
hexagonal_calculate_sizes(struct geometry *geo, int dim)
{
	geo->on_line_width= geo->game_size/dim/15.0;
	geo->off_line_width= geo->board_size/1000.;
	if (geo->on_line_width < 2*geo->off_line_width)
		geo->on_line_width= 2*geo->off_line_width;
	geo->cross_line_width= geo->off_line_width*2;
	geo->cross_radius= geo->sq_width/10.0;
	geo->font_scale= 1.5;
}


/*
 * Build Qbert (quasiregular rhombic) tile geometry data
 */
struct geometry*
build_hex_tile_geometry(const struct gameinfo *info)
{
	struct geometry *geo;
	int i, j;
	int dimx;
	int dimy=info->size;
	int extra;
	double num_x;
	double num_y;
	double height;
	double side;
	int ntiles;
	int nvertex;
	int nlines;
	double x0;
	double y0;
	double yoffset;
	struct point pos;
	struct point pts[6];

	/* estimate how many 'sides' wide we have */
	num_x= (info->size/2)*3;
	if (info->size % 2 == 1) num_x+= 2;
	else num_x+= 1.0/2.0;
	/* how many 'sides' tall */
	num_y= sqrt(3.0) * info->size;						/* number of sides tall */
	side= HEX_GAME_SIZE/((double)num_y);
	height= sqrt(3.0)*side;

	/* see if we can fit an extra row (we need extra 1.5 'sides') */
	extra= (int)floor((num_y - num_x)/1.5);
	num_x+= extra * 1.5;

	/* number of units that fit wide and tall */
	dimx= info->size + extra;
	dimy= info->size;

	printf("num:%lfx%lf\n", num_x, num_y);
	printf("dim:%dx%d\n", dimx, dimy);
	printf("side: %lf\n", side);

	/* coordinates of unit on top left (center of unit coords) */
	x0= HEX_BOARD_MARGIN + (HEX_GAME_SIZE - num_x*side) / 2.0;
	y0= HEX_BOARD_MARGIN + (HEX_GAME_SIZE - num_y*side) / 2.0;

	/* create geometry with plenty of space for lines and vertex */
	ntiles= dimx * (dimy - 1) + dimx/2;
	nvertex= (int)(ntiles*3.5);		/* found by trial-and-error (size 5>) */
	nlines= (int)(ntiles*4.0);		/* found by trial-and-error (size 5>) */
	geo= geometry_create_new(ntiles, nvertex, nlines, 6);
	geo->board_size= HEX_BOARD_SIZE;
	geo->board_margin= HEX_BOARD_MARGIN;
	geo->game_size= HEX_BOARD_SIZE - 2*HEX_BOARD_MARGIN;
	geometry_set_distance_resolution(side/10.0);

	/* create hexagons */
	geo->nsquares= 0;
	geo->nvertex= 0;
	geo->nlines= 0;
	for(i=0; i < dimx; ++i) {
		pos.x= x0 + i*(side + side/2.0);
		yoffset= (i % 2) * height/2.0;
		for(j=0; j < dimy; ++j) {
			if (j == 0 && (i % 2) == 0) continue;
			pos.y= y0 + yoffset + j * height;
			pts[0].x= pos.x;
			pts[0].y= pos.y;
			pts[1].x= pos.x + side/2.0;
			pts[1].y= pos.y - height / 2.0;
			pts[2].x= pos.x + side*3.0/2.0;
			pts[2].y= pos.y - height / 2.0;
			pts[3].x= pos.x + 2.0*side;
			pts[3].y= pos.y;
			pts[4].x= pos.x + side*3.0/2.0;
			pts[4].y= pos.y + height / 2.0;
			pts[5].x= pos.x + side/2.0;
			pts[5].y= pos.y + height / 2.0;

			geometry_add_tile(geo, pts, 6);
		}
	}

	printf("ntiles: %d (%d) %d\n", geo->nsquares, ntiles);
	printf("nvertex: %d (%d) %d\n", geo->nvertex, nvertex);
	printf("nlines: %d (%d) %d\n", geo->nlines, nlines);

	/* make sure we didn't underestimate max numbers */
	g_assert(geo->nsquares <= ntiles);
	g_assert(geo->nvertex <= nvertex);
	g_assert(geo->nlines <= nlines);

	/* realloc to actual number of squares, vertices and lines */
	geo->squares= g_realloc(geo->squares, geo->nsquares*sizeof(struct square));
	geo->vertex= g_realloc(geo->vertex, geo->nvertex*sizeof(struct vertex));
	geo->lines= g_realloc(geo->lines, geo->nlines*sizeof(struct line));

	/* build inter-connections */
	geometry_connect_skeleton(geo);

	/* define sizes of drawing bits */
	hexagonal_calculate_sizes(geo, dimy);

	return geo;
}
