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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "callbacks.h"
#include "gamedata.h"
#include "geometry_tools.h"
#include "draw.h"
#include "draw_thread.h"

/* defined in gamedata.c */
extern struct board board;


/*
 * Callback when mouse is clicked on the board
 */
gboolean 
drawarea_mouseclicked(GtkWidget *widget, GdkEventButton *event, gpointer drawarea)
{
	struct point point;
	int tile;
	GSList *list;
	struct line *lin;
	gboolean inside;
	
	/* Translate pixel coords to board coords */
	point.x= (int)(event->x/500.*board.board_size);
	point.y= (int)(event->y/500.*board.board_size);

	printf("mouse: %3.1lf,%3.1lf  (%d,%d)\n", event->x, event->y,
		   point.x, point.y);

	/* Find in which tile the point falls */
	tile= point.x / board.tile_cache->tile_size;
	tile+= board.tile_cache->ntiles_side*(point.y/board.tile_cache->tile_size);
	printf("mouse: Tile clicked %d\n", tile);
	list= board.tile_cache->tiles[tile];
	
	/* Find in which line's area of influence the point falls */
	while(list != NULL) {
		lin= (struct line *)list->data;
		inside= is_point_inside_area(&point, lin->inf);
		if (inside)
			break;
		list= g_slist_next(list);
	}
	
	/* check if a line was found */
	if (list != NULL) {
		switch(event->button) {
			/* left button */
			case 1: lin->state= (lin->state == LINE_ON) ? LINE_OFF : LINE_ON;
				break;
			/* right button */
			case 3: lin->state= (lin->state == LINE_CROSSED) ? LINE_OFF : LINE_CROSSED;
				break;
		}
		/* make sure changes are drawn */
		request_draw(GTK_WIDGET(drawarea));
		
		/* schedule redraw of box containing line */
		/*gtk_widget_queue_draw_area
			(GTK_WIDGET(drawarea), 
			 (gint)(lin->inf_box[0].x*board.width_pxscale), 
			 (gint)(lin->inf_box[0].y*board.height_pxscale),
			 (gint)(lin->inf_box[1].x*board.width_pxscale), 
			 (gint)(lin->inf_box[1].y*board.height_pxscale)); */
	}

	return FALSE;
}


/*
 * Callback when key is pressed
 */
gboolean 
window_keypressed(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	printf("key: %d\n", event->keyval);
	return FALSE;
}


/*
 * Callback when drawing area is resized (also first time is drawn)
 */
gboolean
drawarea_configure(GtkWidget *drawarea, GdkEventConfigure *event, gpointer user_data)
{
	static int oldw = 0;
	static int oldh = 0;

	printf("configure: %d, %d\n", event->width, event->height);
	
	if (oldw != event->width || oldh != event->height){
		resize_board_pixmap(drawarea, event->width, event->height, oldw, oldh);
	}
	oldw = event->width;
	oldh = event->height;
	
	/* setup pixel scales: to go from field coords to pixels on screen */
	board.width_pxscale= event->width/(double)board.board_size;
	board.height_pxscale= event->height/(double)board.board_size;
	
	return TRUE;
}


gboolean
drawarea_resize(GtkWidget *widget, gpointer user_data)
{
	printf("resize: %d, %d\n", 0, 0);
	return FALSE;
}


/*
 * Function called when drawing area receives expose event 
 */
gboolean
board_expose(GtkWidget *drawarea, GdkEventExpose *event, gpointer data)
{
	/* check if we shouldn't expose (because drawing is busy) */
	if (is_expose_disabled()) {
		printf("expose: skipping\n");
		return TRUE;
	}
	printf("expose: hi\n");
	gdk_draw_drawable(drawarea->window,
			  drawarea->style->fg_gc[GTK_WIDGET_STATE(drawarea)], 
			  get_pixmap(),
			  // Only copy the area that was exposed.
			  event->area.x, event->area.y,
			  event->area.x, event->area.y,
			  event->area.width, event->area.height);
	return TRUE;
}
