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


extern struct board board;


/*
 * Callback when mouse is clicked on the board
 */
gboolean 
drawarea_mouseclicked(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	struct point point;
	int tile;
	GSList *list;
	struct line *lin;
	int line_num=-1;
	gboolean inside;
	int state;
	
	/* Translate pixel coords to board coords */
	point.x= (int)(event->x/500.*board.board_size);
	point.y= (int)(event->y/500.*board.board_size);

	printf("mouse: %3.1lf,%3.1lf  (%d,%d)\n", event->x, event->y,
		   point.x, point.y);

	/* Find in which tile the point falls */
	tile= point.x / board.tile_cache->tile_size +
		board.tile_cache->ntiles_side*(point.y / board.tile_cache->tile_size);
	printf("mouse: Tile clicked %d\n", tile);
	list= board.tile_cache->tiles[tile];
	
	/* Find in which line's area of influence the point falls */
	while(list != NULL) {
		lin= (struct line *)list->data;
		inside= is_point_inside_area(&point, lin->inf);
		if (inside) {
			line_num= lin->id;
			state= lin->state;
			if (state == LINE_ON) state= LINE_OFF;
			else if (state == LINE_OFF) state= LINE_ON;
			else if (state == LINE_CROSSED) state= LINE_ON;
			gtk_widget_queue_draw(GTK_WIDGET(user_data));
			lin->state= state;
			printf("mouse: ** inf: (%d,%d),(%d,%d),(%d,%d),(%d,%d)\n",
				   lin->inf[0].x, lin->inf[0].y, lin->inf[1].x, lin->inf[1].y,
				   lin->inf[2].x, lin->inf[2].y, lin->inf[3].x, lin->inf[3].y);
			printf("Line at point: %d\n",  line_num);
			break;
		}
		list= g_slist_next(list);
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


gboolean
drawarea_configure(GtkWidget *widget, GdkEventConfigure *event, gpointer user_data)
{
	printf("configure: %d, %d\n", event->width, event->height);
	return FALSE;
}


gboolean
drawarea_resize(GtkWidget *widget, gpointer user_data)
{
	printf("resize: %d, %d\n", 0, 0);
	return FALSE;
}
