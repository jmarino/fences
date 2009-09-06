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
#define CAIRO_BOARD_SIZE	100.
#define CAIRO_BOARD_MARGIN	5.
#define CAIRO_GAME_SIZE	(CAIRO_BOARD_SIZE - 2*CAIRO_BOARD_MARGIN)

#define CAIRO_GAME_LEFT		(CAIRO_BOARD_MARGIN - 1.0)
#define CAIRO_GAME_RIGHT		(CAIRO_BOARD_SIZE - CAIRO_BOARD_MARGIN + 1.0)
#define CAIRO_GAME_TOP		(CAIRO_BOARD_MARGIN - 1.0)
#define CAIRO_GAME_BOTTOM	(CAIRO_BOARD_SIZE - CAIRO_BOARD_MARGIN + 1.0)




/*
 * Check that all 4 vertex of rhomb are inside game area.
 * TRUE: rhomb is fully inside game area
 */
static gboolean
cairotile_is_tile_inside(struct point *vertex, int nvertex)
{
	int i;

	for(i=0; i < nvertex; ++i) {
		if (vertex[i].x < CAIRO_GAME_LEFT || vertex[i].x > CAIRO_GAME_RIGHT ||
			vertex[i].y < CAIRO_GAME_TOP || vertex[i].y > CAIRO_GAME_BOTTOM)
			return FALSE;
	}
	return TRUE;
}


/*
 * Set vertex coordinates of rhombs in a unit
 */
static void
cairotile_fill_unit_with_tiles(struct geometry *geo, struct point *pos, double side)
{
	struct point pts[5];
	double half_side;
	double height;
	double shoulder_h;
	double shoulder_w;
	double lside;

	/* pos is point on the x left and y middle of eye unit */
	/* precalc some stuff */
	lside= side/(sqrt(3.0) - 1.0);
	shoulder_h= lside * sqrt(3.0)/2.0;
	shoulder_w= 2 * shoulder_h;
	half_side= side / 2.0;
	height= shoulder_h + lside/2.0;

	/* left pentagon */
	pts[0].x= pos->x;
	pts[0].y= pos->y;
	pts[1].x= pos->x;
	pts[1].y= pos->y - side;
	pts[2].x= pos->x + shoulder_h;
	pts[2].y= pos->y - (shoulder_h + half_side);
	pts[3].x= pos->x + height;
	pts[3].y= pos->y - half_side;
	pts[4].x= pos->x + shoulder_h;
	pts[4].y= pos->y + lside/2.0;
	if (cairotile_is_tile_inside(pts, 5))
		geometry_add_tile(geo, pts, 5, NULL);

	/* top pentagon */
	pts[0].x= pos->x + height;
	pts[0].y= pos->y - half_side;
	pts[1].x= pos->x + shoulder_h;
	pts[1].y= pos->y - (shoulder_h + half_side);
	pts[2].x= pos->x + (height + half_side);
	pts[2].y= pos->y - (height + half_side);
	pts[3].x= pos->x + (height + side + lside/2.0);
	pts[3].y= pos->y - (shoulder_h + half_side);
	pts[4].x= pos->x + (height + side);
	pts[4].y= pos->y - half_side;
	if (cairotile_is_tile_inside(pts, 5))
		geometry_add_tile(geo, pts, 5, NULL);

	/* right pentagon */
	pts[0].x= pos->x + (height + side);
	pts[0].y= pos->y - half_side;
	pts[1].x= pos->x + (height + side + lside/2.0);
	pts[1].y= pos->y - (half_side + shoulder_h);
	pts[2].x= pos->x + (2*height + side);
	pts[2].y= pos->y - side;
	pts[3].x= pos->x + (2*height + side);
	pts[3].y= pos->y;
	pts[4].x= pos->x + (shoulder_w + shoulder_h);
	pts[4].y= pos->y + lside/2.0;
	if (cairotile_is_tile_inside(pts, 5))
		geometry_add_tile(geo, pts, 5, NULL);

	/* bottom pentagon */
	pts[0].x= pos->x + height;
	pts[0].y= pos->y - half_side;
	pts[1].x= pos->x + (height + side);
	pts[1].y= pos->y - half_side;
	pts[2].x= pos->x + (shoulder_w + shoulder_h);
	pts[2].y= pos->y + lside/2.0;
	pts[3].x= pos->x + (height + half_side);
	pts[3].y= pos->y + lside;
	pts[4].x= pos->x + shoulder_h;
	pts[4].y= pos->y + lside/2.0;
	if (cairotile_is_tile_inside(pts, 5))
		geometry_add_tile(geo, pts, 5, NULL);
}


/*
 * Calculate sizes of drawing details
 */
static void
cairotile_calculate_sizes(struct geometry *geo, int dim)
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
build_cairo_tile_geometry(const struct gameinfo *info)
{
	struct geometry *geo;
	int i, j;
	int i1, i2;
	int num_hex;
	int dimy;
	double hex_size;
	double side;
	double lside;
	double shift;
	double xoffset;
	double height;
	int ntiles;
	int nvertex;
	int nlines;
	double x0;
	double y0;
	struct point pos;
	const int geo_params[5][3]= {	/* num of tiles, vertex, lines for all sizes */
		{ 24,  52,  75},
		{ 60, 116, 175},
		{112, 204, 315},
		{180, 316, 495},
		{264, 452, 715}
	};


	/* number of eyes (symmetry unit) wide */
	g_assert(info->size >= 0 && info-> size < 5);
	num_hex= info->size + 2;
	dimy= num_hex*2 + 1;
	hex_size= (sqrt(3.0) + 1.0)/(sqrt(3.0) - 1) + 1;
	side= CAIRO_GAME_SIZE/((double)num_hex * hex_size);
	lside= side/(sqrt(3.0) - 1.0);
	height= lside*(sqrt(3.0) + 1.0)/2.0;
	shift= height + side/2.0;

	/* coordinates of unit on top left (center of unit coords) */
	x0= CAIRO_BOARD_MARGIN - shift;
	y0= CAIRO_BOARD_MARGIN + side/2.0;

	/* create geometry with plenty of space for lines and vertex */
	ntiles= geo_params[info->size][0];
	nvertex= geo_params[info->size][1];
	nlines= geo_params[info->size][2];
	geo= geometry_create_new(ntiles, nvertex, nlines, 5);
	geo->board_size= CAIRO_BOARD_SIZE;
	geo->board_margin= CAIRO_BOARD_MARGIN;
	geo->game_size= CAIRO_BOARD_SIZE - 2*CAIRO_BOARD_MARGIN;
	geometry_set_distance_resolution(side/10.0);

	/* create rhombs (3 rhombs in each unit) */
	geo->ntiles= 0;
	geo->nlines= 0;
	geo->nvertex= 0;
	for(j=0; j <= dimy; ++j) {
		pos.y= y0 + j * shift;
		xoffset= (j % 2) * shift;
		if (j == 0 || j == dimy) {
			i1= 1;
			i2= num_hex - 1;
		} else {
			i1= 0;
			i2= num_hex;
		}
		for(i=i1; i <= i2; ++i) {
			pos.x= x0 + xoffset + i * shift*2;
			cairotile_fill_unit_with_tiles(geo, &pos, side);
		}
	}

	/* make sure we didn't underestimate max numbers */
	g_assert(geo->ntiles == ntiles);
	g_assert(geo->nvertex == nvertex);
	g_assert(geo->nlines == nlines);

	/* build inter-connections */
	geometry_connect_skeleton(geo);

	/* define sizes of drawing bits */
	cairotile_calculate_sizes(geo, info->size);

	return geo;
}
