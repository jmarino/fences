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
#define TRIHEX_BOARD_SIZE	100.
#define TRIHEX_BOARD_MARGIN	5.
#define TRIHEX_GAME_SIZE	(TRIHEX_BOARD_SIZE - 2*TRIHEX_BOARD_MARGIN)

#define TRIHEX_GAME_LEFT		(TRIHEX_BOARD_MARGIN - 1.0)
#define TRIHEX_GAME_RIGHT		(TRIHEX_BOARD_SIZE - TRIHEX_BOARD_MARGIN + 1.0)
#define TRIHEX_GAME_TOP		(TRIHEX_BOARD_MARGIN - 1.0)
#define TRIHEX_GAME_BOTTOM	(TRIHEX_BOARD_SIZE - TRIHEX_BOARD_MARGIN + 1.0)


/* convert degrees to radians */
#define D2R(x)		((x)*M_PI/180.0)

/* possible neighbors of repeated symmetry unit */
#define NEIGHBOR_NW		0x0001
#define NEIGHBOR_NE		0x0002
#define NEIGHBOR_W		0x0004


/*
 * Draw 12 sided round structure that repeats to form tile board
 */
static void
trihex_symmetry_unit(struct geometry *geo, struct point *pos, double side,
					 int neighbor)
{
	struct point pts[4];
	double half_side;
	double height;
	double angle;
	double angle30;
	int i;
	int ringmask;		/* mask with ring shapes to draw */

	/* set mask of ring shapes that are allowed to be drawn (according to neighbor).
	   First shape is right square and we go clockwise. */
	ringmask= ~0;
	if (neighbor & NEIGHBOR_W) ringmask&= ~(0x00E0); // 0000 1110 0000
	if (neighbor & NEIGHBOR_NW) ringmask&= ~(0x0380);	// 0011 1000 0000
	if (neighbor & NEIGHBOR_NE) ringmask&= ~(0x0E00);	// 1110 0000 0000

	/* pos is central point of 12 sided structure */
	height= side * sqrt(3.0)/2.0;
	half_side= side / 2.0;
	angle30= 30 * (M_PI/180.0);

	/* internal "trivial pursuit" pie region (inner triangles) */
	pts[0].x= pos->x;
	pts[0].y= pos->y;
	for(i=0; i < 6; ++i) {
		angle= i * D2R(60);
		pts[1].x= pos->x + side*cos(angle - D2R(30));
		pts[1].y= pos->y + side*sin(angle - D2R(30));
		pts[2].x= pos->x + side*cos(angle + D2R(30));
		pts[2].y= pos->y + side*sin(angle + D2R(30));
		geometry_add_tile(geo, pts, 3, NULL);
	}

	/* Outer ring of alternating squares and triangles */
	/* We need to pay attention to elements that may overlap with previously
	   drawn ones. We use ringmask to decide what we draw. */
	for(i=0; i < 6; ++i) {
		angle= D2R(i * 60);
		/* square */
		if (ringmask & 1) {
			pts[0].x= pos->x + side*cos(angle + angle30);
			pts[0].y= pos->y + side*sin(angle + angle30);
			pts[1].x= pts[0].x + side*cos(angle - D2R(90));
			pts[1].y= pts[0].y + side*sin(angle - D2R(90));
			pts[2].x= pts[0].x + side*sqrt(2.0)*cos(angle - D2R(45));
			pts[2].y= pts[0].y + side*sqrt(2.0)*sin(angle - D2R(45));
			pts[3].x= pts[0].x + side*cos(angle);
			pts[3].y= pts[0].y + side*sin(angle);
			geometry_add_tile(geo, pts, 4, NULL);
		}
		/* triangle */
		if (ringmask & 2) {
			/* pts[0]= square's pts[0] */
			pts[1].x= pts[0].x + side*cos(angle);
			pts[1].y= pts[0].y + side*sin(angle);
			pts[2].x= pts[0].x + side*cos(angle + D2R(60));
			pts[2].y= pts[0].y + side*sin(angle + D2R(60));
			geometry_add_tile(geo, pts, 3, NULL);
		}
		ringmask>>= 2;
	}
}


/*
 * Calculate sizes of drawing details
 */
static void
trihex_calculate_sizes(struct geometry *geo, int dim)
{
	geo->on_line_width= geo->game_size/(5.0 + dim*3.0)/15.0;
	geo->off_line_width= geo->board_size/1000.;
	if (geo->on_line_width < 2*geo->off_line_width)
		geo->on_line_width= 2*geo->off_line_width;
	geo->cross_line_width= geo->off_line_width*2;
	geo->cross_radius= geo->tile_width/10.0;
	geo->font_scale= 0.8;
}


