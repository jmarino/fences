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

#include "i18n.h"
#include "gamedata.h"
#include "draw.h"
#include "geometry.h"


#define PREVIEW_IMAGE_SIZE		160

#define NUM_TILE_TYPE		5
#define NUM_DIFFICULTY		6


/*
 * Data associated with dialog
 */
struct dialog_data {
	GtkWidget *tile_button[NUM_TILE_TYPE];
	GtkWidget *diff_button[NUM_DIFFICULTY];
	int tile_index;
	int diff_index;
	GtkWidget *image;
	GdkPixmap *preview;
	GtkWidget *size;
	GtkWidget *size_container;
	int size_cache[NUM_TILE_TYPE];
};



/*
 * Image and title (top part) of dialog
 */
static GtkWidget*
build_image_title(void)
{
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *label;
	GtkWidget *image;
	gchar *str;

	/* hbox: image and title */
	hbox= gtk_hbox_new(FALSE, 12);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);

	/* Image */
	image= gtk_image_new_from_stock(GTK_STOCK_NEW, GTK_ICON_SIZE_DIALOG);
	gtk_misc_set_alignment(GTK_MISC(image), 0.5, 0.0);
	gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
	/* vbox for main and secondary text */
	vbox= gtk_vbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);
	/* primary label "New game" */
	label= gtk_label_new(NULL);
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_label_set_selectable(GTK_LABEL(label), TRUE);
	str= g_markup_printf_escaped("<span weight=\"bold\" size=\"larger\">%s</span>",
								 _("New Game"));
	gtk_label_set_markup(GTK_LABEL(label), str);
	g_free (str);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	/* secondary label */
	str= g_strdup(_("Select game properties."));
	label= gtk_label_new(str);
	g_free(str);
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_label_set_selectable(GTK_LABEL(label), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	return hbox;
}

#include "benchmark.h"

/*
 * Build preview image
 */
static void
draw_preview_image(struct dialog_data *dialog_data)
{
	cairo_t *cr;
	struct geometry *geo;
	struct gameinfo gameinfo;

	/* create geometry for current tile type */
	switch(dialog_data->tile_index) {
	case 0:	/* square tile */
		gameinfo.type= TILE_TYPE_SQUARE;
		gameinfo.size= 5;
		break;
	case 1: /* penrose tile */
		gameinfo.type= TILE_TYPE_PENROSE;
		gameinfo.size= 1;
		break;
	case 2: /* triangle tile */
		gameinfo.type= TILE_TYPE_TRIANGULAR;
		gameinfo.size= 5;
		break;
	case 3: /* qbert tile */
		gameinfo.type= TILE_TYPE_QBERT;
		gameinfo.size= 8;
		break;
	case 4: /*  */
		gameinfo.type= TILE_TYPE_PENROSE;
		gameinfo.size= 2;
		break;
	default:
		g_message("(draw_prewiew_image) unknown tile type: %d", dialog_data->tile_index);
	};
	fences_benchmark_start();
	geo= build_board_geometry(&gameinfo);
	g_message("time: %lf", fences_benchmark_stop());

	cr= gdk_cairo_create(dialog_data->preview);
    /* set scale so we draw in board_size space */
	cairo_scale (cr,
				 PREVIEW_IMAGE_SIZE/(double)geo->board_size,
				 PREVIEW_IMAGE_SIZE/(double)geo->board_size);
	/* draw board preview */
	draw_board_skeleton(cr, geo);
	cairo_destroy (cr);

	/* destroy geometry */
	geometry_destroy(geo);
}


/*
 * Build game size widget using a spin
 */
static GtkWidget*
build_game_size_spin(const struct dialog_data *dialog_data)
{
	GtkWidget *spin;
	GtkObject *adj;

	adj= gtk_adjustment_new(dialog_data->size_cache[dialog_data->tile_index],
							5, 25, 1, 5, 0);
	spin= gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);

	return spin;
}


/*
 * Build game size widget using a combo box
 */
static GtkWidget*
build_game_size_combo(const struct dialog_data *dialog_data)
{
	GtkWidget *combo;
	int i;
	const gchar *sizes[]={N_("Tiny"),
						  N_("Small"),
						  N_("Medium"),
						  N_("Large"),
						  N_("Huge")};

	combo= gtk_combo_box_new_text();
	for(i=0; i < 5; ++i) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo), sizes[i]);
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(combo),
							 dialog_data->size_cache[dialog_data->tile_index]);

	return combo;
}


