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


/* prefered board dimensions for cartwheel tile */
#define CARTWHEEL_BOARD_SIZE	100.
#define CARTWHEEL_BOARD_MARGIN	5.
#define CARTWHEEL_GAME_SIZE	(CARTWHEEL_BOARD_SIZE - 2*CARTWHEEL_BOARD_MARGIN)


/* Two types of kites */
enum {
	KITE,
	DART
};

// angle increases clockwise
struct kite {
	int type;			// kite or dart
	struct point pos;	// coords of kite/dart tip
	double side;		// length of side
	double angle;		// angle of kite (0=horizontal/right; pi/2: down)
	struct point center;
};

// store parameters to be used to generate puzzle tile
struct puzzle_params {
	double side;
	double seed_side;
	int nfolds;
	int seed_type;
	struct point pos;
};


#define RATIO		1.6180339887
#define D2R(x)		((x)/180.0*M_PI)
#define WRAP(x)		x= ((x) > 2.*M_PI) ? (x) -  2.*M_PI : (x)



static void draw_cartwheel_tile(GSList *cartwheel);
static void get_kite_vertices(struct kite *kite, struct point *vertex);


/* contains minimum distance required to consider two vertices different */
static double separate_distance=0.;



/*
 * Unfold a kite
 * Add new kites & darts to list newcartwheel and return it
 */
static GSList*
cartwheel_unfold_kite(GSList *newcartwheel, struct kite *kite)
{
	struct kite *nkite;
	double nside=kite->side/RATIO;
	double middle=nside*cos(D2R(36));

	g_assert(kite->type == KITE);

	/* create new dart 1/6 (at tip) */
	nkite= (struct kite *)g_malloc(sizeof(struct kite));
	nkite->type= DART;
	nkite->pos.x= kite->pos.x;
	nkite->pos.y= kite->pos.y;
	nkite->side= nside;
	nkite->angle= kite->angle - D2R(36);
	WRAP(nkite->angle);
	nkite->center.x= nkite->pos.x + middle/2.0 * cos(nkite->angle);
	nkite->center.y= nkite->pos.y + middle/2.0 * sin(nkite->angle);
	newcartwheel= g_slist_prepend(newcartwheel, nkite);

	/* next dart kite 2/6 (at tip) */
	nkite= (struct kite *)g_malloc(sizeof(struct kite));
	nkite->type= DART;
	nkite->pos.x= kite->pos.x;
	nkite->pos.y= kite->pos.y;
	nkite->side= nside;
	nkite->angle= kite->angle + D2R(36);
	WRAP(nkite->angle);
	nkite->center.x= nkite->pos.x + middle/2.0 * cos(nkite->angle);
	nkite->center.y= nkite->pos.y + middle/2.0 * sin(nkite->angle);
	newcartwheel= g_slist_prepend(newcartwheel, nkite);

	/* next kite (top) 3/6 */
	nkite= (struct kite *)g_malloc(sizeof(struct kite));
	nkite->type= KITE;
	nkite->pos.x= kite->pos.x + kite->side * cos(kite->angle - D2R(36));
	nkite->pos.y= kite->pos.y + kite->side * sin(kite->angle - D2R(36));
	nkite->side= nside;
	nkite->angle= kite->angle + D2R(90 + (90 - 72));
	WRAP(nkite->angle);
	nkite->center.x= nkite->pos.x + middle*3.0/4.0 * cos(nkite->angle);
	nkite->center.y= nkite->pos.y + middle*3.0/4.0 * sin(nkite->angle);
	newcartwheel= g_slist_prepend(newcartwheel, nkite);

	/* next kite (top) 4/6 */
	nkite= (struct kite *)g_malloc(sizeof(struct kite));
	nkite->type= KITE;
	nkite->pos.x= kite->pos.x + kite->side * cos(kite->angle - D2R(36));
	nkite->pos.y= kite->pos.y + kite->side * sin(kite->angle - D2R(36));
	nkite->side= nside;
	nkite->angle= kite->angle + D2R(180);
	WRAP(nkite->angle);
	nkite->center.x= nkite->pos.x + middle*3.0/4.0 * cos(nkite->angle);
	nkite->center.y= nkite->pos.y + middle*3.0/4.0 * sin(nkite->angle);
	newcartwheel= g_slist_prepend(newcartwheel, nkite);

	/* next kite (bottom) 5/6 */
	nkite= (struct kite *)g_malloc(sizeof(struct kite));
	nkite->type= KITE;
	nkite->pos.x= kite->pos.x + kite->side * cos(kite->angle + D2R(36));
	nkite->pos.y= kite->pos.y + kite->side * sin(kite->angle + D2R(36));
	nkite->side= nside;
	nkite->angle= kite->angle - D2R(90 + (90 - 72));
	WRAP(nkite->angle);
	nkite->center.x= nkite->pos.x + middle*3.0/4.0 * cos(nkite->angle);
	nkite->center.y= nkite->pos.y + middle*3.0/4.0 * sin(nkite->angle);
	newcartwheel= g_slist_prepend(newcartwheel, nkite);

	/* next kite (bottom) 6/6 */
	nkite= (struct kite *)g_malloc(sizeof(struct kite));
	nkite->type= KITE;
	nkite->pos.x= kite->pos.x + kite->side * cos(kite->angle + D2R(36));
	nkite->pos.y= kite->pos.y + kite->side * sin(kite->angle + D2R(36));
	nkite->side= nside;
	nkite->angle= kite->angle - D2R(180);
	WRAP(nkite->angle);
	nkite->center.x= nkite->pos.x + middle*3.0/4.0 * cos(nkite->angle);
	nkite->center.y= nkite->pos.y + middle*3.0/4.0 * sin(nkite->angle);
	newcartwheel= g_slist_prepend(newcartwheel, nkite);

	return newcartwheel;
}


