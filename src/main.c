/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * main.c
 * Copyright (C) Jose Marino 2008 <jose.marino@gmail.com>
 * 
 * main.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * main.c is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <config.h>

#include <gtk/gtk.h>
//#include <glade/glade.h>



/*
 * Standard gettext macros.
 */
#ifdef ENABLE_NLS
#  include <libintl.h>
#  undef _
#  define _(String) dgettext (PACKAGE, String)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else
#  define textdomain(String) (String)
#  define gettext(String) (String)
#  define dgettext(Domain,Message) (Message)
#  define dcgettext(Domain,Message,Type) (Message)
#  define bindtextdomain(Domain,Directory) (Domain)
#  define _(String) (String)
#  define N_(String) (String)
#endif



#include "callbacks.h"



gboolean board_face_expose(GtkWidget *drawarea, GdkEventExpose *event, gpointer gamedata);

/* extern global variables */
extern struct game gamedata;


/* For testing propose use the local (not installed) glade file */
/* #define GLADE_FILE PACKAGE_DATA_DIR"/fences/glade/fences.glade" */
#define GLADE_FILE "fences.glade"
#define XML_FILE "fences.xml"

/*
GtkWidget*
create_window2 (void)
{
	GtkWidget *window;
	GladeXML *gxml;
	
	gxml = glade_xml_new (GLADE_FILE, NULL, NULL);
	
	// This is important 
	glade_xml_signal_autoconnect (gxml);
	window = glade_xml_get_widget (gxml, "window");
	
	return window;
}*/


GtkWidget*
create_window (void)
{
	GtkWidget *window;
	GtkWidget *drawarea;
	GtkBuilder *builder;
	
	builder = gtk_builder_new ();
	gtk_builder_add_from_file (builder, XML_FILE, NULL);
	
	window = GTK_WIDGET(gtk_builder_get_object (builder, "window"));
	/* note: connect_signals doesn't work, it needs some gmodule stuff */
	//gtk_builder_connect_signals (builder, NULL);
	
	drawarea= GTK_WIDGET(gtk_builder_get_object(builder, "drawingarea"));
	g_signal_connect(drawarea, "configure-event", G_CALLBACK(drawarea_configure), 
					 NULL);
	g_signal_connect(drawarea, "check-resize", G_CALLBACK(drawarea_resize), 
					 NULL);
	g_signal_connect(drawarea, "expose_event", G_CALLBACK(board_face_expose), 
					 &gamedata);
	g_signal_connect(window, "delete_event", gtk_main_quit, NULL);

	/* capture any key pressed in the window */
  	g_signal_connect ((gpointer) window, "key-press-event",
					G_CALLBACK (window_keypressed), NULL);
	
	/* catch mouse clicks on game board */
	g_signal_connect (G_OBJECT (drawarea), "button_press_event", 
					G_CALLBACK (drawarea_mouseclicked), NULL);
	
	g_object_unref (G_OBJECT (builder));
	
	/* store gamedata in window */
	g_object_set_data(G_OBJECT(window), "window", &gamedata);
	
	return window;
}


int
main (int argc, char *argv[])
{
 	GtkWidget *window;


#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	generate_example_game();
	
	gtk_set_locale ();
	gtk_init (&argc, &argv);

	/* create GUI from xml file */
	window = create_window ();
	gtk_widget_show (window);

	gtk_main ();
	return 0;
}
