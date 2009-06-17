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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <config.h>

#include <gtk/gtk.h>
//#include <glade/glade.h>


#include "gamedata.h"
#include "gui.h"


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


/* defined in gamedata.c */
extern struct board board;


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


int
main (int argc, char *argv[])
{
	GtkWidget *window;


#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	/* Initialize thread stuff to make gtk thread-aware */
	if (!g_thread_supported()) g_thread_init(NULL);
	gdk_threads_init();
	/* gtk_main must be between gdk_threads_enter and gdk_threads_leave */
	gdk_threads_enter();

	/* Init board */
	initialize_board();

	/* test routine to build loop */
	//try_loop(board.geo, board.game);

	gtk_set_locale ();
	gtk_init (&argc, &argv);

	/* create main window */
	window= gui_setup_main_window(XML_FILE, &board);
	gtk_widget_show (window);

	/* initialize gui */
	gui_initialize(window, &board);

	/* start draw thread */
	//start_draw_thread(g_object_get_data(G_OBJECT(window), "drawarea"));

	gtk_main ();
	gdk_threads_leave();

	return 0;
}