/*
 * Unfold a dart
 * Add new kites &darts to list newcartwheel and return it
 */
static GSList*
cartwheel_unfold_dart(GSList *newcartwheel, struct kite *dart)
{
	struct kite *nkite;
	double nside=dart->side/RATIO;
	double middle=nside*cos(D2R(36));

	g_assert(dart->type == DART);

	/* create new kite 1/5 (at tip) */
	nkite= (struct kite *)g_malloc(sizeof(struct kite));
	nkite->type= KITE;
	nkite->pos.x= dart->pos.x;
	nkite->pos.y= dart->pos.y;
	nkite->side= nside;
	nkite->angle= dart->angle;
	//WRAP(nkite->angle);
	nkite->center.x= nkite->pos.x + middle*3.0/4.0 * cos(nkite->angle);
	nkite->center.y= nkite->pos.y + middle*3.0/4.0 * sin(nkite->angle);
	newcartwheel= g_slist_prepend(newcartwheel, nkite);

	/* create new kite 2/5 (at tip) */
	nkite= (struct kite *)g_malloc(sizeof(struct kite));
	nkite->type= KITE;
	nkite->pos.x= dart->pos.x;
	nkite->pos.y= dart->pos.y;
	nkite->side= nside;
	nkite->angle= dart->angle - D2R(72);
	WRAP(nkite->angle);
	nkite->center.x= nkite->pos.x + middle*3.0/4.0 * cos(nkite->angle);
	nkite->center.y= nkite->pos.y + middle*3.0/4.0 * sin(nkite->angle);
	newcartwheel= g_slist_prepend(newcartwheel, nkite);

	/* create new kite 3/5 (at tip) */
	nkite= (struct kite *)g_malloc(sizeof(struct kite));
	nkite->type= KITE;
	nkite->pos.x= dart->pos.x;
	nkite->pos.y= dart->pos.y;
	nkite->side= nside;
	nkite->angle= dart->angle + D2R(72);
	WRAP(nkite->angle);
	nkite->center.x= nkite->pos.x + middle*3.0/4.0 * cos(nkite->angle);
	nkite->center.y= nkite->pos.y + middle*3.0/4.0 * sin(nkite->angle);
	newcartwheel= g_slist_prepend(newcartwheel, nkite);

	/* next dart 4/5 */
	nkite= (struct kite *)g_malloc(sizeof(struct kite));
	nkite->type= DART;
	nkite->pos.x= dart->pos.x + dart->side * cos(dart->angle - D2R(36));
	nkite->pos.y= dart->pos.y + dart->side * sin(dart->angle - D2R(36));
	nkite->side= nside;
	nkite->angle= dart->angle + D2R(90 + (90 - 36));
	WRAP(nkite->angle);
	nkite->center.x= nkite->pos.x + middle/2.0 * cos(nkite->angle);
	nkite->center.y= nkite->pos.y + middle/2.0 * sin(nkite->angle);
	newcartwheel= g_slist_prepend(newcartwheel, nkite);

	/* next dart 5/5 */
	nkite= (struct kite *)g_malloc(sizeof(struct kite));
	nkite->type= DART;
	nkite->pos.x= dart->pos.x + dart->side * cos(dart->angle + D2R(36));
	nkite->pos.y= dart->pos.y + dart->side * sin(dart->angle + D2R(36));
	nkite->side= nside;
	nkite->angle= dart->angle - D2R(90 + (90 - 36));
	WRAP(nkite->angle);
	nkite->center.x= nkite->pos.x + middle/2.0 * cos(nkite->angle);
	nkite->center.y= nkite->pos.y + middle/2.0 * sin(nkite->angle);
	newcartwheel= g_slist_prepend(newcartwheel, nkite);

	return newcartwheel;
}


