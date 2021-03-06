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

#include <stdio.h>
#include <glib.h>
#include <math.h>
#include <cairo.h>
#include <string.h>

#include "geometry.h"
#include "tiles.h"


/* prefered board dimensions for penrose tile */
#define PENROSE_BOARD_SIZE	100.
#define PENROSE_BOARD_MARGIN	2.
#define PENROSE_GAME_SIZE	(PENROSE_BOARD_SIZE - 2*PENROSE_BOARD_MARGIN)


/* Two types of rombs */
enum {
	FAT_ROMB,
	THIN_ROMB
};

// angle increases clockwise
struct romb {
	int type;
	struct point pos;	// coords of romb vertex
	double side;		// length of side
	double angle;		// angle of romb (0=horizontal/right; pi/2: down)
	struct point center;
};


#define RATIO		1.6180339887
#define D2R(x)		((x)/180.0*M_PI)
#define WRAP(x)		x= ((x) > 2.*M_PI) ? (x) -  2.*M_PI : (x)



static void draw_penrose_tile(GSList *penrose);
static void get_romb_vertices(struct romb *romb, struct point *vertex);


/* contains minimum distance required to consider two vertices different */
static double separate_distance=0.;



/*
 * Unfold a fat romb
 * Add new rombs to list newpenrose and return it
 */
static GSList*
penrose_unfold_fatromb(GSList *newpenrose, struct romb *romb)
{
	struct romb *nromb;
	double nside=romb->side/RATIO;

	g_assert(romb->type == FAT_ROMB);

	/* create new romb 1/5 (I'm going clockwise) */
	nromb= (struct romb *)g_malloc(sizeof(struct romb));
	nromb->type= FAT_ROMB;
	nromb->pos.x= romb->pos.x + romb->side * cos(romb->angle - D2R(36));
	nromb->pos.y= romb->pos.y + romb->side * sin(romb->angle - D2R(36));
	nromb->side= nside;
	nromb->angle= romb->angle + D2R(180 - 36);
	WRAP(nromb->angle);
	nromb->center.x= nromb->pos.x + nromb->side*RATIO/2.0*cos(nromb->angle);
	nromb->center.y= nromb->pos.y + nromb->side*RATIO/2.0*sin(nromb->angle);
	newpenrose= g_slist_prepend(newpenrose, nromb);

	/* next romb 2/5 */
	nromb= (struct romb *)g_malloc(sizeof(struct romb));
	nromb->type= THIN_ROMB;
	nromb->pos.x= romb->pos.x + nside * cos(romb->angle);
	nromb->pos.y= romb->pos.y + nside * sin(romb->angle);
	nromb->side= nside;
	nromb->angle= romb->angle + D2R(360 - (36 + 18));
	WRAP(nromb->angle);
	nromb->center.x= nromb->pos.x + nromb->side*cos(D2R(18))*cos(nromb->angle);
	nromb->center.y= nromb->pos.y + nromb->side*cos(D2R(18))*sin(nromb->angle);
	newpenrose= g_slist_prepend(newpenrose, nromb);

	/* next romb 3/5 */
	nromb= (struct romb *)g_malloc(sizeof(struct romb));
	nromb->type= FAT_ROMB;
	nromb->pos.x= romb->pos.x + (nside + romb->side) * cos(romb->angle);
	nromb->pos.y= romb->pos.y + (nside + romb->side) * sin(romb->angle);
	nromb->side= nside;
	nromb->angle= romb->angle + D2R(180);
	WRAP(nromb->angle);
	nromb->center.x= nromb->pos.x + nromb->side*RATIO/2.0*cos(nromb->angle);
	nromb->center.y= nromb->pos.y + nromb->side*RATIO/2.0*sin(nromb->angle);
	newpenrose= g_slist_prepend(newpenrose, nromb);

	/* next romb 4/5 */
	nromb= (struct romb *)g_malloc(sizeof(struct romb));
	nromb->type= THIN_ROMB;
	nromb->pos.x= romb->pos.x + nside * cos(romb->angle);
	nromb->pos.x+= 2.*nside*cos(D2R(18)) * cos(romb->angle + D2R(36 + 18));
	nromb->pos.y= romb->pos.y + nside * sin(romb->angle);
	nromb->pos.y+= 2.*nside*cos(D2R(18)) * sin(romb->angle + D2R(36 + 18));
	nromb->side= nside;
	nromb->angle= romb->angle + D2R(180 + 36 + 18);
	WRAP(nromb->angle);
	nromb->center.x= nromb->pos.x + nromb->side*cos(D2R(18))*cos(nromb->angle);
	nromb->center.y= nromb->pos.y + nromb->side*cos(D2R(18))*sin(nromb->angle);
	newpenrose= g_slist_prepend(newpenrose, nromb);

	/* next romb 5/5 */
	nromb= (struct romb *)g_malloc(sizeof(struct romb));
	nromb->type= FAT_ROMB;
	nromb->pos.x= romb->pos.x + romb->side * cos(romb->angle + D2R(36));
	nromb->pos.y= romb->pos.y + romb->side * sin(romb->angle + D2R(36));
	nromb->side= nside;
	nromb->angle= romb->angle + D2R(180 + 36);
	WRAP(nromb->angle);
	nromb->center.x= nromb->pos.x + nromb->side*RATIO/2.0*cos(nromb->angle);
	nromb->center.y= nromb->pos.y + nromb->side*RATIO/2.0*sin(nromb->angle);
	newpenrose= g_slist_prepend(newpenrose, nromb);

	return newpenrose;
}


