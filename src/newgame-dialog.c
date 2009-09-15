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

#define NUM_DIFFICULTY		6


/*
 * Relate tile index (in dialog) to tile type
 */
static int index2tiletype[NUMBER_TILE_TYPE]={
	TILE_TYPE_SQUARE,
	TILE_TYPE_PENROSE,
	TILE_TYPE_TRIANGULAR,
	TILE_TYPE_QBERT,
	TILE_TYPE_HEX,
	TILE_TYPE_SNUB,
	TILE_TYPE_CAIRO,
	TILE_TYPE_CARTWHEEL,
	TILE_TYPE_TRIHEX
};

const gchar *tiletype_name[]={N_("Square"),
							  N_("Penrose"),
							  N_("Triangular"),
							  N_("Qbert"),
							  N_("Hexagon"),
							  N_("Snub"),
							  N_("Cairo"),
							  N_("Cartwheel"),
							  N_("Trihex")
};

/*
 * Size widget type
 */
enum {
	SIZE_WIDGET_SPIN,
	SIZE_WIDGET_COMBO
};
static int size_widget_type[]={
	SIZE_WIDGET_SPIN,	// square
	SIZE_WIDGET_COMBO,	// penrose
	SIZE_WIDGET_SPIN,	// triangular
	SIZE_WIDGET_SPIN,	// qbert
	SIZE_WIDGET_SPIN,	// hexagonal
	SIZE_WIDGET_COMBO,	// snub
	SIZE_WIDGET_COMBO,	// cairo
	SIZE_WIDGET_COMBO,	// cartwheel
	SIZE_WIDGET_COMBO	// trihex
};

/* Default widget sizes */
static int default_sizes[]={
	7,		// square
	2,		// penrose
	7,		// triangular
	7,		// qbert
	7,		// hexagonal
	2,		// snub
	2,		// cairo
	2,		// cartwheel
	2		// trihex
};

/* Sizes of board to show in the preview */
static const int preview_sizes[]={
	5,		// square
	1,		// penrose
	5,		// triangular
	8,		// qbert
	5,		// hexagonal
	1,		// snub
	1,		// cairo
	1,		// cartwheel
	1		// trihex
};


/*
 * Data associated with dialog
 */
struct dialog_data {
	GtkWidget *tile_button[NUMBER_TILE_TYPE];
	GtkWidget *diff_button[NUM_DIFFICULTY];
	int tile_index;
	int diff_index;
	GtkWidget *image;
	GdkPixmap *preview;
	GtkWidget *size;
	GtkWidget *size_container;
	int size_cache[NUMBER_TILE_TYPE];
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
	gameinfo.type= index2tiletype[dialog_data->tile_index];
	gameinfo.size= preview_sizes[dialog_data->tile_index];

	/* build preview geometry */
	fences_benchmark_start();
	geo= build_geometry_tile(&gameinfo);
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

	switch(size_widget_type[dialog_data->tile_index]) {
	case SIZE_WIDGET_SPIN:
		widget= build_game_size_spin(dialog_data);
		break;
	case SIZE_WIDGET_COMBO:
		widget= build_game_size_combo(dialog_data);
		break;
	default:
		g_message("(build_gamesize_widget) unknown size widget type: %d",
				  size_widget_type[dialog_data->tile_index]);
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

	switch(size_widget_type[dialog_data->tile_index]) {
	case SIZE_WIDGET_SPIN:
		value= (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(dialog_data->size));
		break;
	case SIZE_WIDGET_COMBO:
		value= gtk_combo_box_get_active(GTK_COMBO_BOX(dialog_data->size));
		if (value < 0) value= 2;
		break;
	default:
		g_message("(size_widget_get_value) unknown size widget type: %d",
				  size_widget_type[dialog_data->tile_index]);
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
	for(i=0; i < NUMBER_TILE_TYPE; ++i) {
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
	radio= gtk_radio_button_new_with_label(NULL, tiletype_name[0]);
	gtk_box_pack_start(GTK_BOX(vbox), radio, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(radio), "toggled",
					 G_CALLBACK(tiletype_radio_cb), dialog_data);
	dialog_data->tile_button[0]= radio;
	for(i=1; i < NUMBER_TILE_TYPE; ++i) {
		radio= gtk_radio_button_new_with_label_from_widget(
			GTK_RADIO_BUTTON(radio), tiletype_name[i]);
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
	info->type= index2tiletype[dialog_data->tile_index];
	info->size= size_widget_get_value(dialog_data);
	info->diff_index= dialog_data->diff_index;
}



/*
 * Setup initial dialog_data
 */
static void
setup_dialog_data(const struct gameinfo *info, struct dialog_data *dialog_data)
{

	int i;

	/* copy default size cache */
	memcpy(dialog_data->size_cache, default_sizes, NUMBER_TILE_TYPE*sizeof(int));

	for(i=0; i < NUMBER_TILE_TYPE; ++i) {
		if (index2tiletype[i] == info->type) {
			dialog_data->tile_index= i;
			break;
		}
	}
	g_assert(i < NUMBER_TILE_TYPE);

	dialog_data->size_cache[i]= info->size;
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
