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
#include <string.h>
#include <stdio.h>

#include "gamedata.h"
#include "history.h"


/*
 * History is a double-linked list containing a series of events.
 * Event is a single-linked list containing a series of line changes.
 */

void history_free_event(GSList *event);


/* contains info about a line change */
struct line_change {
	int id;			// id of line that changes
	int old_state;		// old state of line
	int new_state;		// new state of line
};



/**
 * Set correct sensitiveness for undo/redo buttons
 */
static void
history_set_undoredo_state(struct board *board)
{
	gboolean state;
	GtkWidget *window;
	GObject *object;

	window= g_object_get_data(G_OBJECT(board->drawarea), "window");

	/* set undo button & menu */
	if (g_list_next(board->history) == NULL) state= FALSE;
	else state= TRUE;
	object= g_object_get_data(G_OBJECT(window), "undo-action");
	gtk_action_set_sensitive(GTK_ACTION(object), state);

	/* set redo button & menu */
	if (g_list_previous(board->history) == NULL) state= FALSE;
	else state= TRUE;
	object= g_object_get_data(G_OBJECT(window), "redo-action");
	gtk_action_set_sensitive(GTK_ACTION(object), state);
}


/*
 * Record an event
 */
void
history_record_event(struct board *board, GSList *event)
{
	GList *head;

	/* history empty? -> create empty stub to make undo behave */
	if (board->history == NULL) {
		board->history= g_list_prepend(NULL, NULL);
	}

	/* if we're not at start of history list -> delete useless history */
	head= g_list_first(board->history);
	while(head != board->history) {
		history_free_event(head->data);
		head= g_list_delete_link(head, head);
	}

	/* DEBUG: history should now be head of list */
	g_assert(board->history == g_list_first(board->history));

	board->history= g_list_prepend(board->history, event);

	/* set undo/redo sensitivity */
	history_set_undoredo_state(board);
}


/*
 * Record a "single" event in history. Single event means an event with only
 * one change.
 */
void
history_record_event_single(struct board *board, int id, int old_state, int new_state)
{
	GSList *event;
	struct line_change *change;

	change= (struct line_change*)g_malloc(sizeof(struct line_change));
	change->id= id;
	change->old_state= old_state;
	change->new_state= new_state;
	event= g_slist_prepend(NULL, change);

	history_record_event(board, event);
}


/*
 * Free event (slist of line changes)
 */
void
history_free_event(GSList *event)
{
	while(event != NULL) {
		g_free(event->data);
		event= g_slist_delete_link(event, event);
	}
}


/*
 * Undo given event (slist of line changes)
 */
void
history_undo_event(GSList *event)
{
	struct line_change *change;

	/* travel slist setting lines to old_state */
	while(event != NULL) {
		change= (struct line_change*)event->data;
		game_set_line(change->id, change->old_state);
		event= g_slist_next(event);
	}
}


/*
 * Redo given event (slist of line changes)
 */
void
history_redo_event(GSList *event)
{
	struct line_change *change;

	/* travel slist setting lines to new_state */
	while(event != NULL) {
		change= (struct line_change*)event->data;
		game_set_line(change->id, change->new_state);
		event= g_slist_next(event);
	}
}


/*
 * Revisit history
 * offset indicates which direction and how many steps to go
 */
void
history_travel_history(struct board *board, int offset)
{
	GList *list;
	GList *history=board->history;

	if (history == NULL || offset == 0) return;
	if (offset < 0) {	// backward (undo)
		while(offset != 0) {
			list= g_list_next(history);
			if (list == NULL) break;
			history_undo_event(history->data);
			history= list;
			++offset;
		}
	} else {		// forward (redo)
		while(offset != 0) {
			list= g_list_previous(history);
			if (list == NULL) break;
			history= list;
			history_redo_event(history->data);
			--offset;
		}
	}
	/* redraw board if necessary */
	if (board->history != history) {
		board->history= history;
		/* set undo/redo sensitivity */
		history_set_undoredo_state(board);
		gtk_widget_queue_draw(board->drawarea);
	}

	board->history= history;
}


/*
 * Free history memory
 */
void
history_free(GList *history)
{
	history= g_list_first(history);

	while(history != NULL) {
		history_free_event(history->data);
		history= g_list_delete_link(history, history);
	}
}
