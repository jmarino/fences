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
#include <gdk/gdkkeysyms.h>

#include "callbacks.h"
#include "gamedata.h"
#include "geometry_tools.h"
#include "draw.h"

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
	int *states;
	
	/* Translate pixel coords to board coords */
	point.x= event->x/board.width_pxscale;
	point.y= event->y/board.height_pxscale;

	/* Find in which tile the point falls */
	tile= point.x / board.tile_cache->tile_size;
	tile+= board.tile_cache->ntiles_side*(int)(point.y/board.tile_cache->tile_size);
	//printf("mouse: Tile clicked %d\n", tile);
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
		//printf("mouse: - Line %d\n", lin->id);
		states= board.game->states;
		switch(event->button) {
			/* left button */
			case 1: 
				states[lin->id]= (states[lin->id] == LINE_ON) ? 
					LINE_OFF : LINE_ON;
				break;
			/* right button */
			case 3: 
				states[lin->id]= (states[lin->id] == LINE_CROSSED) ? 
					LINE_OFF : LINE_CROSSED;
				break;
		}
		/* schedule redraw of box containing line */
		gtk_widget_queue_draw_area
			(GTK_WIDGET(drawarea), 
			 (gint)(lin->inf_box[0].x*board.width_pxscale), 
			 (gint)(lin->inf_box[0].y*board.height_pxscale),
			 (gint)(lin->inf_box[1].x*board.width_pxscale), 
			 (gint)(lin->inf_box[1].y*board.height_pxscale));
	}
	
	return TRUE;
}


/*
 * Callback when key is pressed
 */
gboolean 
window_keypressed(GtkWidget *widget, GdkEventKey *event, gpointer drawarea)
{
	//printf("key: %d\n", event->keyval);
	
	if (event->keyval == GDK_b) {
		draw_benchmark(drawarea);
	}
	//(void)g_timeout_add(200, (GSourceFunc)timer_function, drawarea);
	
	return FALSE;
}


/*
 * Callback when drawing area is resized (also first time is drawn)
 */
gboolean
drawarea_configure(GtkWidget *drawarea, GdkEventConfigure *event, gpointer user_data)
{
	//printf("configure: %d, %d\n", event->width, event->height);
		
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
	cairo_t *cr;
	
	//printf("expose\n");
	/* get a cairo_t */
	cr = gdk_cairo_create (drawarea->window);
	
	/* set a clip region for the expose event (faster) */
	cairo_rectangle (cr,
			 event->area.x, event->area.y,
			 event->area.width, event->area.height);
	cairo_clip (cr);
	
	draw_board (cr, drawarea->allocation.width, drawarea->allocation.height);
	
	cairo_destroy (cr);
	
	return TRUE;
}