/*
 * Unfold a thin romb
 * Add new rombs to list newpenrose and return it
 */
static GSList*
penrose_unfold_thinromb(GSList *newpenrose, struct romb *romb)
{
	struct romb *nromb;
	double nside=romb->side/RATIO;

	g_assert(romb->type == THIN_ROMB);

	/* create new romb 1/4 (I'm going clockwise) */
	nromb= (struct romb *)g_malloc(sizeof(struct romb));
	nromb->type= FAT_ROMB;
	nromb->pos.x= romb->pos.x;
	nromb->pos.y= romb->pos.y;
	nromb->side= nside;
	nromb->angle= romb->angle + D2R(270 + (90 - 18));
	WRAP(nromb->angle);
	nromb->center.x= nromb->pos.x + nromb->side*RATIO/2.0*cos(nromb->angle);
	nromb->center.y= nromb->pos.y + nromb->side*RATIO/2.0*sin(nromb->angle);
	newpenrose= g_slist_prepend(newpenrose, nromb);

	/* next romb 2/4 */
	nromb= (struct romb *)g_malloc(sizeof(struct romb));
	nromb->type= FAT_ROMB;
	nromb->pos.x= romb->pos.x + (2 * romb->side * cos(D2R(18))) * cos(romb->angle);
	nromb->pos.y= romb->pos.y + (2 * romb->side * cos(D2R(18))) * sin(romb->angle);
	nromb->side= nside;
	nromb->angle= romb->angle + D2R(180 + 18);
	WRAP(nromb->angle);
	nromb->center.x= nromb->pos.x + nromb->side*RATIO/2.0*cos(nromb->angle);
	nromb->center.y= nromb->pos.y + nromb->side*RATIO/2.0*sin(nromb->angle);
	newpenrose= g_slist_prepend(newpenrose, nromb);

	/* next romb 3/4 */
	nromb= (struct romb *)g_malloc(sizeof(struct romb));
	nromb->type= THIN_ROMB;
	nromb->pos.x= romb->pos.x + romb->side * cos(romb->angle + D2R(18));
	nromb->pos.x+= nside * cos(romb->angle + D2R(90 - 36));
	nromb->pos.y= romb->pos.y + romb->side * sin(romb->angle + D2R(18));
	nromb->pos.y+= nside * sin(romb->angle + D2R(90 - 36));
	nromb->side= nside;
	nromb->angle= romb->angle + D2R(270 - 18);
	WRAP(nromb->angle);
	nromb->center.x= nromb->pos.x + nromb->side*cos(D2R(18))*cos(nromb->angle);
	nromb->center.y= nromb->pos.y + nromb->side*cos(D2R(18))*sin(nromb->angle);
	newpenrose= g_slist_prepend(newpenrose, nromb);

	/* next romb 4/4 */
	nromb= (struct romb *)g_malloc(sizeof(struct romb));
	nromb->type= THIN_ROMB;
	nromb->pos.x= romb->pos.x + romb->side * cos(romb->angle - D2R(18));
	nromb->pos.y= romb->pos.y + romb->side * sin(romb->angle - D2R(18));
	nromb->side= nside;
	nromb->angle= romb->angle + D2R(90 + 18);
	WRAP(nromb->angle);
	nromb->center.x= nromb->pos.x + nromb->side*cos(D2R(18))*cos(nromb->angle);
	nromb->center.y= nromb->pos.y + nromb->side*cos(D2R(18))*sin(nromb->angle);
	newpenrose= g_slist_prepend(newpenrose, nromb);

	return newpenrose;
}