/*
 * Build gamesize widget for all different tile types
 */
static GtkWidget*
build_gamesize_widget(struct dialog_data *dialog_data)
{
	GtkWidget *widget=NULL;

	switch(dialog_data->tile_index) {
	case 0:	/* square tile */
		widget= build_game_size_spin(dialog_data);
		break;
	case 1: /* penrose tile */
		widget= build_game_size_combo(dialog_data);
		break;
	case 2: /* triangle tile */
		widget= build_game_size_spin(dialog_data);
		break;
	case 3: /* qbert tile */
		widget= build_game_size_spin(dialog_data);
		break;
	case 4: /*  */
		widget= build_game_size_combo(dialog_data);
		break;
	default:
		g_message("(build_gamesize_widget)unknown tile type:%d", dialog_data->tile_index);
	}

	return widget;
}


/*
 * Return value in size widget
 */
static int
size_widget_get_value(const struct dialog_data *dialog_data)
{
	int value=0;

	switch(dialog_data->tile_index) {
	case 0:	/* square tile */
		value= (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(dialog_data->size));
		break;
	case 1: /* penrose tile */
		value= gtk_combo_box_get_active(GTK_COMBO_BOX(dialog_data->size));
		if (value < 0) value= 2;
		break;
	case 2: /* triangle tile */
		value= gtk_spin_button_get_value(GTK_SPIN_BUTTON(dialog_data->size));
		if (value < 0) value= 2;
		break;
	case 3: /* qbert tile */
		value= gtk_spin_button_get_value(GTK_SPIN_BUTTON(dialog_data->size));
		if (value < 0) value= 2;
		break;
	case 4: /*  */
		value= gtk_combo_box_get_active(GTK_COMBO_BOX(dialog_data->size));
		if (value < 0) value= 2;
		break;
	default:
		g_message("(size_widget_get_value) unknown tile type: %d", dialog_data->tile_index);
	};
	return value;
}


/*
 * Callback for when a tile type radio is toggled
 */
static void
tiletype_radio_cb(GtkToggleButton *button, gpointer user_data)
{
	struct dialog_data *dialog_data=(struct dialog_data*)user_data;
	int i=0;

	/* ignore early callback when default is set */
	if (GTK_WIDGET_REALIZED(button) == FALSE)
		return;
	if (!gtk_toggle_button_get_active(button))
		return;

	/* get current setting from size widget */
	dialog_data->size_cache[dialog_data->tile_index]=
		size_widget_get_value(dialog_data);

	/* find which button was selected */
	for(i=0; i < NUM_TILE_TYPE; ++i) {
		if (GTK_WIDGET(button) == dialog_data->tile_button[i]) break;
	}
	dialog_data->tile_index= i;

	/* redraw tile to new type */
	draw_preview_image(dialog_data);
	gtk_widget_queue_draw(dialog_data->image);

	/* change tile size widget */
	gtk_widget_destroy(dialog_data->size);
	dialog_data->size= build_gamesize_widget(dialog_data);
	gtk_box_pack_start(GTK_BOX(dialog_data->size_container), dialog_data->size,
					   FALSE, FALSE, 0);
	gtk_widget_show(dialog_data->size);
}


/*
 * Game type and properties widget
 */
