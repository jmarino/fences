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



/*
 * Unfold a fat romb
 * Add new rombs to list newpenrose and return it
 */ 
GSList*
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
GSList*
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
	/***TODO***/

	return newpenrose;
}



/*
 * Build a penrose tiling by 
 */ 
void
build_penrose_tiling(void)
{
	GSList *penrose=NULL;
	struct romb *start;

	/* starting shape */
	start= (struct romb*)g_malloc(sizeof(struct romb));
	start->type= THIN_ROMB;	/**NOTE** thin romb is not unfolding well */
	start->pos.x= board.board_size/6.;
	start->pos.y= board.board_size/2.;
	start->side= board.board_size/2.5;
	start->angle= 0.;

	/* create initial list of shapes (just a big one) */
	penrose= g_slist_prepend(penrose, start);

	//draw_penrose_tile(penrose);
	
	/* unfold list of shapes */
	penrose= penrose_unfold(penrose);
	penrose= penrose_unfold(penrose); 
	penrose= penrose_unfold(penrose); 
	/*penrose= penrose_unfold(penrose);
	penrose= penrose_unfold(penrose);*/
	
	draw_penrose_tile(penrose);
	
	/* free data */
	while (penrose != NULL) {
		g_free(penrose->data);
		penrose= g_slist_delete_link(penrose, penrose);
	}
}


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