/*
 * Check if two rombs are the same
 * Finds center of both rombs and see if they're within side/10.
 * **NOTE** unused
 */
static gboolean
are_rombs_same(struct romb *r1, struct romb *r2)
{
	double x, y;

	if (r1->type != r2->type) return FALSE;
	if (r1->type == FAT_ROMB) {
		x= r1->pos.x + r1->side*RATIO/2.0*cos(r1->angle);
		y= r1->pos.y + r1->side*RATIO/2.0*sin(r1->angle);
		x-= r2->pos.x + r2->side*RATIO/2.0*cos(r2->angle);
		y-= r2->pos.y + r2->side*RATIO/2.0*sin(r2->angle);
	} else {
		x= r1->pos.x + r1->side*cos(D2R(18))*cos(r1->angle);
		y= r1->pos.y + r1->side*cos(D2R(18))*sin(r1->angle);
		x-= r2->pos.x + r2->side*cos(D2R(18))*cos(r2->angle);
		y-= r2->pos.y + r2->side*cos(D2R(18))*sin(r2->angle);
	}
	if ( sqrt(x*x + y*y) < r1->side/10. ) return TRUE;
	return FALSE;
}


/*
 * Eliminate repeated rombs in the list
 * Returns new trimmed list
 */
GSList *
trim_repeated_rombs(GSList *penrose)
{
	GSList *current;
	GSList *next;
	GSList *p;
	struct romb *r1, *r2;
	struct point c;

	current= penrose;
	while(current != NULL) {
		p= g_slist_next(current);
		r1= (struct romb*)current->data;
		/* iterate over rest of rombs */
		while(p != NULL) {
			r2= (struct romb*)p->data;
			if (r1->type != r2->type) { // not same type, next
				p= g_slist_next(p);
				continue;
			}
			/* calculate dist between centers of both rombs */
			c.x= r2->center.x - r1->center.x;
			c.y= r2->center.y - r1->center.y;
			if ((c.x*c.x + c.y*c.y) < separate_distance ) { // same romb
				//g_debug("delete romb, dist: %lf < %lf", dist, r1->side/10.);
				g_free(p->data);
				next= g_slist_next(p);
				penrose= g_slist_delete_link(penrose, p);
				p= next;
			} else {
				p= g_slist_next(p);
			}
		}
		current= g_slist_next(current);
	}
	return penrose;
}


/*
 * Eliminate rombs outside a certain radius
 * Returns new trimmed list
 */
static GSList *
trim_outside_rombs(GSList *penrose, double radius)
{
	GSList *current;
	GSList *next;
	struct romb *romb;
	struct point vertex[4];
	double dist;
	int i;
	double center=PENROSE_BOARD_SIZE/2.;

	current= penrose;
	while(current != NULL) {
		romb= (struct romb*) current->data;

		get_romb_vertices(romb, vertex);
		next= g_slist_next(current);
		for(i=0; i < 4; ++i) {
			vertex[i].x-= center;
			vertex[i].y-= center;
			dist= sqrt(vertex[i].x*vertex[i].x + vertex[i].y*vertex[i].y);
			if (dist > radius) {
				g_free(current->data);
				penrose= g_slist_delete_link(penrose, current);
				break;
			}
		}
		current= next;
	}
	return penrose;
}


/*
 * Unfold current list of rombs
 */
