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
	printf("mouse: %lf, %lf\n", event->x, event->y);
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
