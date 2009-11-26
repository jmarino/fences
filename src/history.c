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
#include "gui.h"


/*
 * History is a double-linked list containing changes.
 * Event is an instance of line_change describing a line change
 */

void history_free_event(GSList *event);



/*
 * History is made up of chunks chained together
 * Each chunk is HISTORY_SEGMENT_SIZE consecutive struct line_change
 * num_segments: total number of allocated segments
 * current_segment: index of current segment
 * index: current position in history
 * last: head of history
 * Variables index and last point to last change, initially (no changes yet)
 * they point to -1.
 * Segments are prepended into list. Example: (segment size 5)
 *      | 5 | 6 | x | x | x |   -   | 0 | 1 | 2 | 3 | 4 |
 *             segment 1                    segment 0
 *       index and last point to 6
 */
#define HISTORY_SEGMENT_SIZE	100
/*
 * Stores history data
 */
struct history {
	GList *segments;			// start of list of segments
	GList *segment;				// current segment (where index points)
	int num_segments;			// total number of segments allocated
	int current_segment;		// current segment
	int index;					// index of current history position
	int last;					// last history change
};


/*
 * Initialize history as empty
 */
static void
history_reset(struct history *history)
{
	history->segments= NULL;
	history->segment= NULL;
	history->num_segments= 0;
	history->current_segment= -1;
	history->index= -1;
	history->last= -1;
}


/*
 * Create new variable to store history
 */
struct history *
history_create(void)
{
	struct history *history;

	history= g_malloc(sizeof(struct history));
	history_reset(history);
	return history;
}


/*
 * Record an event
 */
void
history_record_change(struct board *board, struct line_change *change)
{
	struct line_change *change_ptr;
	struct history *history=board->history;
	int pos;

	/* allocate a new segment if needed:
	   if first change or current segment is full */
	pos= (history->num_segments) * HISTORY_SEGMENT_SIZE - 1;
	if (history->index == pos) {
		change_ptr= g_malloc(sizeof(struct line_change)*HISTORY_SEGMENT_SIZE);
		history->segments= g_list_prepend(history->segments, change_ptr);
		history->segment= history->segments;
		++history->num_segments;
		++history->current_segment;
	}

	/* store change */
	++history->index;
	history->last= history->index;
	pos= history->index % HISTORY_SEGMENT_SIZE;
	change_ptr= ((struct line_change *)history->segment->data) + pos;
	change_ptr->id= change->id;
	change_ptr->old_state= change->old_state;
	change_ptr->new_state= change->new_state;

	/* set undo/redo sensitivity */
	fencesgui_set_undoredo_state(board);
}


/*
 * Revisit history
 * offset indicates which direction (only goes one step)
 */
void
history_travel_history(struct board *board, int offset)
{
	struct history *history=board->history;
	int pos;
	struct line_change *change;

	if (offset == 0) return;
	if (offset < 0) {	// backward (undo)
		if (history->index == -1) return;
		// get change
		pos= history->index % HISTORY_SEGMENT_SIZE;
		g_assert(pos == history->index - (history->current_segment*HISTORY_SEGMENT_SIZE));
		change= ((struct line_change *)history->segment->data) + pos;
		// decrement index
		--history->index;
		if (pos == 0 && history->index != -1) {			// change segment
			--history->current_segment;
			history->segment= g_list_next(history->segment);
		}
		g_assert(history->segment != NULL);
		game_set_line(change->id, change->old_state);
	} else {		// forward (redo)
		if (history->index == history->last) return;
		// increment index
		++history->index;
		pos= history->index % HISTORY_SEGMENT_SIZE;
		if (pos == 0 && history->index != 0) {
			++history->current_segment;
			history->segment= g_list_previous(history->segment);
		}
		g_assert(history->segment != NULL);
		// get change
		g_assert(pos == history->index - (history->current_segment*HISTORY_SEGMENT_SIZE));
		change= ((struct line_change *)history->segment->data) + pos;
		game_set_line(change->id, change->new_state);
	}

	/* set undo/redo sensitivity */
	fencesgui_set_undoredo_state(board);
	/* redraw board */
	gtk_widget_queue_draw(board->drawarea);
}


/*
 * Clear history: Free memory used by history and reset to empty.
 * Variable history is not freed and can be reused.
 */
void
history_clear(struct history *history)
{
	GList *list;

	g_assert(history != NULL);
	list= history->segments;
	while(list != NULL) {
		g_free(list->data);
		list= g_list_delete_link(list, list);
	}
	history_reset(history);
}


/*
 * Can we undo?
 */
inline gboolean
history_can_undo(struct history *history)
{
	return (history->index > -1);
}


/*
 * Can we redo?
 */
inline gboolean
history_can_redo(struct history *history)
{
	return (history->index < history->last);
}