static GSList*
penrose_unfold(GSList* penrose, double edge)
{
	GSList *newpenrose=NULL;
	struct romb *romb;

	while(penrose != NULL) {
		romb= (struct romb*)penrose->data;
		switch(romb->type) {
		case FAT_ROMB:
			newpenrose= penrose_unfold_fatromb(newpenrose, romb);
			break;
		case THIN_ROMB:
			newpenrose= penrose_unfold_thinromb(newpenrose, romb);
			break;
		default:
			g_debug("penrose_unfold: unknown romb type: %d", romb->type);
		}
		g_free(romb);
		penrose= g_slist_delete_link(penrose, penrose);
	}

	/* get rid of repeated rombs */
	separate_distance= ((struct romb*)newpenrose->data)->side/10.;
	separate_distance*= separate_distance;
	newpenrose= trim_repeated_rombs(newpenrose);

	/* get rid of rombs outside a certain radius */
	if (edge > 0)
		newpenrose= trim_outside_rombs(newpenrose, edge);

	/* debug: count number of rombs in list */
	g_debug("rombs in list: %d", g_slist_length(newpenrose));

	return newpenrose;
}


/*
 * Return coordinates of vertices of romb
 */
static void
get_romb_vertices(struct romb *romb, struct point *vertex)
{
	vertex[0].x= romb->pos.x;
	vertex[0].y= romb->pos.y;
	if (romb->type == FAT_ROMB) {
		vertex[1].x= romb->pos.x + romb->side*cos(romb->angle - D2R(36));
		vertex[1].y= romb->pos.y + romb->side*sin(romb->angle - D2R(36));
		vertex[2].x= romb->pos.x + romb->side*RATIO*cos(romb->angle);
		vertex[2].y= romb->pos.y + romb->side*RATIO*sin(romb->angle);
		vertex[3].x= romb->pos.x + romb->side*cos(romb->angle + D2R(36));
		vertex[3].y= romb->pos.y + romb->side*sin(romb->angle + D2R(36));
	} else {
		vertex[1].x= romb->pos.x + romb->side*cos(romb->angle - D2R(18));
		vertex[1].y= romb->pos.y + romb->side*sin(romb->angle - D2R(18));
		vertex[2].x= romb->pos.x + 2*romb->side*cos(D2R(18))*cos(romb->angle);
		vertex[2].y= romb->pos.y + 2*romb->side*cos(D2R(18))*sin(romb->angle);
		vertex[3].x= romb->pos.x + romb->side*cos(romb->angle + D2R(18));
		vertex[3].y= romb->pos.y + romb->side*sin(romb->angle + D2R(18));
	}
}


/*
 * Calculate sizes of drawing details
 */
static void
penrose_calculate_sizes(struct geometry *geo, int size_index)
{
	geo->on_line_width= geo->board_size/(5.0 + size_index*3.0)/15.0;
	geo->off_line_width= geo->board_size/1000.;
	if (geo->on_line_width < 2*geo->off_line_width)
		geo->on_line_width= 2*geo->off_line_width;
	geo->cross_line_width= geo->off_line_width*2;
	geo->cross_radius= MIN(geo->tile_width, geo->tile_height)/5.;
	geo->font_scale= 2.;
}


/*
 * Transform list of rombs to tile skeleton (no connections)
 */
