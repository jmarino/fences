/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * callbacks.h
 * Copyright (C) Jose Marino 2008 <jose.marino@gmail.com>
 * 
 * callbacks.h is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * callbacks.h is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtk/gtk.h>


gboolean 
drawarea_mouseclicked(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
gboolean 
window_keypressed(GtkWidget *widget, GdkEventKey *event, gpointer user_data);

gboolean
drawarea_configure(GtkWidget *widget, GdkEventConfigure *event, gpointer user_data);
gboolean
drawarea_resize(GtkWidget *widget, gpointer user_data);
