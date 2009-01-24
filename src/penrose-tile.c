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

#include "gamedata.h"


void draw_penrose_tile(GSList *penrose);

extern struct board board;


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
};


#define RATIO		1.6180339887
#define D2R(x)		((x)/180.0*M_PI)
#define WRAP(x)		x= ((x) > 2.*M_PI) ? (x) -  2.*M_PI : (x)


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
	newpenrose= g_slist_prepend(newpenrose, nromb);

	/* next romb 2/5 */
	nromb= (struct romb *)g_malloc(sizeof(struct romb));
	nromb->type= THIN_ROMB;
	nromb->pos.x= romb->pos.x + nside * cos(romb->angle);
	nromb->pos.y= romb->pos.y + nside * sin(romb->angle);
	nromb->side= nside;
	nromb->angle= romb->angle + D2R(360 - (36 + 18));
	WRAP(nromb->angle);
	newpenrose= g_slist_prepend(newpenrose, nromb);

	/* next romb 3/5 */
	nromb= (struct romb *)g_malloc(sizeof(struct romb));
	nromb->type= FAT_ROMB;
	nromb->pos.x= romb->pos.x + (nside + romb->side) * cos(romb->angle);
	nromb->pos.y= romb->pos.y + (nside + romb->side) * sin(romb->angle);
	nromb->side= nside;
	nromb->angle= romb->angle + D2R(180);
	WRAP(nromb->angle);
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
	newpenrose= g_slist_prepend(newpenrose, nromb);

	/* next romb 5/5 */
	nromb= (struct romb *)g_malloc(sizeof(struct romb));
	nromb->type= FAT_ROMB;
	nromb->pos.x= romb->pos.x + romb->side * cos(romb->angle + D2R(36));
	nromb->pos.y= romb->pos.y + romb->side * sin(romb->angle + D2R(36));
	nromb->side= nside;
	nromb->angle= romb->angle + D2R(180 + 36);
	WRAP(nromb->angle);
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
	newpenrose= g_slist_prepend(newpenrose, nromb);

	/* next romb 2/4 */
	nromb= (struct romb *)g_malloc(sizeof(struct romb));
	nromb->type= FAT_ROMB;
	nromb->pos.x= romb->pos.x + (2 * romb->side * cos(D2R(18))) * cos(romb->angle);
	nromb->pos.y= romb->pos.y + (2 * romb->side * cos(D2R(18))) * sin(romb->angle);
	nromb->side= nside;
	nromb->angle= romb->angle + D2R(180 + 18);
	WRAP(nromb->angle);
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
	newpenrose= g_slist_prepend(newpenrose, nromb);
	
	/* next romb 4/4 */
	nromb= (struct romb *)g_malloc(sizeof(struct romb));
	nromb->type= THIN_ROMB;
	nromb->pos.x= romb->pos.x + romb->side * cos(romb->angle - D2R(18));
	nromb->pos.y= romb->pos.y + romb->side * sin(romb->angle - D2R(18));
	nromb->side= nside;
	nromb->angle= romb->angle + D2R(90 + 18);
	WRAP(nromb->angle);
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
 * Calculate center point of romb
 */
