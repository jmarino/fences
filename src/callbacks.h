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

#ifndef __INCLUDED_CALLBACKS_H__
#define __INCLUDED_CALLBACKS_H__

#include <gtk/gtk.h>


gboolean drawarea_mouseclicked(GtkWidget *widget, GdkEventButton *event,
			       gpointer drawarea);
gboolean window_keypressed(GtkWidget *widget, GdkEventKey *event, gpointer user_data);
gboolean drawarea_configure(GtkWidget *widget, GdkEventConfigure *event,
			    gpointer user_data);
gboolean drawarea_resize(GtkWidget *widget, gpointer user_data);
gboolean board_expose(GtkWidget *drawarea, GdkEventExpose *event, gpointer data);
void undo_button_clicked(GtkWidget *button, gpointer data);
void redo_button_clicked(GtkWidget *button, gpointer data);
void new_button_clicked(GtkWidget *button, gpointer data);


#endif
