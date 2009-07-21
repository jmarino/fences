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
#define SNUB_BOARD_SIZE	100.
#define SNUB_BOARD_MARGIN	5.
#define SNUB_GAME_SIZE	(SNUB_BOARD_SIZE - 2*SNUB_BOARD_MARGIN)

#define SNUB_GAME_LEFT		(SNUB_BOARD_MARGIN - 1.0)
#define SNUB_GAME_RIGHT		(SNUB_BOARD_SIZE - SNUB_BOARD_MARGIN + 1.0)
#define SNUB_GAME_TOP		(SNUB_BOARD_MARGIN + 2.0)
#define SNUB_GAME_BOTTOM	(SNUB_BOARD_SIZE - SNUB_BOARD_MARGIN - 2.0)




/*
 * Check that all 4 vertex of rhomb are inside game area.
 * TRUE: rhomb is fully inside game area
 */
static gboolean
snub_is_tile_inside(struct point *vertex, int nvertex)
{
	int i;

	for(i=0; i < nvertex; ++i) {
		if (vertex[i].x < SNUB_GAME_LEFT || vertex[i].x > SNUB_GAME_RIGHT ||
			vertex[i].y < SNUB_GAME_TOP || vertex[i].y > SNUB_GAME_BOTTOM)
		return FALSE;
	}
	return TRUE;
}


/*
 * Set vertex coordinates of rhombs in a unit
 */
static void
snub_fill_unit_with_tiles(struct geometry *geo, struct point *pos, double side)
{
	struct point pts[4];
	double half_side;
	double height;
	double sq_wide;
	int i;

	/* pos is point on the x left and y middle of eye unit */
	/* precalc some stuff */
	half_side= side / 2.0;
	height= side * sqrt(3.0)/2.0;
	sq_wide= height + half_side;

	/* triangle left (looking up) */
	pts[0].x= pos->x;
	pts[0].y= pos->y;
	pts[1].x= pos->x + half_side;
	pts[1].y= pos->y - height;
	pts[2].x= pos->x + side;
	pts[2].y= pos->y;
	if (snub_is_tile_inside(pts, 3))
		geometry_add_tile(geo, pts, 3);

	/* triangle bottom middle (looking up) */
	for(i=0; i < 3; ++i) {
		pts[i].x+= sq_wide;
		pts[i].y+= sq_wide;
	}
	if (snub_is_tile_inside(pts, 3))
		geometry_add_tile(geo, pts, 3);

	/* triangle left (looking down) */
	pts[0].x= pos->x;
	pts[0].y= pos->y;
	pts[1].x= pos->x + side;
	pts[1].y= pos->y;
	pts[2].x= pos->x + half_side;
	pts[2].y= pos->y + height;
	if (snub_is_tile_inside(pts, 3))
		geometry_add_tile(geo, pts, 3);

	/* triangle top center (looking down) */
	for(i=0; i < 3; ++i) {
		pts[i].x+= sq_wide;
		pts[i].y-= sq_wide;
	}
	if (snub_is_tile_inside(pts, 3))
		geometry_add_tile(geo, pts, 3);

	/* triangle center (looking left) */
	pts[0].x= pos->x + side;
	pts[0].y= pos->y;
	pts[1].x= pos->x + (height + side);
	pts[1].y= pos->y - half_side;
	pts[2].x= pos->x + (height + side);
	pts[2].y= pos->y + half_side;
	/* point 2 in the same as before */
	if (snub_is_tile_inside(pts, 3))
		geometry_add_tile(geo, pts, 3);

	/* triangle bottom right (looking left) */
	for(i=0; i < 3; ++i) {
		pts[i].x+= sq_wide;
		pts[i].y+= sq_wide;
	}
	if (snub_is_tile_inside(pts, 3))
		geometry_add_tile(geo, pts, 3);

	/* triangle bottom left (looking right) */
	pts[0].x= pos->x + half_side;
	pts[0].y= pos->y + height;
	pts[1].x= pos->x + sq_wide;
	pts[1].y= pos->y + sq_wide;
	pts[2].x= pos->x + half_side;
	pts[2].y= pos->y + (height + side);
	if (snub_is_tile_inside(pts, 3))
		geometry_add_tile(geo, pts, 3);

	/* triangle center (looking right) */
	for(i=0; i < 3; ++i) {
		pts[i].x+= sq_wide;
		pts[i].y-= sq_wide;
	}
	if (snub_is_tile_inside(pts, 3))
		geometry_add_tile(geo, pts, 3);

	/* square top left */
	pts[0].x= pos->x + half_side;
	pts[0].y= pos->y - height;
	pts[1].x= pos->x + sq_wide;
	pts[1].y= pos->y - sq_wide;
	pts[2].x= pos->x + (sq_wide + half_side);
	pts[2].y= pos->y - half_side;
	pts[3].x= pos->x + side;
	pts[3].y= pos->y;
	if (snub_is_tile_inside(pts, 4)) {
		geometry_add_tile(geo, pts, 4);
	}

	/* square bot right */
	for(i=0; i < 4; ++i) {
		pts[i].x+= sq_wide;
		pts[i].y+= sq_wide;
	}
	if (snub_is_tile_inside(pts, 4))
		geometry_add_tile(geo, pts, 4);

	/* square bottom left */
	pts[0].x= pos->x + side;
	pts[0].y= pos->y;
	pts[1].x= pos->x + (sq_wide + half_side);
	pts[1].y= pos->y + half_side;
	pts[2].x= pos->x + sq_wide;
	pts[2].y= pos->y + sq_wide;
	pts[3].x= pos->x + half_side;
	pts[3].y= pos->y + height;
	if (snub_is_tile_inside(pts, 4))
		geometry_add_tile(geo, pts, 4);

	/* square top right */
	for(i=0; i < 4; ++i) {
		pts[i].x+= sq_wide;
		pts[i].y-= sq_wide;
	}
	if (snub_is_tile_inside(pts, 4))
		geometry_add_tile(geo, pts, 4);
}


