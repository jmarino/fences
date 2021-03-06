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

#ifndef __INCLUDED_GUI_H__
#define __INCLUDED_GUI_H__


/*
 * Functions
 */
/* gui.c */
GtkWidget* gui_setup_main_window(const char *xml_file, struct board *board);
gboolean fences_clear_dialog(GtkWindow *parent);
void fencesgui_set_undoredo_state(struct board *board);
void fencesgui_show_about_dialog(struct board *board);
void gui_initialize(struct board *board);

/* newgame-dialog.c */
gboolean fencesgui_newgame_dialog(struct board *board, struct gameinfo *info);


#endif