static void
get_romb_center(struct romb *romb, struct point *center)
{
	if (romb->type == FAT_ROMB) {
		center->x= romb->pos.x + romb->side*RATIO/2.0*cos(romb->angle);
		center->y= romb->pos.y + romb->side*RATIO/2.0*sin(romb->angle);
	} else {
		center->x= romb->pos.x + romb->side*cos(D2R(18))*cos(romb->angle);
		center->y= romb->pos.y + romb->side*cos(D2R(18))*sin(romb->angle);
	}
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
	struct point center1, center2;
	double dist;
	
	current= penrose;
	while(current != NULL) {
		p= g_slist_next(current);
		r1= (struct romb*)current->data;
		get_romb_center(r1, &center1);
		/* iterate over rest of rombs */
		while(p != NULL) {
			r2= (struct romb*)p->data;
			if (r1->type != r2->type) { // not same type, next
				p= g_slist_next(p);
				continue;
			}
			get_romb_center(r2, &center2);
			/* calculate dist between centers of both rombs */
			center2.x-= center1.x;
			center2.y-= center1.y;
			dist= sqrt(center2.x*center2.x + center2.y*center2.y); 
			if (dist < separate_distance ) { // same romb
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
 * Unfold current list of rombs
 */
GSList*
penrose_unfold(GSList* penrose)
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
	newpenrose= trim_repeated_rombs(newpenrose);

	/* debug: count number of rombs in list */
	g_debug("rombs in list: %d", g_slist_length(newpenrose));
		
	return newpenrose;
}


/*
 * Return coordinates of vertices of romb
 */
void
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
 * Check if coordinates of vertex are unique (not in the list of previous vertices)
 */
gboolean
is_vertex_unique(struct point *v, struct dot* dots, int ndots)
{
	int i;
	double x, y;
	
	for(i=0; i < ndots; ++i) {
		x= v->x - dots[i].pos.x;
		y= v->y - dots[i].pos.y;
		if ( sqrt(x*x + y*y) < separate_distance )
			return FALSE;
	}
	
	return TRUE;
}


/*
 * Find vertex on dot list by vertex coordinates
 */
struct dot*
find_dot_by_coords(struct point *v, struct game *game)
{
	int i;
	double x, y;
	
	for(i=0; i < game->ndots; ++i) {
		x= v->x - game->dots[i].pos.x;
		y= v->y - game->dots[i].pos.y;
		if ( sqrt(x*x + y*y) < separate_distance )
			return game->dots + i;
	}
	
	g_debug("find_dot_by_coords: vertex not found on dot list");
	return NULL;
}


/*
 * Square has vertices already set. Now set lines.
 */
void
set_lines_on_square(struct square *sq, struct game *game)
{
	struct dot *d1;
	struct dot *d2;
	struct line *lin;
	int i, j;
	double x, y;
	
	g_debug("set lines in square %d", sq->id);
	for(i=0; i < 4; ++i) {
		j= (i + 1) % 4;
		d1= sq->vertex[i];
		d2= sq->vertex[j];
		
		/* align points in line so first point is closest to origin */
		if ( (d1->pos.x*d1->pos.x + d1->pos.y*d1->pos.y) > 
		    (d2->pos.x*d2->pos.x + d2->pos.y*d2->pos.y) ) {
			d2= sq->vertex[i];
			d1= sq->vertex[j];
		}
		
		/* look for line in line list */
		for(j=0; j < game->nlines; ++j) {
			x= d1->pos.x - game->lines[j].ends[0]->pos.x;
			y= d1->pos.y - game->lines[j].ends[0]->pos.y;
			if ( sqrt(x*x + y*y) < separate_distance ) {
				/* if top left point matches: try bottom right */
				x= d2->pos.x - game->lines[j].ends[1]->pos.x;
				y= d2->pos.y - game->lines[j].ends[1]->pos.y;
				if ( sqrt(x*x + y*y) < separate_distance )
					break;	// line found!
			}
		}
		
		/* set square and line fields accordingly */
		if (j < game->nlines) { // line found
			lin= game->lines + j;
			sq->sides[i]= lin;
			if (lin->nsquares == 0) {
				lin->sq[0]= sq;
				++lin->nsquares;
			} else if (lin->nsquares == 1 && lin->sq[0] != sq) {	
				// one was already assigned -> add if different
				lin->sq[1]= sq;
				lin->nsquares= 2;
			}
		} else {		// line NOT found, add it
			lin= game->lines + game->nlines;
			sq->sides[i]= lin;
			lin->id= game->nlines;
			lin->ends[0]= d1;
			lin->ends[1]= d2;
			lin->sq[0]= sq;
			++game->nlines;
		}
	}
	g_debug("-set lines in square %d", sq->id);
}


/*
 * Transform list of rombs to game data
 */
static struct game*
penrose_tile_to_game(GSList *penrose)
{
	struct game *game;
	struct dot *dot;
	struct line *lin;
	struct square *sq;
	int ndots;
	GSList *list;
	struct point vertex[4];
	int i, j;
	
	/* Allocate memory for game data */
	game= (struct game*)g_malloc(sizeof(struct game));
	game->nsquares= g_slist_length(penrose);
	game->ndots= game->nsquares*4.0; // over count (undo at the end)
	game->squares= (struct square*)g_malloc(game->nsquares*sizeof(struct square));
	game->dots= (struct dot*)g_malloc(game->ndots*sizeof(struct dot));
	
	/* Compile vertices (and count how many) */
	ndots= 0;
	list= penrose;
	dot= game->dots;
	while(list != NULL) {
		get_romb_vertices((struct romb*)list->data, vertex);
		for(i=0; i < 4; ++i) {
			if (is_vertex_unique(&vertex[i], game->dots, ndots)) {
				dot->id= ndots;
				dot->nlines= 0; // value will be set later
				dot->lines= NULL;
				dot->pos.x= vertex[i].x;
				dot->pos.y= vertex[i].y;
				++ndots;
				++dot;
			}
		}
		list= g_slist_next(list);
	}
	
	printf("ndots: %d\n", ndots);
	
	/* change to actual number of dots */
	game->ndots= ndots;
	game->dots= g_realloc(game->dots, ndots*sizeof(struct dot));
	
	/* allocate space for lines */
	game->nlines= game->nsquares*4; // assume same number of lines as dots
	game->lines= (struct line*)g_malloc(game->nlines*sizeof(struct line));
	
	/* initialize lines */
	lin= game->lines;
	for (i= 0; i < game->nlines; ++i) {
		lin->id= i;
		lin->nsquares= 0;
		lin->fx_status= 0;
		lin->fx_frame= 0;
		++lin;
	}
	
	printf("nlines guess: %d\n", game->nlines);
	
	/* initialize squares (rombs) */
	list= penrose;
	sq= game->squares;
	game->nlines= 0;		// we'll add them as we find them
	for(i=0; i < game->nsquares; ++i) {
		g_debug("square %d (%d)", i, (int)sq);
		sq->id= i;
		sq->number= -1;	// all squares are initialized empty
		sq->nvertex= 4;
		sq->vertex= (struct dot **)g_malloc(4 * sizeof(void*));
		
		/* set square (romb) vertex pointers */
		get_romb_vertices((struct romb*)list->data, vertex);
		for(j=0; j < 4; ++j) {
			sq->vertex[j]= find_dot_by_coords(&vertex[j], game);
		}
		
		/* calculate position of center of square (romb) */
		get_romb_center((struct romb*)list->data, &sq->center);
		
		/* set lines on edges of square (romb) */
		sq->nsides= 4;
		sq->sides= (struct line **)g_malloc(4 * sizeof(void *));
		set_lines_on_square(sq, game);
		
		/* ini FX status */
		sq->fx_status= 0;
		sq->fx_frame= 0;
			
		++sq;	
		list= g_slist_next(list);
		g_debug("-square %d", i);
	}
	
	/* change to actual number of lines */
	game->lines= g_realloc(game->lines, game->nlines*sizeof(struct line));
	
	printf("nsquares: %d (%d)\n", game->nsquares, 4*game->nsquares);
	printf("ndots: %d\n", game->ndots);
	printf("nlines actual: %d\n", game->nlines);
	
	return game;
}


/*
 * Build a penrose tiling by 
 */ 
struct game*
build_penrose_tiling(void)
{
	GSList *penrose=NULL;
	struct romb *romb;
	struct game *game;

	/* starting shape */
	romb= (struct romb*)g_malloc(sizeof(struct romb));
	romb->type= THIN_ROMB;	/**NOTE** thin romb is not unfolding well */
	romb->pos.x= board.board_size/6.;
	romb->pos.y= board.board_size/2.;
	romb->side= board.board_size/2.5;
	romb->angle= 0.;

	/* create initial list of shapes (just a big one) */
	penrose= g_slist_prepend(penrose, romb);

	//draw_penrose_tile(penrose);
	
	/* unfold list of shapes */
	penrose= penrose_unfold(penrose);
	//penrose= penrose_unfold(penrose);
	//penrose= penrose_unfold(penrose);
	//penrose= penrose_unfold(penrose);
	//penrose= penrose_unfold(penrose);

	/* clip rombs to make tile round */
	/***TODO***/
	
	/* transform tile into game data (points, lines & squares) */
	game= penrose_tile_to_game(penrose);
	
	
	
	/* debug: draw to file */
	//draw_penrose_tile(penrose);
		
	/* free data */
	while (penrose != NULL) {
		g_free(penrose->data);
		penrose= g_slist_delete_link(penrose, penrose);
	}
	
	//exit(1);
	
	return game;
}


/*
 * Draw tile to png file (for debug purposes only)
 */
void
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
	cairo_scale (cr, width/(double)board.board_size, 
		     height/(double)board.board_size);
	cairo_set_line_width (cr, 1/500.0*board.board_size);
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
