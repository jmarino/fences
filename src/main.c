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

#include "i18n.h"
#include "gamedata.h"
#include "gui.h"




/* UI definition file */
#define XML_FILE "fences.ui"


/*
 * Clean up things just before exiting
 */
static void
fences_exit_cleanup(struct board *board)
{
	gamedata_destroy_current_game(board);
}


/*
 *
 */
int
main (int argc, char *argv[])
{
	GtkWidget *window;
	struct board *board;


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
	board= initialize_board();

	gtk_set_locale ();
	gtk_init (&argc, &argv);

	/* create main window */
	window= gui_setup_main_window(XML_FILE, board);
	gtk_widget_show (window);

	/* initialize gui */
	gui_initialize(board);

	/* start draw thread */
	//start_draw_thread(g_object_get_data(G_OBJECT(window), "drawarea"));

	gtk_main ();
	gdk_threads_leave();

	/* clean up */
	fences_exit_cleanup(board);

	return 0;
}