static struct geometry*
penrose_tile_to_skeleton(GSList *penrose, double side)
{
	struct geometry *geo;
	GSList *list;
	struct point pts[4];
	int i;
	int ntiles;
	int nvertex;
	int nlines;

	/* create new geometry (ntiles, nvertex, nlines) */
	/* NOTE: oversize nvertex and nlines. Will adjust below
	   Oversize factors determined by trial and error. */
	ntiles= g_slist_length(penrose);
	nvertex= (int)(ntiles*1.5);
	nlines= (int)(ntiles*2.5);
	geo= geometry_create_new(ntiles, nvertex, nlines, 4);
	geo->board_size= PENROSE_BOARD_SIZE;
	geo->board_margin= PENROSE_BOARD_MARGIN;
	geo->game_size= geo->board_size - 2*geo->board_margin;
	geometry_set_distance_resolution(side/10.0);


	/* iterate through tiles creating skeleton geometry
	   (skeleton geometry: lines hold all the topology info) */
	geo->ntiles= 0;
	geo->nlines= 0;
	geo->nvertex= 0;
	list= penrose;
	for(i=0; i < ntiles; ++i) {
		/* get vertices of tile (rhomb) and add it to skeleton geometry */
		get_romb_vertices((struct romb*)list->data, pts);
		geometry_add_tile(geo, pts, 4, NULL);

		list= g_slist_next(list);
	}

	/* make sure we didn't underestimate max numbers */
	g_assert(geo->ntiles <= ntiles);
	g_assert(geo->nvertex <= nvertex);
	g_assert(geo->nlines <= nlines);

	/* realloc to actual number of vertices and lines */
	geo->vertex= g_realloc(geo->vertex, geo->nvertex*sizeof(struct vertex));
	geo->lines= g_realloc(geo->lines, geo->nlines*sizeof(struct line));

	/*printf("ntiles: %d (%d) %d\n", geo->ntiles, ntiles);
	printf("nvertex: %d (%d) %d\n", geo->nvertex, nvertex);
	printf("nlines: %d (%d) %d\n", geo->nlines, nlines);*/

	return geo;
}


/*
 * Define seed to generate penrose tile
 * We use seed that generates krazydad's tile with 4 unfoldings:
 *	5 fat rombs forming a star, with star tip pointing down
 * Input 'side' is size of initial rombs size
 */
static GSList*
create_tile_seed(double side)
{
	GSList *penrose=NULL;
	struct romb *romb;
	int i;
	int angle=90;	// angle of star tip romb

	for(i=0; i < 5; ++i) {
		romb= (struct romb*)g_malloc(sizeof(struct romb));
		romb->type= FAT_ROMB;
		romb->side= side;
		romb->angle= D2R(angle);
		romb->pos.x= PENROSE_BOARD_SIZE/2.;
		romb->pos.y= PENROSE_BOARD_SIZE/2.;
		penrose= g_slist_prepend(penrose, romb);
		angle= (angle + 72)%360;
	}

	return penrose;
}


/*
 * Compute side and number of folds to produce a penrose
 * of the requested size index
 * Returns: number of folds
 *  side size is passed by pointer in '*side'
 */
static int
penrose_calculate_params(int size_index, double *side)
{
	int nfolds=0;

	/*
	 * radius of penrose tiling must be game_size/2
	 *  game_size/2 = side * Factor
	 * Factor is number of sides that fit in radius.
	 *   Any rhomb side:  1
	 *   Fat rhomb height: RATIO
	 *   Thin rhomb width: 2 * sin (D2R(18))
	 */
	*side= PENROSE_GAME_SIZE/2.0;
	switch(size_index) {
	case 0:		/* small */
		nfolds= 2;
		*side/= RATIO + 1.0 + 2.*sin(D2R(18));
		break;
	case 1:		/* medium */
		nfolds= 3;
		*side/= 2.*RATIO + 2. + 2.*sin(D2R(18));
		break;
	case 2:		/* normal */
		nfolds= 4;
		*side/= 3.*RATIO + 2. + 2.*sin(D2R(18));
	    g_message("nfolds: %d, side: %lf", nfolds, *side);
		break;
	case 3:		/* large */
		nfolds= 4;
		*side/= 3.*RATIO + 5. + 2.*2.*sin(D2R(18));
		break;
	case 4:		/* huge */
		nfolds= 5;
		*side/= 4.*RATIO + 5. + 2.*2.*sin(D2R(18));
		break;
	default:
		g_message("(penrose_calculate_params) unknown penrose size: %d", size_index);
	}
	return nfolds;
}


/*
 * Build a penrose tiling skeleton by unfolding two sets of rombs
 */
