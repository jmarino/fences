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

#include <gtk/gtk.h>
#include <math.h>
#include <stdlib.h>

#include "gamedata.h"
#include "draw_thread.h"


/* defined in gamedata.c */
extern struct board board;


static GdkPixmap *pixmap = NULL;	// pixmap where thread draws


/*
 * Hack to display tile cache as a green grid
 */
static void
draw_tiles(cairo_t *cr)
{
	int i, j;
	int width=board.tile_cache->tile_size;
	
	cairo_set_source_rgb(cr, 0., 150/256., 0.);
	cairo_set_line_width (cr, 10);
	for(i=0; i < 10; ++i) {
		for(j=0; j < 10; ++j) {
			cairo_rectangle (cr, i*width, j*width, width, width);
			cairo_stroke(cr);
		}
	}
}


/*
 * Select color according to FX status and frame
 */
static void
fx_setcolor(cairo_t *cr, struct line *line)
{
	switch(line->fx_status) {
		case 0: //FX_OFF:
			cairo_set_source_rgb(cr, 0., 0., 1.);
		break;
		case 1://FX_LOOP:
			cairo_set_source_rgb(cr,  
					     0.2 + 0.8*sin(line->fx_frame/20.0*M_PI),
					     0., 1.); 
		break;
		default:
			g_debug("line %d, unknown FX: %d", line->id, line->fx_status);
	}
}


/*
 * Increase frame number for FX animation
 */
static void
fx_nextframe(struct line *line)
{
	switch(line->fx_status) {
		case 0: //FX_OFF 
			return;
		break;
		case 1://FX_LOOP:
			gdk_threads_enter();
			line->fx_frame= (line->fx_frame + 1)%20;
			gdk_threads_leave();
		break;
		default:
			g_debug("line %d, unknown FX: %d", line->id, line->fx_status);
	}
}


/*
 * Draw game on board
 */