/*
 * Build Qbert (quasiregular rhombic) tile geometry data
 */
struct geometry*
build_trihex_tile_geometry(const struct gameinfo *info)
{
	struct geometry *geo;
	int i, j;
	int dimx, dimy;
	double side;
	double xshift, yshift;
	double xoffset;
	int ntiles;
	int nvertex;
	int nlines;
	double x0;
	double y0;
	struct point pos;
	const int geo_params[5][3]= {	/* num of tiles, vertex, lines for all sizes */
		{ 46,  42,  87},
		{ 96,  79, 174},
		{183, 143, 325},
		{277, 208, 484},
		{465, 338, 802}
	};
	int neighbor=0;

	/* size parameter determines number of symmetry units */
	g_assert(info->size >= 0 && info->size < 5);
	dimx= info->size + 2;
	dimy= dimx;
	if (info->size == 4) ++dimy;

	/* num of sides wide: info->size*(1 + sqrt(3)) + 1 */
	side= TRIHEX_GAME_SIZE/(dimx*(1.0 + sqrt(3.0)) + 1.0);
	/* num of sides tall: info->size*(sqrt(3)/2 + 1 + 1/2) + sqrt(3)/2 + 1/2 */
	/* NOTE: shameless reuse of variable yshift */
	yshift= TRIHEX_GAME_SIZE/(dimy*(sqrt(3.0) + 3.0)/2.0 + (sqrt(3.0) + 1.0)/2.0);
	/* choose xfit or yfit (see which produces smaller side -> more limiting) */
	if (yshift < side) side= yshift;

	xshift= side*sqrt(3.0) + side;
	yshift= side*(sqrt(3.0) + 1.0)/2.0 + side;

	/* coordinates of unit on top left (center of unit coords) */
	x0= (dimx*(1.0 + sqrt(3.0)) + 1.0)*side;
	x0= (TRIHEX_BOARD_SIZE - x0)/2.0 + side*(1.0 + sqrt(3.0)/2.0);
	y0= (dimy*(3.0 + sqrt(3.0))/2.0 + (sqrt(3.0) + 1.0)/2.0)*side;
	y0= (TRIHEX_BOARD_SIZE - y0)/2.0 + (sqrt(3.0)/2.0 + 1.0)*side;

	/* create geometry with plenty of space for lines and vertex */
	ntiles= geo_params[info->size][0];
	nvertex= geo_params[info->size][1];
	nlines= geo_params[info->size][2];
	geo= geometry_create_new(ntiles, nvertex, nlines, 5);
	geo->board_size= TRIHEX_BOARD_SIZE;
	geo->board_margin= TRIHEX_BOARD_MARGIN;
	geo->game_size= TRIHEX_BOARD_SIZE - 2*TRIHEX_BOARD_MARGIN;
	geometry_set_distance_resolution(side/10.0);

	/* create units */
	geo->ntiles= 0;
	geo->nlines= 0;
	geo->nvertex= 0;
	for(j=0; j < dimy; ++j) {
		pos.y= y0 + j*yshift;
		xoffset= ((j+1) % 2) * xshift/2.0;
		if (j > 0) neighbor= NEIGHBOR_NE;
		for(i=0; i < dimx; ++i) {
			if (i == dimx - 1 && (j%2) == 0) break;
			if (j > 0) {
				if (i == 0) {
					if ((j%2) == 0) neighbor|= NEIGHBOR_NW;
				} else {
					if ((j%2) == 1) neighbor|= NEIGHBOR_NW;
				}
				if (i == dimx - 1 && ((j%2) == 1)) neighbor&= ~(NEIGHBOR_NE);
			}
			if (i > 0) neighbor|= NEIGHBOR_W;
			pos.x= x0 + xoffset + i * xshift;
			trihex_symmetry_unit(geo, &pos, side, neighbor);
		}
	}

	/* make sure we didn't underestimate max numbers */
	g_assert(geo->ntiles == ntiles);
	g_assert(geo->nvertex == nvertex);
	g_assert(geo->nlines == nlines);

	/* build inter-connections */
	geometry_connect_skeleton(geo);

	/* define sizes of drawing bits */
	trihex_calculate_sizes(geo, info->size);

	return geo;
}
