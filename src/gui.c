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


#define MENU_TOOLBAR_UI_FILE	"menu_toolbar.ui"


/* **HACK** **FIXME** define gettext macro to avoid errors */
#ifndef N_
#define N_(a) (a)
#define _(a)  (a)
#endif


/*
 * Menu entries
 */
static const GtkActionEntry ui_actions[]=
{
	/* Toplevel */
	{ "Game", NULL, N_("_Game"), NULL, NULL, NULL },
	{ "Edit", NULL, N_("_Edit"), NULL, NULL, NULL },
	{ "Help", NULL, N_("_Help"), NULL, NULL, NULL },
	/* Game menu */
	{ "new", GTK_STOCK_NEW, NULL, "<control>N",
	  N_("Start new game"), G_CALLBACK(action_new_cb) },
	{ "clear", GTK_STOCK_CLEAR, NULL, "<control>C",
	  N_("Clear board"), G_CALLBACK(action_clear_cb) },
	{ "quit", GTK_STOCK_QUIT, NULL, "<control>Q",
	  N_("Quit"), G_CALLBACK(gtk_main_quit) },
	/* Edit menu */
	{ "undo", GTK_STOCK_UNDO, NULL, "<control>Z",
	  N_("Undo move"), G_CALLBACK(action_undo_cb) },
	{ "redo", GTK_STOCK_REDO, NULL, "<control>R",
	  N_("Redo move"), G_CALLBACK(action_redo_cb) },
	/* Help menu */
	{ "about", GTK_STOCK_ABOUT, NULL, NULL,
	  N_("About fences"), G_CALLBACK(action_about_cb) },
	/* Toolbar only */
	{ "hint", GTK_STOCK_DIALOG_INFO, N_("Hint"), NULL,
	  N_("Give a hint"), G_CALLBACK(action_hint_cb) }
};


/*
 * Setup UIManager
 */
static GtkUIManager*
gui_setup_uimanager(GtkWidget *window, struct board *board)
{
	GtkUIManager *uiman;
	GtkAccelGroup *accel_group;
	GtkActionGroup *action_group;
	GtkAction *action;
	GError *error=NULL;

	/* UI Manager */
	uiman= gtk_ui_manager_new();
	gtk_ui_manager_add_ui_from_file(uiman, MENU_TOOLBAR_UI_FILE, &error);
	if (error != NULL) {
		g_warning ("Could not merge %s: %s", "menus.ui", error->message);
		g_error_free (error);
	}

	/* add UI manager's accel group to main window */
	accel_group= gtk_ui_manager_get_accel_group(uiman);
	gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);

	/* create action group */
	action_group= gtk_action_group_new("UIFences");
	gtk_action_group_add_actions(action_group,
								 ui_actions, G_N_ELEMENTS(ui_actions),
								 board);
	gtk_ui_manager_insert_action_group  (uiman, action_group, 0);

	/* store some actions in main window */
	action= gtk_action_group_get_action(action_group, "undo");
	g_object_set_data(G_OBJECT(window), "undo-action", action);
	action= gtk_action_group_get_action(action_group, "redo");
	g_object_set_data(G_OBJECT(window), "redo-action", action);

	g_object_unref(action_group);

	return uiman;
}


/*
 * Setup gui
 */