void
draw_board(cairo_t *cr, int width, int height)
{
	struct dot *dot1, *dot2;
	struct line *line;
	struct square *sq;
	int i, j;
	double x, y;
	cairo_text_extents_t extent[4];
	const char *nums[]={"0", "1", "2", "3"};
	double font_scale;
	struct game *game=board.game;
	int lines_on;	// how many ON lines a dot has
	
	/* white background */
	cairo_set_source_rgb(cr, 1, 1, 1);
	//cairo_rectangle (cr, 0, 0, width, height);
	cairo_rectangle (cr, 0, 0, board.board_size, board.board_size);
	cairo_fill(cr);
	//cairo_paint(cr);
	
	//printf("%d, %d\n", w, h);
	//cairo_scale (cr, width/(double)board.board_size, 
	//	     height/(double)board.board_size);
	//debug
	//draw_tiles(cr);

	/* Draw lines */
	double dash[]={50};
	cairo_set_source_rgb(cr, 150/256., 150/256., 150/256.);
	cairo_set_line_width (cr, 10);
	cairo_set_dash(cr, dash, 1, 100);
	line= game->lines;
	for(i=0; i<game->nlines; ++i) {
		dot1= line->ends[0];
		dot2= line->ends[1];
		if (line->state == LINE_OFF || line->state == LINE_CROSSED) {
			cairo_set_source_rgb(cr, 150/256., 150/256., 150/256.);
			cairo_set_line_width (cr, 10);
			cairo_set_dash(cr, dash, 1, 100);
			cairo_move_to(cr, dot1->pos.x, dot1->pos.y);
			cairo_line_to(cr, dot2->pos.x, dot2->pos.y);
			cairo_stroke(cr);
			if (line->state == LINE_CROSSED) { // draw cross
				cairo_set_source_rgb(cr, 1., 0., 0.);
				cairo_set_line_width (cr, 50);
				cairo_set_dash(cr, dash, 0, 100);
				x= (dot1->pos.x + dot2->pos.x)/2.;
				y= (dot1->pos.y + dot2->pos.y)/2.;
				cairo_move_to(cr, x-120, y-120);
				cairo_line_to(cr, x+120, y+120);
				cairo_move_to(cr, x-120, y+120);
				cairo_line_to(cr, x+120, y-120);
				cairo_stroke(cr);
			}
		} else if (line->state == LINE_ON) {
			fx_setcolor(cr, line);
			//cairo_set_source_rgb(cr, 0., 0., 1.);
			cairo_set_line_width (cr, 100);
			cairo_set_dash(cr, dash, 0, 100);
			cairo_move_to(cr, dot1->pos.x, dot1->pos.y);
			cairo_line_to(cr, dot2->pos.x, dot2->pos.y);
			cairo_stroke(cr);
			//fx_nextframe(line);
		} else {
			g_debug("draw_line: line (%d) state invalid: %d", line->id, line->state);
		}
		++line;
	}
	//cairo_stroke(cr);
	
	/* Draw dots */
	dot1= game->dots;	
	for(i=0; i<game->ndots; ++i) {
		/* count how many lines on are touching this */
		lines_on= 0;
		for(j=0; j < dot1->nlines; ++j) {
			if (dot1->lines[j]->state == LINE_ON)
				++lines_on;
		}
		/* draw dot */
		if (lines_on == 2) cairo_set_source_rgb(cr, 0, 0, 1);
		else cairo_set_source_rgb(cr, 0, 0, 0);
		cairo_arc (cr, dot1->pos.x, dot1->pos.y, 75, 0, 2 * M_PI);
		cairo_fill(cr);
		++dot1;
	}
	
	/* Text in squares */
	/* calibrate font */
	cairo_set_font_size(cr, 100.);
	cairo_text_extents(cr, nums[0], extent);
	font_scale= 100./extent[0].height;
	
	cairo_set_font_size(cr, game->sq_height*font_scale/2.);
	for(i=0; i<4; ++i) 
		cairo_text_extents(cr, nums[i], extent + i);
	sq= game->squares;
	cairo_set_source_rgb(cr, 0, 0, 0);
	
	for(i=0; i<game->nsquares; ++i) {
		if (sq->number != -1) {		// square has a number
			cairo_move_to(cr, sq->center.x - extent[sq->number].width/2, 
				      sq->center.y + extent[sq->number].height/2);
			cairo_show_text (cr, nums[sq->number]);
		}
		++sq;
	}
}


/*
 * Resize drawarea pixmap (called from configure callback
 */
void
resize_board_pixmap(GtkWidget *drawarea, int width, int height, 
		    int oldw, int oldh)
{	
	if (pixmap == NULL) { 	// first time, need to create pixmap
		pixmap = gdk_pixmap_new(drawarea->window, width, height,-1);
		/* because we paint our pixmap manually during expose events
		  we can turn off gtk's automatic painting and double buffering */
		gtk_widget_set_app_paintable(drawarea, TRUE);
		gtk_widget_set_double_buffered(drawarea, FALSE);
		
		/* request draw of board */
		request_draw(drawarea);
	} else {
		/* unref old pixmap and create new one with new size */
		g_object_unref(pixmap); 
		pixmap= gdk_pixmap_new(drawarea->window, width, height, -1);
		
		/* request draw of board */
		request_draw(drawarea);
	}
}


/*
 * Return pixmap pointer. To be used by functions from outside this file.
 */
inline GdkPixmap*
get_pixmap(void)
{
	return pixmap;
}


/*
 * Return pixel size of pixmap.
 */
void 
get_pixmap_size(int *width, int *height)
{
	gdk_drawable_get_size(pixmap, width, height);
}


/*
 * Copy cairo surface on gtk pixmap
 */
void
copy_board_on_pixmap(cairo_surface_t *csurf)
{
	cairo_t *cr_pixmap = gdk_cairo_create(pixmap);
	cairo_set_source_surface (cr_pixmap, csurf, 0, 0);
	cairo_paint(cr_pixmap);
	cairo_destroy(cr_pixmap);
}