struct geometry*
build_penrose_tile_skeleton(const struct gameinfo *info)
{
	GSList *penrose=NULL;
	struct geometry *geo;
	double side;
	int nfolds;
	int i;
	double edge;
	int size_index=info->size;

	/* get side size and number of folds */
	nfolds= penrose_calculate_params(size_index, &side);

	/* Create the seed (increase size to account for foldings) */
	penrose= create_tile_seed(side*pow(RATIO, nfolds));

	/* unfold list of shapes */
	for(i=0; i < nfolds; ++i) {
		if (i == nfolds - 1) edge= PENROSE_GAME_SIZE/2.0;
		else if (i > 1 && i == nfolds - 2) edge= PENROSE_GAME_SIZE/1.5;
		else edge= PENROSE_GAME_SIZE;
		penrose= penrose_unfold(penrose, edge);
	}

	/* draw to file */
	//draw_penrose_tile(penrose);

	/* transform tile into geometry data (points, lines & tiles) */
	geo= penrose_tile_to_skeleton(penrose, side);

	/* debug: draw to file */
	//draw_penrose_tile(penrose);

	/* free penrose rhombs data */
	while (penrose != NULL) {
		g_free(penrose->data);
		penrose= g_slist_delete_link(penrose, penrose);
	}

	return geo;
}


/*
 * Build a penrose tiling by unfolding two sets of rombs
 */
struct geometry*
build_penrose_tile_geometry(const struct gameinfo *info)
{
	struct geometry *geo;

	/* build geometry skeleton (no connections) */
	geo= build_penrose_tile_skeleton(info);

	/* finalize geometry data: tie everything together */
	geometry_connect_skeleton(geo);

	/* define sizes of drawing bits */
	penrose_calculate_sizes(geo, info->size);

	return geo;
}


/*
 * Draw tile to png file (for debug purposes only)
 */
static void
draw_penrose_tile(GSList *penrose)
{
	const char filename[]="/home/jos/Desktop/penrose.png";
	const int width=500;
	const int height=500;
	cairo_surface_t *surf;
	cairo_t *cr;
	struct romb *romb;
	double x, y;

	surf= cairo_image_surface_create(CAIRO_FORMAT_RGB24, width, height);
	cr= cairo_create(surf);

	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_paint(cr);
	cairo_scale (cr, width/(double)PENROSE_BOARD_SIZE,
		     height/(double)PENROSE_BOARD_SIZE);
	cairo_set_line_width (cr, 1/500.0*PENROSE_BOARD_SIZE);
	cairo_set_source_rgb(cr, 0, 0, 0);

	while(penrose != NULL) {
		romb= (struct romb*)penrose->data;
		cairo_move_to(cr, romb->pos.x, romb->pos.y);
		if (romb->type == FAT_ROMB) {
			x= romb->pos.x + romb->side * cos(romb->angle - D2R(36));
			y= romb->pos.y + romb->side * sin(romb->angle - D2R(36));
			cairo_line_to(cr, x, y);
			x= romb->pos.x + romb->side*RATIO * cos(romb->angle);
			y= romb->pos.y + romb->side*RATIO * sin(romb->angle);
			cairo_line_to(cr, x, y);
			x= romb->pos.x + romb->side * cos(romb->angle + D2R(36));
			y= romb->pos.y + romb->side * sin(romb->angle + D2R(36));
			cairo_line_to(cr, x, y);
			cairo_line_to(cr, romb->pos.x, romb->pos.y);
		} else {
			x= romb->pos.x + romb->side * cos(romb->angle - D2R(18));
			y= romb->pos.y + romb->side * sin(romb->angle - D2R(18));
			cairo_line_to(cr, x, y);
			x= romb->pos.x + 2*romb->side*cos(D2R(18)) * cos(romb->angle);
			y= romb->pos.y + 2*romb->side*cos(D2R(18)) * sin(romb->angle);
			cairo_line_to(cr, x, y);
			x= romb->pos.x + romb->side * cos(romb->angle + D2R(18));
			y= romb->pos.y + romb->side * sin(romb->angle + D2R(18));
			cairo_line_to(cr, x, y);
			cairo_line_to(cr, romb->pos.x, romb->pos.y);
		}
		cairo_stroke(cr);
		penrose= g_slist_next(penrose);
	}

	cairo_destroy(cr);
	/* write png file */
	if (cairo_surface_write_to_png(surf, filename) != 0) {
		g_debug("draw_penrose_tile: error on write to png");
	}

}
