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
#define QBERT_BOARD_SIZE	100.
#define QBERT_BOARD_MARGIN	5.
#define QBERT_GAME_SIZE	(QBERT_BOARD_SIZE - 2*QBERT_BOARD_MARGIN)

#define QBERT_GAME_LEFT		(QBERT_BOARD_MARGIN - 1.0)
#define QBERT_GAME_RIGHT	(QBERT_BOARD_SIZE - QBERT_BOARD_MARGIN + 1.0)



/*
 * Check that all 4 vertex of rhomb are inside game area.
 * TRUE: rhomb is fully inside game area
 */
static gboolean
qbert_is_rhomb_inside(struct point *vertex)
{
	int i;

	for(i=0; i < 4; ++i) {
		if (vertex[i].x < QBERT_GAME_LEFT || vertex[i].x > QBERT_GAME_RIGHT ||
			vertex[i].y < QBERT_GAME_LEFT || vertex[i].y > QBERT_GAME_RIGHT)
		return FALSE;
	}
	return TRUE;
}


/*
 * Set vertex coordinates of rhombs in a unit
 */
static void
qbert_fill_unit_with_rhombs(struct geometry *geo, struct point *pos, double side)
{
	struct point pts[4];

	/* rhomb top right (vertex clockwise starting in the center) */
	pts[0].x= pos->x;
	pts[0].y= pos->y;
	pts[1].x= pos->x;
	pts[1].y= pos->y - side;
	pts[2].x= pos->x + side * sqrt(3.0) / 2.0;
	pts[2].y= pos->y - side / 2.0;
	pts[3].x= pos->x + side * sqrt(3.0) / 2.0;
	pts[3].y= pos->y + side / 2.0;
	if (qbert_is_rhomb_inside(pts)) {
		//g_message("  rhomb 1");
		geometry_add_tile(geo, pts, 4, NULL);
	}

	/* bottom (vertex clockwise starting in the center) */
	pts[0].x= pos->x;
	pts[0].y= pos->y;
	pts[1].x= pos->x + side * sqrt(3.0) / 2.0;
	pts[1].y= pos->y + side / 2.0;
	pts[2].x= pos->x;
	pts[2].y= pos->y + side;
	pts[3].x= pos->x - side * sqrt(3.0) / 2.0;
	pts[3].y= pos->y + side / 2.0;
	if (qbert_is_rhomb_inside(pts)) {
		//g_message("  rhomb 2");
		geometry_add_tile(geo, pts, 4, NULL);
	}

	/* rhomb top left (vertex clockwise starting in the center) */
	pts[0].x= pos->x;
	pts[0].y= pos->y;
	pts[1].x= pos->x - side * sqrt(3.0) / 2.0;
	pts[1].y= pos->y + side / 2.0;
	pts[2].x= pos->x - side * sqrt(3.0) / 2.0;
	pts[2].y= pos->y - side / 2.0;
	pts[3].x= pos->x;
	pts[3].y= pos->y - side;
	if (qbert_is_rhomb_inside(pts)) {
		//g_message("  rhomb 3");
		geometry_add_tile(geo, pts, 4, NULL);
	}
}


/*
 * Calculate sizes of drawing details
 */
static void
qbert_calculate_sizes(struct geometry *geo, int dim)
{
	geo->on_line_width= geo->game_size/dim/15.0;
	geo->off_line_width= geo->board_size/1000.;
	if (geo->on_line_width < 2*geo->off_line_width)
		geo->on_line_width= 2*geo->off_line_width;
	geo->cross_line_width= geo->off_line_width*2;
	geo->cross_radius= geo->tile_width/10.0;
	geo->font_scale= 1.5;
}


/*
 * Build Qbert (quasiregular rhombic) tile geometry data
 */
struct geometry*
build_qbert_tile_geometry(const struct gameinfo *info)
{
	struct geometry *geo;
	int i, j;
	int dimx;
	int dimy;
	int num_x=info->size;
	int num_y;
	double side;
	int nrhomb_max;
	int nvertex_max;
	int nlines_max;
	double x0;
	double xoffset;
	double y0;
	struct point pos;

	/* size of rhomb size: fit dimx vert rhombs wide */
	side= QBERT_GAME_SIZE/((double)num_x * sqrt(3.0)/2.0);

	/* check if this corresponds to an unbroken vertical setup,
	   i.e., vertical has whole number of side's */
	y0= QBERT_GAME_SIZE/side;
	num_y= (int)round(y0);
	/* if it's closer to ceil, then use it to estimate side, if not then clip. */
	if ((int)ceil(y0) == num_y) {
		side= QBERT_GAME_SIZE/ceil(y0);
	}

	/* number of units that fit wide and tall */
	dimx= (int)ceil(num_x / 2.0);
	dimy= (int)( QBERT_GAME_SIZE / (side*3.0/2.0) + 1.0 );

	/* coordinates of unit on top left (center of unit coords) */
	x0= QBERT_BOARD_MARGIN + sqrt(3.0) * side / 2.0;
	x0+= (QBERT_GAME_SIZE - num_x*(side*sqrt(3.0)/2.0)) / 2.0;
	y0= QBERT_BOARD_MARGIN + side;
	y0+= (QBERT_GAME_SIZE - num_y*side) / 2.0;
	/* found by trial-and-error, makes the game look better */
	if (num_y % 3 == 1) y0-= side;

	/* estimate max number of rhombs from game area */
	/* Area of one rhomb: sqrt(3)/2*side^2 */
	nrhomb_max= (int) (QBERT_GAME_SIZE * QBERT_GAME_SIZE /
					   (sqrt(3.0)* side * side / 2.0));

	/* create geometry with plenty of space for lines and vertex */
	nvertex_max= (int)(nrhomb_max*1.3);	/* found by trial-and-error (size 5>) */
	nlines_max= (int)(nrhomb_max*2.2);	/* found by trial-and-error (size 5>) */
	geo= geometry_create_new(nrhomb_max, nvertex_max, nlines_max, 4);
	geo->board_size= QBERT_BOARD_SIZE;
	geo->board_margin= QBERT_BOARD_MARGIN;
	geo->game_size= QBERT_BOARD_SIZE - 2*QBERT_BOARD_MARGIN;
	geometry_set_distance_resolution(side/10.0);

	/* create rhombs (3 rhombs in each unit) */
	geo->ntiles= geo->nlines= geo->nvertex= 0;
	for(j=0; j < dimy; ++j) {
		pos.y= y0 + j * (side + side/2.0);
		xoffset= x0 - (j % 2)*( sqrt(3.0)*side/2.0 );
		for(i=0; i < dimx + (j%2); ++i) {
			pos.x= xoffset + i*( sqrt(3.0)*side );
			qbert_fill_unit_with_rhombs(geo, &pos, side);
		}
	}

	/* make sure we didn't underestimate max numbers */
	g_assert(geo->ntiles <= nrhomb_max);
	g_assert(geo->nvertex <= nvertex_max);
	g_assert(geo->nlines <= nlines_max);

	/* realloc to actual number of tiles, vertices and lines */
	geo->tiles= g_realloc(geo->tiles, geo->ntiles*sizeof(struct tile));
	geo->vertex= g_realloc(geo->vertex, geo->nvertex*sizeof(struct vertex));
	geo->lines= g_realloc(geo->lines, geo->nlines*sizeof(struct line));

	/* build inter-connections */
	geometry_connect_skeleton(geo);

	/* define sizes of drawing bits */
	qbert_calculate_sizes(geo, dimy);

	return geo;
}