GtkWidget*
gui_setup_main_window(const char *xml_file, struct board *board)
{
	GtkWidget *window;
	GtkWidget *drawarea;
	GtkUIManager *uiman;
	GtkWidget *vbox;
	GtkWidget *menubar;
	GtkWidget *toolbar;
	GtkWidget *statbar;

	window= gtk_window_new(GTK_WINDOW_TOPLEVEL);
	board->window= window;
	gtk_window_set_title(GTK_WINDOW(window), "fences game");
	gtk_window_set_default_size(GTK_WINDOW(window), 500, 400);

	/* vbox containing window elements */
	vbox= gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	/* connect widgets from UI manager to window */
	uiman= gui_setup_uimanager(window, board);
	menubar= gtk_ui_manager_get_widget(uiman, "/MenuBar");
	gtk_box_pack_start(GTK_BOX(vbox), menubar, TRUE, TRUE, 0);
	toolbar= gtk_ui_manager_get_widget(uiman, "/ToolBar");
	gtk_box_pack_start(GTK_BOX(vbox), toolbar, TRUE, TRUE, 0);
	g_object_unref(uiman);

	/* drawing area */
	drawarea= gtk_drawing_area_new();
	gtk_box_pack_start(GTK_BOX(vbox), drawarea, TRUE, TRUE, 0);
	board->drawarea= drawarea;
	/* store data in drawarea */
	g_object_set_data(G_OBJECT(drawarea), "board", board);
	g_object_set_data(G_OBJECT(drawarea), "window", window);
	g_object_set_data(G_OBJECT(window), "drawarea", drawarea);
	/* Ensure drawing area has aspect ratio of 1 */
	/*GdkGeometry geo;
	geo.min_aspect= geo.max_aspect= 1.0;
	gtk_window_set_geometry_hints(GTK_WINDOW(window), drawarea, &geo,
					GDK_HINT_ASPECT);*/
	gtk_widget_set_size_request(drawarea, 500,500);

	/* status bar */
	statbar= gtk_statusbar_new();
	gtk_box_pack_start(GTK_BOX(vbox), statbar, FALSE, TRUE, 0);


	/* connect some signals */
	g_signal_connect(drawarea, "configure-event",
					 G_CALLBACK(drawarea_configure), NULL);
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

	gtk_widget_show_all(window);

	return window;
}


/*
 * Initialize gui aspects after game data and main window have been setup
 */
void
gui_initialize(GtkWidget *window, struct board *board)
{
	gboolean state;
	GObject *object;

	/* set state of undo */
	if (g_list_next(board->history) != NULL) state= TRUE;
	else state= FALSE;
	object= g_object_get_data(G_OBJECT(window), "undo-action");
	gtk_action_set_sensitive(GTK_ACTION(object), state);

	/* set state of redo */
	if (g_list_previous(board->history) != NULL) state= TRUE;
	else state= FALSE;
	object= g_object_get_data(G_OBJECT(window), "redo-action");
	gtk_action_set_sensitive(GTK_ACTION(object), state);
}


/*
 * Warning dialog for game about to be cleared
 */
gboolean
fences_clear_dialog(GtkWindow *parent)
{
	GtkWidget *dialog;
	GtkWidget *image;
	GtkWidget *primary_label;
	GtkWidget *secondary_label;
	GtkWidget *hbox;
	GtkWidget *vbox;
	gchar *str;
	gint response;

	/* create dialog */
	dialog= gtk_dialog_new_with_buttons(
		"", parent,
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
		GTK_STOCK_CANCEL, GTK_RESPONSE_NO,
		GTK_STOCK_CLEAR, GTK_RESPONSE_YES, NULL);
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
	gtk_box_set_spacing(GTK_BOX(GTK_DIALOG(dialog)->vbox), 14);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);

	/* Image */
	image= gtk_image_new_from_stock(GTK_STOCK_CLEAR, GTK_ICON_SIZE_DIALOG);
	gtk_misc_set_alignment(GTK_MISC(image), 0.5, 0.0);

	/* Primary label */
	primary_label= gtk_label_new(NULL);
	gtk_label_set_line_wrap(GTK_LABEL(primary_label), TRUE);
	gtk_label_set_use_markup(GTK_LABEL(primary_label), TRUE);
	gtk_misc_set_alignment(GTK_MISC(primary_label), 0.0, 0.5);
	gtk_label_set_selectable(GTK_LABEL(primary_label), TRUE);
	str= g_markup_printf_escaped("<span weight=\"bold\" size=\"larger\">%s</span>",
								 _("Clear Game?"));
	gtk_label_set_markup(GTK_LABEL(primary_label), str);
	g_free (str);
	/* Secondary label */
	str= g_strdup(_("Current game will be lost."));
	secondary_label= gtk_label_new(str);
	g_free(str);
	gtk_label_set_line_wrap(GTK_LABEL(secondary_label), TRUE);
	gtk_misc_set_alignment(GTK_MISC(secondary_label), 0.0, 0.5);
	gtk_label_set_selectable(GTK_LABEL(secondary_label), TRUE);

	/* build dialog contents */
	hbox= gtk_hbox_new(FALSE, 12);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
	gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
	vbox= gtk_vbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), primary_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), secondary_label, FALSE, FALSE, 0);

	/* add content to dialog's vbox */
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox),
						hbox, FALSE, FALSE,	0);

	/* show all */
	gtk_widget_show_all (dialog);

	/* run dialog */
	response= gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);

	return response == GTK_RESPONSE_YES;
}
