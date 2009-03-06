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

#ifndef __INCLUDED_HISTORY_H__
#define __INCLUDED_HISTORY_H__


/*
 * Functions
 */

void history_record_event(struct board *board, GSList *event);
void history_record_event_single(struct board *board, int id, int old_state, 
				 int new_state);
void history_undo_event(GSList *event);
void history_redo_event(GSList *event);
void history_travel_history(struct board *board, int offset);
void history_free(GList *history);


#endif