static GtkWidget*
build_tile_type_properties(struct dialog_data *dialog_data)
{
	GtkWidget *frame;
	GtkWidget *mainvbox;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *radio;
	GtkWidget *image;
	GdkPixmap *pixmap;
	GtkWidget *label;
	int i;
	const gchar *tiles[]={N_("Square"),
						  N_("Penrose"),
						  N_("Triangular"),
						  N_("Qbert"),
						  N_("Hexagon")};

	/* preview image */
	pixmap= gdk_pixmap_new(NULL, PREVIEW_IMAGE_SIZE, PREVIEW_IMAGE_SIZE, 24);
	dialog_data->preview= pixmap;

	/* frame: game type and properties */
	frame= gtk_frame_new(_("Tile Type"));

	/* main vbox */
	mainvbox= gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(mainvbox), 5);
	gtk_container_add(GTK_CONTAINER(frame), mainvbox);

	/* hbox: left radios and right preview */
	hbox= gtk_hbox_new(FALSE, 5);
	//gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
	gtk_box_pack_start(GTK_BOX(mainvbox), hbox, FALSE, FALSE, 0);
	/* vbox with game types (radios) */
	vbox= gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);
	radio= gtk_radio_button_new_with_label(NULL, tiles[0]);
	gtk_box_pack_start(GTK_BOX(vbox), radio, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(radio), "toggled",
					 G_CALLBACK(tiletype_radio_cb), dialog_data);
	dialog_data->tile_button[0]= radio;
	for(i=1; i < NUM_TILE_TYPE; ++i) {
		radio= gtk_radio_button_new_with_label_from_widget(
			GTK_RADIO_BUTTON(radio), tiles[i]);
		gtk_box_pack_start(GTK_BOX(vbox), radio, FALSE, FALSE, 0);
		g_signal_connect(G_OBJECT(radio), "toggled",
						 G_CALLBACK(tiletype_radio_cb), dialog_data);
		dialog_data->tile_button[i]= radio;
	}

	/* preview image */
	image= gtk_image_new_from_pixmap(pixmap, NULL);
	gtk_misc_set_alignment(GTK_MISC(image), 0.5, 0.5);
	g_object_unref(pixmap);
	draw_preview_image(dialog_data);
	gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 5);
	dialog_data->image= image;

	/* game size */
	hbox= gtk_hbox_new(FALSE, 15);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
	gtk_box_pack_start(GTK_BOX(mainvbox), hbox, FALSE, FALSE, 0);
	label= gtk_label_new(_("Game Size:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	dialog_data->size= build_gamesize_widget(dialog_data);
	gtk_box_pack_start(GTK_BOX(hbox), dialog_data->size, FALSE, FALSE, 0);

	/* store container that hold size widget, i.e. hbox */
	dialog_data->size_container= hbox;

	/* set default tile setting */
	i= dialog_data->tile_index;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog_data->tile_button[i]), TRUE);

	return frame;
}


/*
 * Callback when a difficulty setting radio button is toggled
 */
static void
difficulty_radio_cb(GtkToggleButton *button, gpointer user_data)
{
	struct dialog_data *dialog_data=(struct dialog_data*)user_data;
	int i=0;

	/* ignore early callback when default is set */
	if (GTK_WIDGET_REALIZED(button) == FALSE)
		return;
	if (!gtk_toggle_button_get_active(button))
		return;

	/* find which button was selected */
	for(i=0; i < NUM_DIFFICULTY; ++i) {
		if (GTK_WIDGET(button) == dialog_data->diff_button[i]) break;
	}
	dialog_data->diff_index= i;
}


/*
 * Build difficulty settings
 */
static GtkWidget*
build_difficulty_settings(struct dialog_data *dialog_data)
{
	GtkWidget *frame;
	GtkWidget *vbox;
	const gchar *diffs[]={N_("Beginner"),
						  N_("Easy"),
						  N_("Normal"),
						  N_("Hard"),
						  N_("Expert"),
						  N_("Impossible")};
	int i;
	GtkWidget *radio;

	frame= gtk_frame_new(_("Difficulty"));

	vbox= gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	radio= gtk_radio_button_new_with_label(NULL, diffs[0]);
	gtk_box_pack_start(GTK_BOX(vbox), radio, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(radio), "toggled",
					 G_CALLBACK(difficulty_radio_cb), dialog_data);
	dialog_data->diff_button[0]= radio;
	for(i=1; i < NUM_DIFFICULTY; ++i) {
		radio= gtk_radio_button_new_with_label_from_widget(
			GTK_RADIO_BUTTON(radio), diffs[i]);
		gtk_box_pack_start(GTK_BOX(vbox), radio, FALSE, FALSE, 0);
		g_signal_connect(G_OBJECT(radio), "toggled",
					 G_CALLBACK(difficulty_radio_cb), dialog_data);
		dialog_data->diff_button[i]= radio;
	}

	/* set default setting */
	i= dialog_data->diff_index;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog_data->diff_button[i]), TRUE);

	return frame;
}