/*
 * Eliminate repeated kites & darts in the list
 * Returns new trimmed list
 */
GSList *
trim_repeated_kites(GSList *cartwheel)
{
	GSList *current;
	GSList *next;
	GSList *p;
	struct kite *r1, *r2;
	struct point c;

	current= cartwheel;
	while(current != NULL) {
		p= g_slist_next(current);
		r1= (struct kite*)current->data;
		/* iterate over rest of kites */
		while(p != NULL) {
			r2= (struct kite*)p->data;
			if (r1->type != r2->type) { // not same type, next
				p= g_slist_next(p);
				continue;
			}
			/* calculate dist between centers of both kites */
			c.x= r2->center.x - r1->center.x;
			c.y= r2->center.y - r1->center.y;
			if ((c.x*c.x + c.y*c.y) < separate_distance ) { // same kite
				//g_debug("delete kite, dist: %lf < %lf", dist, r1->side/10.);
				g_free(p->data);
				next= g_slist_next(p);
				cartwheel= g_slist_delete_link(cartwheel, p);
				p= next;
			} else {
				p= g_slist_next(p);
			}
		}
		current= g_slist_next(current);
	}
	return cartwheel;
}


/*
 * Eliminate kites outside a certain radius
 * Returns new trimmed list
 */
static GSList *
trim_outside_kites(GSList *cartwheel, double radius)
{
	GSList *current;
	GSList *next;
	struct kite *kite;
	struct point vertex[4];
	double dist;
	int i;
	double center=CARTWHEEL_BOARD_SIZE/2.;

	current= cartwheel;
	while(current != NULL) {
		kite= (struct kite*) current->data;

		get_kite_vertices(kite, vertex);
		next= g_slist_next(current);
		for(i=0; i < 4; ++i) {
			vertex[i].x-= center;
			vertex[i].y-= center;
			dist= sqrt(vertex[i].x*vertex[i].x + vertex[i].y*vertex[i].y);
			if (dist > radius) {
				g_free(current->data);
				cartwheel= g_slist_delete_link(cartwheel, current);
				break;
			}
		}
		current= next;
	}
	return cartwheel;
}


/*
 * Unfold current list of darts
 */
