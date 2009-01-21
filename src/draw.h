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

#ifndef __INCLUDED_DRAW_H__
#define __INCLUDED_DRAW_H__


void draw_board(cairo_t *cr, int width, int height);
void resize_board_pixmap(GtkWidget *drawarea, int width, int height, 
			 int oldw, int oldh);
inline GdkPixmap* get_pixmap(void);
void get_pixmap_size(int *width, int *height);
void copy_board_on_pixmap(cairo_surface_t *csurf);

#endif