/*
 * Calculate sizes of drawing details
 */
static void
snub_calculate_sizes(struct geometry *geo, int dim)
{
	geo->on_line_width= geo->game_size/(5.0 + dim*3.0)/15.0;
	geo->off_line_width= geo->board_size/1000.;
	if (geo->on_line_width < 2*geo->off_line_width)
		geo->on_line_width= 2*geo->off_line_width;
	geo->cross_line_width= geo->off_line_width*2;
	geo->cross_radius= geo->sq_width/10.0;
	geo->font_scale= 0.8;
}


/*
 * Build Qbert (quasiregular rhombic) tile geometry data
 */
struct geometry*
build_snub_tile_geometry(const struct gameinfo *info)
{
	struct geometry *geo;
	int i, j;
	int num_eyes;
	double side;
	int ntiles;
	int nvertex;
	int nlines;
	double x0;
	double y0;
	struct point pos;
	const int geo_params[5][3]= {	/* num of tiles, vertex, lines for all sizes */
		{ 48,  44,  91},
		{108,  90, 197},
		{192, 152, 343},
		{300, 230, 529},
		{432, 324, 755}
	};


	/* number of eyes (symmetry unit) wide */
	g_assert(info->size >= 0 && info-> size < 5);
	num_eyes= info->size + 2;
	side= SNUB_GAME_SIZE/((double)(num_eyes + 1) + num_eyes*sqrt(3.0));

	/* coordinates of unit on top left (center of unit coords) */
	x0= SNUB_BOARD_MARGIN;
	y0= (SNUB_GAME_SIZE - (sqrt(3.0) + 1.0)*side*num_eyes) / 2.0;
	y0= SNUB_BOARD_MARGIN + y0 + (sqrt(3.0) + 1.0)*side/2.0;

	/* create geometry with plenty of space for lines and vertex */
	ntiles= geo_params[info->size][0];
	nvertex= geo_params[info->size][1];
	nlines= geo_params[info->size][2];
	geo= geometry_create_new(ntiles, nvertex, nlines, 4);
	geo->board_size= SNUB_BOARD_SIZE;
	geo->board_margin= SNUB_BOARD_MARGIN;
	geo->game_size= SNUB_BOARD_SIZE - 2*SNUB_BOARD_MARGIN;
	geometry_set_distance_resolution(side/10.0);

	/* create rhombs (3 rhombs in each unit) */
	geo->nsquares= 0;
	geo->nlines= 0;
	geo->nvertex= 0;
	for(j=0; j < num_eyes; ++j) {
		pos.y= y0 + j * (sqrt(3) + 1.0)*side;
		for(i=0; i <= num_eyes; ++i) {
			pos.x= x0 + i * (sqrt(3) + 1.0)*side;
			snub_fill_unit_with_tiles(geo, &pos, side);
		}
	}

	/* make sure we didn't underestimate max numbers */
	g_assert(geo->nsquares == ntiles);
	g_assert(geo->nvertex == nvertex);
	g_assert(geo->nlines == nlines);

	/* build inter-connections */
	geometry_connect_skeleton(geo);

	/* define sizes of drawing bits */
	snub_calculate_sizes(geo, info->size);

	return geo;
}