/*
 * Fill gameinfo from dialog_data
 */
static void
extract_game_info(const struct dialog_data *dialog_data, struct gameinfo *info)
{
	int i;

	switch(dialog_data->tile_index) {
	case 0:	/* square tile */
		info->type= TILE_TYPE_SQUARE;
		break;
	case 1: /* penrose tile */
		info->type= TILE_TYPE_PENROSE;
		break;
	case 2: /* triangle tile */
		info->type= TILE_TYPE_TRIANGULAR;
		break;
	case 3: /* qbert tile */
		info->type= TILE_TYPE_QBERT;
		break;
	case 4: /*  */
		info->type= TILE_TYPE_PENROSE;
		break;
	default:
		g_message("(extract_game_info) unknown tile type: %d", dialog_data->tile_index);
	};
	info->size= size_widget_get_value(dialog_data);
	info->diff_index= dialog_data->diff_index;
}



/*
 * Setup initial dialog_data
 */
static void
setup_dialog_data(const struct gameinfo *info, struct dialog_data *dialog_data)
{
	const int default_cache[]={7, 2, 2 ,2 ,2};

	/* copy default size cache */
	memcpy(dialog_data->size_cache, default_cache, NUM_TILE_TYPE*sizeof(int));

	switch(info->type) {
	case TILE_TYPE_SQUARE:
		dialog_data->tile_index= 0;
		dialog_data->size_cache[0]= info->size;
		break;
	case TILE_TYPE_PENROSE:
		dialog_data->tile_index= 1;
		dialog_data->size_cache[1]= info->size;
		break;
	case TILE_TYPE_TRIANGULAR:
		dialog_data->tile_index= 2;
		dialog_data->size_cache[2]= info->size;
		break;
	case TILE_TYPE_QBERT:
		dialog_data->tile_index= 3;
		dialog_data->size_cache[3]= info->size;
		break;
	default:
		g_message("(setup_dialog_data) unknown tile type: %d", info->type);
	}
	dialog_data->diff_index= info->diff_index;
	dialog_data->size= NULL;
}


/*
 * New game dialog
 */
gboolean
fencesgui_newgame_dialog(struct board *board, struct gameinfo *info)
{
	GtkWidget *dialog;
	GtkWidget *hbox;
	GtkWidget *vbox;
	gint response;
	GtkBox *dlg_vbox;
	GtkWidget *widget;
	struct dialog_data dialog_data;

	/* setup dialog_data */
	setup_dialog_data(&board->gameinfo, &dialog_data);

	/* create dialog */
	dialog= gtk_dialog_new_with_buttons(
		"", GTK_WINDOW(board->window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
		GTK_STOCK_CANCEL, GTK_RESPONSE_NO,
		GTK_STOCK_NEW, GTK_RESPONSE_YES, NULL);
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
	gtk_box_set_spacing(GTK_BOX(GTK_DIALOG(dialog)->vbox), 14);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
	dlg_vbox= GTK_BOX(GTK_DIALOG(dialog)->vbox);

	/* Image and title */
	widget= build_image_title();
	gtk_box_pack_start(dlg_vbox, widget, FALSE, FALSE, 10);

	/* vbox with main content */
	vbox= gtk_vbox_new(FALSE, 5);
	gtk_box_pack_start(dlg_vbox, vbox, FALSE, FALSE, 0);

	/* hbox containing the frames for game type and difficulty*/
	hbox= gtk_hbox_new(FALSE, 8);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	/* game type and properties */
	widget= build_tile_type_properties(&dialog_data);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);

	/* difficulty settings */
	widget= build_difficulty_settings(&dialog_data);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);

	g_object_set_data(G_OBJECT(dialog), "dialog-data", &dialog_data);

	/* show all */
	gtk_widget_show_all (dialog);

	/* run dialog */
	response= gtk_dialog_run(GTK_DIALOG(dialog));

	/* extract game info */
	extract_game_info(&dialog_data, info);
	g_message("tile:%d ; diff:%d ; size:%d", dialog_data.tile_index, dialog_data.diff_index, info->size);

	/* destroy dialog */
	gtk_widget_destroy(dialog);

	return response == GTK_RESPONSE_YES;
}
