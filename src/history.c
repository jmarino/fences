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

#include <glib.h>
#include <string.h>
#include <stdio.h>

#include "history.h"
#include "gamedata.h"


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


/*
 * Record an event
 */
GList*
history_record_event(GList *history, GSList *event)
{
	GList *head;
	
	/* if we're not at start of history list -> delete useless history */
	head= g_list_previous(history);
	while(head != NULL && head != history) {
		history_free_event(head->data);
		head= g_list_delete_link(head, head);
	}
	
	/* DEBUG: history should now be head of list */
	g_assert(history == g_list_first(history));
	
	return g_list_prepend(history, event);
}


/*
 * Record a "single" event in history. Single event means an event with only 
 * one change.	
 */
GList*
history_record_event_single(GList *history, int id, int old_state, int new_state)
{
	GSList *event;
	struct line_change *change;
	
	change= (struct line_change*)g_malloc(sizeof(struct line_change));
	change->id= id;
	change->old_state= old_state;
	change->new_state= new_state;
	event= g_slist_prepend(NULL, change);
	
	return history_record_event(history, event);
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
GList*
history_travel_history(GList *history, int offset)
{
	GList *list;
	
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
	
	return history;
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