static GSList*
cartwheel_unfold(GSList* cartwheel, double edge)
{
	GSList *newcartwheel=NULL;
	struct kite *kite;

	while(cartwheel != NULL) {
		kite= (struct kite*)cartwheel->data;
		switch(kite->type) {
		case KITE:
			newcartwheel= cartwheel_unfold_kite(newcartwheel, kite);
			break;
		case DART:
			newcartwheel= cartwheel_unfold_dart(newcartwheel, kite);
			break;
		default:
			g_debug("cartwheel_unfold: unknown kite type: %d", kite->type);
		}
		g_free(kite);
		cartwheel= g_slist_delete_link(cartwheel, cartwheel);
	}

	/* get rid of repeated kites */
	separate_distance= ((struct kite*)newcartwheel->data)->side/10.;
	separate_distance*= separate_distance;
	newcartwheel= trim_repeated_kites(newcartwheel);

	/* get rid of kites outside a certain radius */
	if (edge > 0)
		newcartwheel= trim_outside_kites(newcartwheel, edge);

	/* debug: count number of kites in list */
	g_debug("kites in list: %d", g_slist_length(newcartwheel));

	return newcartwheel;
}


/*
 * Return coordinates of vertices of kite
 */
static void
get_kite_vertices(struct kite *kite, struct point *vertex)
{
	double length;

	vertex[0].x= kite->pos.x;
	vertex[0].y= kite->pos.y;
	vertex[1].x= kite->pos.x + kite->side*cos(kite->angle - D2R(36));
	vertex[1].y= kite->pos.y + kite->side*sin(kite->angle - D2R(36));
	vertex[3].x= kite->pos.x + kite->side*cos(kite->angle + D2R(36));
	vertex[3].y= kite->pos.y + kite->side*sin(kite->angle + D2R(36));
	if (kite->type == KITE) {
		length= kite->side*cos(D2R(36)) + kite->side/RATIO*sin(D2R(18));
	} else {		// DART
		length= kite->side*cos(D2R(36)) - kite->side/RATIO*sin(D2R(18));
	}
	vertex[2].x= kite->pos.x + length * cos(kite->angle);
	vertex[2].y= kite->pos.y + length * sin(kite->angle);
}


/*
 * Calculate sizes of drawing details
 */
static void
cartwheel_calculate_sizes(struct geometry *geo)
{
	geo->on_line_width= geo->board_size/250.;
	geo->off_line_width= geo->board_size/1000.;
	geo->cross_line_width= geo->off_line_width*1.5;
	geo->cross_radius= MIN(geo->tile_width, geo->tile_height)/10.;
	geo->font_scale= 0.8;
}


/*
 * Transform list of darts to geometry data
 */
