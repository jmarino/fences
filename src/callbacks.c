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
#include "draw.h"
#include "history.h"
#include "gui.h"



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
	gboolean inside;
	struct board *board=(struct board*)user_data;
	struct line_change change;

	/* check game state to decide what to do */
	if (board->game_state == GAMESTATE_FINISHED ||
		board->game_state == GAMESTATE_NOGAME) {
		//return TRUE;
	}

	/* Translate pixel coords to board coords */
	point.x= event->x/board->width_pxscale;
	point.y= event->y/board->height_pxscale;

	/* Find in which tile the point falls */
	tile= point.x / board->click_mesh->tile_size;
	tile+= board->click_mesh->ntiles_side*(int)(point.y/board->click_mesh->tile_size);
	//printf("mouse: Tile clicked %d\n", tile);
	list= board->click_mesh->tiles[tile];

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
		change.id= lin->id;
		change.old_state= board->game->states[lin->id];
		switch(event->button) {
			/* left button */
			case 1:
				change.new_state= (change.old_state == LINE_ON) ?
					LINE_OFF : LINE_ON;
				break;
			/* right button */
			case 3:
				change.new_state= (change.old_state == LINE_CROSSED) ?
					LINE_OFF : LINE_CROSSED;
				break;
		    default:
				return TRUE;
		}

		/* record change in history */
		history_record_change(board, &change);

		/* make change to line */
		make_line_change(board, &change);

		/* schedule redraw of box containing line */
		gtk_widget_queue_draw_area
			(GTK_WIDGET(board->drawarea),
			 (gint)(board->geo->clip.x*board->width_pxscale),
			 (gint)(board->geo->clip.y*board->height_pxscale),
			 (gint)(board->geo->clip.w*board->width_pxscale),
			 (gint)(board->geo->clip.h*board->height_pxscale));
	}

	return TRUE;
}


/*
 * Callback when key is pressed
 */
gboolean
window_keypressed(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	struct board *board=(struct board *)user_data;
	GtkWidget *drawarea;

	drawarea= GTK_WIDGET(board->drawarea);
	//printf("key: %d\n", event->keyval);

	if (event->keyval == GDK_b) {
		draw_benchmark(drawarea);
	}
	if (event->keyval == GDK_l) {
		build_new_loop(board->geo, board->game, TRUE);
		gtk_widget_queue_draw(drawarea);
	}
	if (event->keyval == GDK_S) {
		test_solve_game(board->geo, board->game);
		gtk_widget_queue_draw(drawarea);
	}
	if (event->keyval == GDK_s) {
		test_solve_game_trace(board->geo, board->game);
		gtk_widget_queue_draw(drawarea);
	}
	if (event->keyval == GDK_f) {
		brute_force_test(board->geo, board->game);
		gtk_widget_queue_draw(drawarea);
	}
	if (event->keyval == GDK_n) {
		free_gamedata(board->game);
		board->game= build_new_game(board->geo, 0);

		gtk_widget_queue_draw(drawarea);
	}
	if (event->keyval == GDK_c) {
		/* clear board */
		int i;
		for(i=0; i < board->geo->nlines; ++i)
			board->game->states[i]= LINE_OFF;
		gtk_widget_queue_draw(drawarea);
	}
	if (event->keyval == GDK_D) {
		int i;
		printf("Numbers(%d): {", board->geo->ntiles);
		for(i=0; i < board->geo->ntiles - 1; ++i) {
			printf("%d, ", board->game->numbers[i]);
		}
		printf("%d};\n", board->game->numbers[i]);
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
	struct board *board=(struct board *)user_data;

	//printf("configure: %d, %d\n", event->width, event->height);

	/* setup pixel scales: to go from field coords to pixels on screen */
	board->width_pxscale= event->width/(double)board->geo->board_size;
	board->height_pxscale= event->height/(double)board->geo->board_size;

	/*
	 * Recalculate font size and extent boxes of tile numbers
	 * This has to be done after every window resize because the
	 * accuracy of the measurements depends on the pixel size.
	 */
	draw_measure_font(drawarea, event->width, event->height, board->geo);

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
	struct board *board=(struct board *)data;

	//printf("expose\n");
	/* get a cairo_t */
	cr = gdk_cairo_create (drawarea->window);

	/* set a clip region for the expose event (faster) */
	cairo_rectangle (cr,
					 event->area.x, event->area.y,
					 event->area.width, event->area.height);
	cairo_clip (cr);

	/* set scale so we draw in board_size space */
	cairo_scale (cr,
				 drawarea->allocation.width/(double)board->geo->board_size,
				 drawarea->allocation.height/(double)board->geo->board_size);

	draw_board (cr, board->geo, board->game);

	cairo_destroy (cr);

	return TRUE;
}


/*
 * Toolbar button or menuitem 'Undo'
 */
void
action_undo_cb(GtkAction *action, gpointer data)
{
	history_travel_history((struct board*)data, -1);
}


/*
 * Toolbar button or menuitem 'Redo'
 */
void
action_redo_cb(GtkAction *action, gpointer data)
{
	history_travel_history((struct board*)data, 1);
}


/*
 * Tool button or menuitem 'New' clicked
 */
void
action_new_cb(GtkAction *action, gpointer data)
{
	struct board *board=(struct board*)data;
	struct gameinfo info;
	GtkWidget *drawarea=GTK_WIDGET(board->drawarea);

	if (fencesgui_newgame_dialog(board, &info) == FALSE)
		return;

	/* destroy current game and set up new one */
	gamedata_destroy_current_game(board);
	gamedata_create_new_game(board, &info);
	/* force redraw of gui parts */
	fencesgui_set_undoredo_state(board);
	draw_measure_font(drawarea,
					  drawarea->allocation.width,
					  drawarea->allocation.height, board->geo);
	gtk_widget_queue_draw(GTK_WIDGET(board->drawarea));
}


/*
 * Tool button or menuitem 'Clear' clicked
 */
void
action_clear_cb(GtkAction *action, gpointer data)
{
	struct board *board=(struct board*)data;

	if ( fences_clear_dialog(board->window) ) {
		gamedata_clear_game(board);
		fencesgui_set_undoredo_state(board);
		gtk_widget_queue_draw(GTK_WIDGET(board->drawarea));
	}
}


/*
 * Tool button or menuitem 'Hint' clicked
 */
void
action_hint_cb(GtkAction *action, gpointer data)
{
	g_debug("hint action");
}


/*
 * Tool button or menuitem 'About' clicked
 */
void
action_about_cb(GtkAction *action, gpointer data)
{
	fencesgui_show_about_dialog((struct board*)data);
}
