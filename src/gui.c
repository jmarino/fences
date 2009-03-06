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

#include "gamedata.h"
#include "callbacks.h"



/*
 * Setup gui
 */
GtkWidget*
gui_setup_main_window(const char *xml_file, struct board *board)
{
	GtkWidget *window;
	GtkWidget *drawarea;
	GtkBuilder *builder;
	GObject *toolbutton;
	
	builder = gtk_builder_new ();
	gtk_builder_add_from_file (builder, xml_file, NULL);
	
	window = GTK_WIDGET(gtk_builder_get_object (builder, "window"));
	/* note: connect_signals doesn't work, it needs some gmodule stuff */
	//gtk_builder_connect_signals (builder, NULL);
	
	drawarea= GTK_WIDGET(gtk_builder_get_object(builder, "drawingarea"));
	/* store data in drawarea */
	g_object_set_data(G_OBJECT(drawarea), "board", board);
	g_object_set_data(G_OBJECT(drawarea), "window", window);
	board->drawarea= drawarea;
	
	g_signal_connect(drawarea, "configure-event", G_CALLBACK(drawarea_configure), 
			 NULL);
	//g_signal_connect(drawarea, "check-resize", G_CALLBACK(drawarea_resize), 
	//		 NULL);
	g_signal_connect(drawarea, "expose_event", G_CALLBACK(board_expose), 
			 board);
	g_signal_connect(window, "delete_event", gtk_main_quit, NULL);

	/* capture any key pressed in the window */
  	g_signal_connect ((gpointer) window, "key-press-event",
			    G_CALLBACK (window_keypressed), drawarea);
	
	/* catch mouse clicks on game board */
	gtk_widget_add_events(drawarea, 
			      GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK);
	g_signal_connect (G_OBJECT (drawarea), "button_release_event", 
			  G_CALLBACK (drawarea_mouseclicked), drawarea);

	g_signal_connect(gtk_builder_get_object (builder, "Game_Quit_menuitem"),
			 "activate", gtk_main_quit, NULL);
		
	/* done with builder */
	g_object_unref (G_OBJECT (builder));

	/* Ensure drawing area has aspect ratio of 1 */
	/*GdkGeometry geo;
	geo.min_aspect= geo.max_aspect= 1.0;
	gtk_window_set_geometry_hints(GTK_WINDOW(window), drawarea, &geo, 
					GDK_HINT_ASPECT);*/
	gtk_widget_set_size_request(drawarea, 500,500);
	
	/* store current drawarea (maybe there'll be a netbook) in window */
	g_object_set_data(G_OBJECT(window), "drawarea", drawarea);
	
	return window;
}


/*
 * Initialize gui aspects after game data and main window have been setup
 */
void
gui_initialize(GtkWidget *window, struct board *board)
{
	
}