static struct geometry*
cartwheel_tile_to_geometry(GSList *cartwheel, double side)
{
	struct geometry *geo;
	GSList *list;
	struct point pts[4];
	int i;
	int ntiles;
	int nvertex;
	int nlines;
	struct kite *kite;

	/* create new geometry (ntiles, nvertex, nlines) */
	/* NOTE: oversize nvertex and nlines. Will adjust below
	   Oversize factors determined by trial and error. */
	ntiles= g_slist_length(cartwheel);
	nvertex= ntiles*4;//(int)(ntiles*1.5);
	nlines= ntiles*4;//(int)(ntiles*2.5);
	geo= geometry_create_new(ntiles, nvertex, nlines, 4);
	geo->board_size= CARTWHEEL_BOARD_SIZE;
	geo->board_margin= CARTWHEEL_BOARD_MARGIN;
	geo->game_size= geo->board_size - 2*geo->board_margin;
	geometry_set_distance_resolution(side/10.0);


	/* iterate through tiles creating skeleton geometry
	   (skeleton geometry: lines hold all the topology info) */
	geo->ntiles= 0;
	geo->nlines= 0;
	geo->nvertex= 0;
	list= cartwheel;
	for(i=0; i < ntiles; ++i) {
		/* get vertices of tile (rhomb) and add it to skeleton geometry */
		kite= (struct kite*)list->data;
		get_kite_vertices((struct kite*)list->data, pts);
		geometry_add_tile(geo, pts, 4, &kite->center);

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

	/* finalize geometry data: tie everything together */
	geometry_connect_skeleton(geo);

	/* define sizes of drawing bits */
	cartwheel_calculate_sizes(geo);

	return geo;
}


/*
 * Create arrow seed.
 * at POS, with SIDE and at ANGLE.
 */
static GSList*
create_arrow_seed(struct point *pos, double angle, double side)
{
	GSList *cartwheel=NULL;
	struct kite *kite;

	/* dart on tip */
	kite= (struct kite*)g_malloc(sizeof(struct kite));
	kite->type= DART;
	kite->side= side;
	kite->angle= D2R(angle);
	kite->pos.x= pos->x;
	kite->pos.y= pos->y;
	cartwheel= g_slist_prepend(cartwheel, kite);

	/* top kite */
	kite= (struct kite*)g_malloc(sizeof(struct kite));
	kite->type= KITE;
	kite->side= side;
	kite->angle= D2R(angle + 180 + 36);
	kite->pos.x= pos->x + (side + side/RATIO);
	kite->pos.y= pos->y;
	cartwheel= g_slist_prepend(cartwheel, kite);

	/* bottom kite */
	kite= (struct kite*)g_malloc(sizeof(struct kite));
	kite->type= KITE;
	kite->side= side;
	kite->angle= D2R(angle + 180 - 36);
	kite->pos.x= pos->x + (side + side/RATIO);
	kite->pos.y= pos->y;
	cartwheel= g_slist_prepend(cartwheel, kite);

	return cartwheel;
}


/*
 * Define seed to generate cartwheel tile
 * Input 'side' is size of initial kites and darts size
 */
static GSList*
create_tile_seed(struct puzzle_params *params, int size_index)
{
	GSList *cartwheel=NULL;
	struct kite *kite;
	struct point pos;
	int i;

	pos.x= params->pos.x;
	pos.y= params->pos.y;

	switch (size_index) {
	case 0:
		cartwheel= create_arrow_seed(&pos, 0.0, params->seed_side);
		break;
	case 1:
	case 2:
	case 3:
	case 4:
		for(i=0; i < 5; ++i) {
			kite= (struct kite*)g_malloc(sizeof(struct kite));
			kite->type= params->seed_type;
			kite->side= params->seed_side;
			kite->angle= D2R(i*72-90);
			kite->pos.x= pos.x;
			kite->pos.y= pos.y;
			cartwheel= g_slist_prepend(cartwheel, kite);
		}
		break;
	default:
		g_message("(create_tile_seed) unknown size_index %d", size_index);
	}

	return cartwheel;
}


/*
 * Compute side and number of folds to produce a cartwheel
 * of the requested size index
 * Returns: number of folds
 *  side size is passed by pointer in '*side'
 */
static void
cartwheel_calculate_params(int size_index, struct puzzle_params *params)
{
	/*
	 * radius of cartwheel tiling must be game_size/2
	 *  game_size/2 = side * Factor
	 * Factor is number of sides that fit in radius.
	 *   Both kite & dart have 2 LONG and 2 SHORT sides
	 *      LONG= 1   ;  SHORT= 1.0/RATIO
	 *   Height of dart is == SHORT
	 *   Heigth of kite is == LONG
	 */
	params->side= CARTWHEEL_GAME_SIZE/2.0;
	params->pos.x= CARTWHEEL_BOARD_SIZE/2.;
	params->pos.y= CARTWHEEL_BOARD_SIZE/2.;
	params->seed_type= KITE;	// only useful for some sizes
	switch(size_index) {
	case 0:		/* small */
		params->nfolds= 2;
		params->side/= (4 + 2.0/RATIO)/2.0;
		params->pos.x-= params->side*pow(RATIO, params->nfolds);
		break;
	case 1:		/* medium */
		params->nfolds= 3;
		params->side/= 2.0 + 2.0/RATIO;
		params->seed_type= KITE;
		//params->pos.x+= 2.5*params->side;
		//params->pos.y-= params->side*13.0/16.0;
		break;
	case 2:		/* normal */
		params->nfolds= 3;
		params->side/= 3.0 + 2.0/RATIO;
		params->seed_type= DART;
		break;
	case 3:		/* large */
		params->nfolds= 4;
		params->side/= 4.0 + 3.0/RATIO + 1.0/RATIO/2.0;
		params->seed_type= DART;
		break;
	case 4:		/* huge */
		params->nfolds= 4;
		params->side/= 6.0 + 5.0/RATIO + 1.0/RATIO/2.0;
		params->seed_type= KITE;
		break;
	default:
		g_message("(cartwheel_calculate_params) unknown cartwheel size: %d", size_index);
	}
	/* calculate side of big seed */
	params->seed_side= params->side*pow(RATIO, params->nfolds);
}


/*
 * Build a cartwheel tiling by unfolding kites and darts
 */
struct geometry*
build_cartwheel_tile_geometry(const struct gameinfo *info)
{
	GSList *cartwheel=NULL;
	struct geometry *geo;
	int i;
	double edge;
	int size_index=info->size;
	struct puzzle_params params;

	/* get side size and number of folds */
	cartwheel_calculate_params(size_index, &params);

	/* Create the seed (increase size to account for foldings) */
	cartwheel= create_tile_seed(&params, size_index);

	/* unfold list of shapes */
	for(i=0; i < params.nfolds; ++i) {
		if (i == params.nfolds - 1) edge= CARTWHEEL_GAME_SIZE/2.0;
		else if (i > 1 && i == params.nfolds - 2) edge= CARTWHEEL_GAME_SIZE/1.5;
		else edge= CARTWHEEL_GAME_SIZE;
		cartwheel= cartwheel_unfold(cartwheel, edge);
	}

	/* draw to file */
	//draw_cartwheel_tile(cartwheel);

	/* transform tile into geometry data (points, lines & tiles) */
	geo= cartwheel_tile_to_geometry(cartwheel, params.side);

	/* free data */
	while (cartwheel != NULL) {
		g_free(cartwheel->data);
		cartwheel= g_slist_delete_link(cartwheel, cartwheel);
	}

	return geo;
}


/*
 * Draw tile to png file (for debug purposes only)
 */
static void
draw_cartwheel_tile(GSList *cartwheel)
{
	const char filename[]="cartwheel.png";
	const int width=500;
	const int height=500;
	cairo_surface_t *surf;
	cairo_t *cr;
	struct kite *kite;
	struct point pts[4];
	int i;

	surf= cairo_image_surface_create(CAIRO_FORMAT_RGB24, width, height);
	cr= cairo_create(surf);

	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_paint(cr);
	cairo_scale (cr, width/(double)CARTWHEEL_BOARD_SIZE,
		     height/(double)CARTWHEEL_BOARD_SIZE);
	cairo_set_line_width (cr, 1/500.0*CARTWHEEL_BOARD_SIZE);
	cairo_set_source_rgb(cr, 0, 0, 0);

	while(cartwheel != NULL) {
		kite= (struct kite*)cartwheel->data;
		get_kite_vertices(kite, pts);
		cairo_move_to(cr, pts[0].x, pts[0].y);
		for(i= 1; i < 4 ; ++i) {
			cairo_line_to(cr, pts[i].x, pts[i].y);
		}
		cairo_line_to(cr, pts[0].x, pts[0].y);
		cairo_stroke(cr);
		cartwheel= g_slist_next(cartwheel);
	}

	cairo_destroy(cr);
	/* write png file */
	if (cairo_surface_write_to_png(surf, filename) != 0) {
		g_debug("draw_cartwheel_tile: error on write to png");
	}
	printf("File %s saved to disk.\n", filename);

}
