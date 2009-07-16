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


static double QBERT_EPSILON=0.0;



/*
 * Find vertex in list of vertices
 * Returns pointer to vertex structure or NULL if not found.
 */
static struct vertex*
qbert_find_vertex(struct geometry *geo, struct point *point)
{
	int i;
	struct vertex *vertex;
	double x, y;

	if (geo->nvertex == 0) return NULL;

	/* search backwards to optimize */
	vertex= geo->vertex + (geo->nvertex - 1);
	for(i=0; i < geo->nvertex; ++i) {
		x= point->x - vertex->pos.x;
		y= point->y - vertex->pos.y;
		if ( x*x + y*y < QBERT_EPSILON)
			return vertex;
		--vertex;
	}
	return NULL;
}


/*
 * Find line in list of lines
 * Returns pointer to line structure or NULL if not found.
 */
static struct line*
qbert_find_line(struct geometry *geo, struct vertex *v1, struct vertex *v2)
{
	int i;
	struct line *lin;

	if (geo->nlines == 0) return NULL;

	/* search backwards to optimize */
	lin= geo->lines + (geo->nlines - 1);
	for(i=0; i < geo->nlines; ++i) {
		if ((lin->ends[0] == v1 && lin->ends[1] == v2) ||
			(lin->ends[1] == v1 && lin->ends[0] == v2))
			return lin;
		--lin;
	}
	return NULL;
}


/*
 * Add tile to list of tiles in geometry
 * Add vertices as required.
 * Add and connect lines as required
 */
static void
qbert_add_tile(struct geometry *geo, struct point *pts)
{
	struct square *sq;
	struct vertex *vertex;
	struct vertex *vlist[4];
	struct line *lin;
	int i, i2;

	sq= geo->squares + geo->nsquares;
	sq->id= geo->nsquares;
	sq->nvertex= 0;
	sq->vertex= NULL;
	sq->nsides= 0;
	sq->sides= NULL;
	sq->fx_status= sq->fx_frame= 0;
	++geo->nsquares;

	/* add vertices */
	for (i=0; i < 4; ++i) {
		vertex= qbert_find_vertex(geo, pts + i);
		if (vertex == NULL) {
			vertex= geo->vertex + geo->nvertex;
			vertex->id= geo->nvertex;
			vertex->pos.x= pts[i].x;
			vertex->pos.y= pts[i].y;
			vertex->nlines= 0;
			vertex->lines= NULL;
			vertex->nsquares= 0;
			vertex->sq= NULL;
			++geo->nvertex;
		}
		vlist[i]= vertex;
	}
	/* add lines */
	for (i=0; i < 4; ++i) {
		i2= (i + 1) % 4;
		lin= qbert_find_line(geo, vlist[i], vlist[i2]);
		if (lin == NULL) {
			lin= geo->lines + geo->nlines;
			lin->id= geo->nlines;
			lin->ends[0]= vlist[i];
			lin->ends[1]= vlist[i2];
			lin->nsquares= 0;
			lin->nin= 0;
			lin->in= NULL;
			lin->nout= 0;
			lin->out= NULL;
			lin->fx_status= lin->fx_frame= 0;
			++geo->nlines;
		}
		/* connect square to line */
		g_assert(lin->nsquares < 2);
		lin->sq[lin->nsquares]= sq;
		++lin->nsquares;
	}
}


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
		qbert_add_tile(geo, pts);
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
		qbert_add_tile(geo, pts);
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
		qbert_add_tile(geo, pts);
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
	geo->cross_radius= geo->sq_width/10.0;
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
	if ((int)ceil(y0) == num_y) {
		side= QBERT_GAME_SIZE/ceil(y0);
		g_message("Y changed. new side: %lf", side);
	}

	QBERT_EPSILON= (side/10.0)*(side/10.0);

	/* number of units that fit wide and tall */
	dimx= (int)ceil(num_x / 2.0);
	dimy= (int)( QBERT_GAME_SIZE / (side*3.0/2.0) + 1.0 );

	g_message("dims: %dx%d ;   side: %lf", dimx, dimy, side);

	/* coordinates of unit on top left (center of unit coords) */
	x0= QBERT_BOARD_MARGIN + sqrt(3.0) * side / 2.0;
	x0+= (QBERT_GAME_SIZE - num_x*(side*sqrt(3.0)/2.0)) / 2.0;
	y0= QBERT_BOARD_MARGIN + side;
	y0+= (QBERT_GAME_SIZE - num_y*side) / 2.0;
	if (num_y % 3 == 1) y0-= side;

	/* estimate max number of rhombs from game area */
	/* Area of one rhomb: sqrt(3)/2*side^2 */
	nrhomb_max= (int) (QBERT_GAME_SIZE * QBERT_GAME_SIZE /
					   (sqrt(3.0)* side * side / 2.0));

	/* create geometry with plenty of space for lines and vertex */
	geo= geometry_create_new(nrhomb_max, nrhomb_max*4, nrhomb_max*4, 4);
	geo->board_size= QBERT_BOARD_SIZE;
	geo->board_margin= QBERT_BOARD_MARGIN;
	geo->game_size= QBERT_BOARD_SIZE - 2*QBERT_BOARD_MARGIN;

	/* create rhombs (3 rhombs in each unit) */
	geo->nsquares= geo->nlines= geo->nvertex= 0;
	for(j=0; j < dimy; ++j) {
		pos.y= y0 + j * (side + side/2.0);
		xoffset= x0 - (j % 2)*( sqrt(3.0)*side/2.0 );
		for(i=0; i < dimx + (j%2); ++i) {
			pos.x= xoffset + i*( sqrt(3.0)*side );
			qbert_fill_unit_with_rhombs(geo, &pos, side);
		}
	}

	g_message("|| nsquares: %d (%d)", geo->nsquares, nrhomb_max);
	g_message("|| nlines: %d (%d)", geo->nlines, 4*nrhomb_max);
	g_message("|| nvertex: %d (%d)", geo->nvertex, 4*nrhomb_max);

	/* realloc to actual number of squares, vertices and lines */
	geo->squares= g_realloc(geo->squares, geo->nsquares*sizeof(struct square));
	geo->vertex= g_realloc(geo->vertex, geo->nvertex*sizeof(struct vertex));
	geo->lines= g_realloc(geo->lines, geo->nlines*sizeof(struct line));

	/* build inter-connections */
	geometry_connect_elements_qbert(geo);

	/* define sizes of drawing bits */
	qbert_calculate_sizes(geo, dimy);


	/* DEBUG */
	/* grab a square and print everything about it */
	struct square *sq=geo->squares + 5;
	printf("Square %d:\n", sq->id);
	printf("  nsides: %d, nvertex: %d\n", sq->nsides, sq->nvertex);
	printf("  Sides: ");
	for(i=0; i < sq->nsides; ++i) printf("%d  ", sq->sides[i]->id);
	printf("\n  Vertex: ");
	for(i=0; i < sq->nvertex; ++i) printf("%d  ", sq->vertex[i]->id);
	printf("\n");

	sq= geo->squares+6;
	printf("Square %d:\n", sq->id);
	printf("  nsides: %d, nvertex: %d\n", sq->nsides, sq->nvertex);
	printf("  Sides: ");
	for(i=0; i < sq->nsides; ++i) printf("%d  ", sq->sides[i]->id);
	printf("\n  Vertex: ");
	for(i=0; i < sq->nvertex; ++i) printf("%d  ", sq->vertex[i]->id);
	printf("\n");
	return geo;
}
