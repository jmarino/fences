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
#define NUM_SIZE_BUTTONS	6


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

/* names of tile types */
const gchar *tiletype_name[]={
	N_("Square"),
	N_("Penrose"),
	N_("Triangular"),
	N_("Qbert"),
	N_("Hexagon"),
	N_("Snub"),
	N_("Cairo"),
	N_("Cartwheel"),
	N_("Trihex")
};

static gboolean allow_custom_size[NUMBER_TILE_TYPE]={
	TRUE,		// square
	FALSE,		// penrose
	TRUE,		// triangular
	TRUE,		// qbert
	TRUE,		// hex
	FALSE,		// snub
	FALSE,		// cairo
	FALSE,		// cartwheel
	FALSE		// trihex
};


/* Default widget sizes */
static int initial_size_index[]={
	1,		// square
	2,		// penrose
	1,		// triangular
	1,		// qbert
	1,		// hexagonal
	1,		// snub
	2,		// cairo
	2,		// cartwheel
	1		// trihex
};

/* names of tile sizes */
const gchar *tilesize_name[]={
	N_("Tiny"),
	N_("Small"),
	N_("Medium"),
	N_("Large"),
	N_("Huge"),
	N_("Custom")
};

/* names for game difficulties */
const gchar *difficulty_name[]={
	N_("Beginner"),
	N_("Easy"),
	N_("Normal"),
	N_("Hard"),
	N_("Expert"),
	N_("Impossible")
};

/* how game size index "Tiny,Small,..." relates to actual size for
 * tiles that need a size number (such as Square, Triangular, ...) */
static int index2size[]={5, 10, 15, 20, 25};


/*
 * Data associated with dialog
 */
struct dialog_data {
	GtkWidget *tile_button[NUMBER_TILE_TYPE];	// tile type radio buttons
	GtkWidget *size_button[NUM_SIZE_BUTTONS];	// sizes radio buttons
	GtkWidget *diff_button[NUM_DIFFICULTY];		// difficulty radio buttons
	int tile_index;								// tile button selected
	int size_index;								// size button selected
	int diff_index;								// difficulty button selected
	int custom_size;							// current custom size
	GtkWidget *image;							// image widget (used to schedule redraws)
	GdkPixmap *preview;							// preview pixmap to draw on
	GtkWidget *custom_spin;						// spin button (custom size)
	int size_index_cache[NUMBER_TILE_TYPE];		// cached size buttons selected
	int custom_cache[NUMBER_TILE_TYPE];			// cached custom sizes
	gboolean cb_lock;							// lock to avoid nested callbacks
};



/*
 * Return actual size of tile, as it needs to be set in gameinfo.
 */
static int
tilesize_get_size(const struct dialog_data *dialog_data)
{
	/* need to transform index into actual size? */
	if (dialog_data->size_index < NUM_SIZE_BUTTONS - 1 &&
		allow_custom_size[dialog_data->tile_index] == TRUE) {
		return index2size[dialog_data->size_index];
	}
	/* return custom value */
	if (dialog_data->size_index == NUM_SIZE_BUTTONS - 1) {
		return dialog_data->custom_size;
	}

	return dialog_data->size_index;
}


#include "benchmark.h"

/*
 * Build preview image
 */
static void
draw_preview_image(struct dialog_data *dialog_data)
{
	cairo_t *cr;
	struct geometry *geo_skel;
	struct gameinfo gameinfo;

	/* create geometry for current tile type */
	gameinfo.type= index2tiletype[dialog_data->tile_index];
	gameinfo.size= tilesize_get_size(dialog_data);

	/* build preview geometry */
	fences_benchmark_start();
	geo_skel= build_tile_skeleton(&gameinfo);
	g_message("tile creation time (preview): %lf", fences_benchmark_stop());

	cr= gdk_cairo_create(dialog_data->preview);
    /* set scale so we draw in board_size space */
	cairo_scale (cr,
				 PREVIEW_IMAGE_SIZE/(double)geo_skel->board_size,
				 PREVIEW_IMAGE_SIZE/(double)geo_skel->board_size);
	/* draw board preview */
	draw_board_skeleton(cr, geo_skel);
	cairo_destroy (cr);

	/* destroy geometry */
	geometry_destroy(geo_skel);
}


/*
 * Callback called when custom size spin is changed.
 * Updates custom size setting and redraws preview image.
 */
static void
custom_size_spin_changed_cb(GtkWidget *widget, gpointer user_data)
{
	struct dialog_data *dialog_data=(struct dialog_data*)user_data;

	if (dialog_data->cb_lock) return;

	dialog_data->custom_size= (int)gtk_spin_button_get_value(
			GTK_SPIN_BUTTON(dialog_data->custom_spin));
	draw_preview_image(dialog_data);
	gtk_widget_queue_draw(dialog_data->image);
}


/*
 * Callback called when the tile size has been changed.
 * Makes sure the preview image is redrawn.
 */
static void
tile_size_radio_changed_cb(GtkToggleButton *button, gpointer user_data)
{
	struct dialog_data *dialog_data=(struct dialog_data*)user_data;
	int i;

	/* ignore early callback when default is set */
	if (GTK_WIDGET_REALIZED(button) == FALSE)
		return;
	if (!gtk_toggle_button_get_active(button))
		return;
	if (dialog_data->cb_lock) return;

	/* find which button was selected */
	for(i=0; i < NUM_SIZE_BUTTONS; ++i) {
		if (GTK_WIDGET(button) == dialog_data->size_button[i]) break;
	}
	g_assert(i < NUM_SIZE_BUTTONS);

	dialog_data->cb_lock= TRUE;			/* disable nested callbacks */

	/* disable spin if we're leaving custom setting */
	if (dialog_data->size_index == NUM_SIZE_BUTTONS - 1)
		gtk_widget_set_sensitive(dialog_data->custom_spin, FALSE);

	dialog_data->size_index= i;

	/* enable spin if we're now in custom setting */
	if (dialog_data->size_index == NUM_SIZE_BUTTONS - 1) {
		gtk_widget_set_sensitive(dialog_data->custom_spin, TRUE);
	} else {
		if (allow_custom_size[dialog_data->tile_index]) {
			dialog_data->custom_size= tilesize_get_size(dialog_data);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog_data->custom_spin),
									  dialog_data->custom_size);
		}
	}

	draw_preview_image(dialog_data);
	gtk_widget_queue_draw(dialog_data->image);

	dialog_data->cb_lock= FALSE;		/* done. Reenable callbacks */
}


/*
 * Callback for when a tile type radio is toggled
 */
static void
tile_type_radio_cb(GtkToggleButton *button, gpointer user_data)
{
	struct dialog_data *dialog_data=(struct dialog_data*)user_data;
	int i=0;

	/* ignore early callback when default is set */
	if (GTK_WIDGET_REALIZED(button) == FALSE)
		return;
	if (!gtk_toggle_button_get_active(button))
		return;

	/* store current size settings in cache */
	dialog_data->size_index_cache[dialog_data->tile_index]=
		dialog_data->size_index;
	dialog_data->custom_cache[dialog_data->tile_index]=
		dialog_data->custom_size;

	/* find which button was selected */
	for(i=0; i < NUMBER_TILE_TYPE; ++i) {
		if (GTK_WIDGET(button) == dialog_data->tile_button[i]) break;
	}
	dialog_data->tile_index= i;

	dialog_data->cb_lock= TRUE;			/* disable nested callbacks */

	/* update size settings */
	dialog_data->size_index= dialog_data->size_index_cache[i];
	dialog_data->custom_size= dialog_data->custom_cache[i];
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(dialog_data->size_button[dialog_data->size_index]),
		TRUE);
	gtk_widget_set_sensitive(dialog_data->size_button[5],
							 allow_custom_size[i]);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog_data->custom_spin),
							  dialog_data->custom_size);
	if (dialog_data->size_index == NUM_SIZE_BUTTONS - 1) { 	// custom size
		gtk_widget_set_sensitive(dialog_data->custom_spin, TRUE);
	} else {
		gtk_widget_set_sensitive(dialog_data->custom_spin, FALSE);
	}

	/* redraw tile to new type */
	draw_preview_image(dialog_data);
	gtk_widget_queue_draw(dialog_data->image);

	dialog_data->cb_lock= FALSE;		/* done. Reenable callbacks */
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
 * Image and title (top part header) of dialog
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
	str= g_markup_printf_escaped(
		"<span weight=\"bold\" size=\"larger\">%s</span>", _("New Game"));
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


/*
 * Frame with possible tile sizes
 */
static GtkWidget*
build_tile_size_frame(struct dialog_data *dialog_data)
{
	GtkWidget *frame;
	GtkWidget *vbox;
	GtkWidget *radio=NULL;
	GtkWidget *spin;
	GtkObject *adj;
	int i;

	/* frame: game type and properties */
	frame= gtk_frame_new(_("Size"));

	/* main vbox */
	vbox= gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
	gtk_container_add(GTK_CONTAINER(frame), vbox);

	for(i=0; i < NUM_SIZE_BUTTONS; ++i) {
		radio= gtk_radio_button_new_with_label_from_widget(
			GTK_RADIO_BUTTON(radio), tilesize_name[i]);
		gtk_box_pack_start(GTK_BOX(vbox), radio, FALSE, FALSE, 0);
		g_signal_connect(G_OBJECT(radio), "toggled",
						 G_CALLBACK(tile_size_radio_changed_cb), dialog_data);
		dialog_data->size_button[i]= radio;
	}
	/* add custom spin button */
	adj= gtk_adjustment_new(dialog_data->custom_size, 5, 25, 1, 5, 0);
	spin= gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
	g_signal_connect(G_OBJECT(spin), "value-changed",
					 G_CALLBACK(custom_size_spin_changed_cb), dialog_data);
	gtk_box_pack_start(GTK_BOX(vbox), spin, FALSE, FALSE, 0);
	dialog_data->custom_spin= spin;

	gtk_widget_set_sensitive(radio,
							 allow_custom_size[dialog_data->tile_index]);
	if (dialog_data->size_index < NUM_SIZE_BUTTONS - 1) {
		gtk_widget_set_sensitive(spin, FALSE);
	}

	/* set initial size from cache */
	radio= dialog_data->size_button[dialog_data->size_index];
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio), TRUE);

	return frame;
}


/*
 * Game type frame
 */
static GtkWidget*
build_tile_type_frame(struct dialog_data *dialog_data)
{
	GtkWidget *frame;
	GtkWidget *mainvbox;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *radio=NULL;
	GtkWidget *image;
	GdkPixmap *pixmap;
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

	for(i=0; i < NUMBER_TILE_TYPE; ++i) {
		radio= gtk_radio_button_new_with_label_from_widget(
			GTK_RADIO_BUTTON(radio), tiletype_name[i]);
		gtk_box_pack_start(GTK_BOX(vbox), radio, FALSE, FALSE, 0);
		g_signal_connect(G_OBJECT(radio), "toggled",
						 G_CALLBACK(tile_type_radio_cb), dialog_data);
		dialog_data->tile_button[i]= radio;
	}

	/* preview image */
	image= gtk_image_new_from_pixmap(pixmap, NULL);
	gtk_misc_set_alignment(GTK_MISC(image), 0.5, 0.5);
	g_object_unref(pixmap);
	draw_preview_image(dialog_data);
	gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 5);
	dialog_data->image= image;

	/* set default tile setting */
	i= dialog_data->tile_index;
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(dialog_data->tile_button[i]), TRUE);

	return frame;
}


/*
 * Build difficulty settings
 */
static GtkWidget*
build_difficulty_settings(struct dialog_data *dialog_data)
{
	GtkWidget *frame;
	GtkWidget *vbox;
	int i;
	GtkWidget *radio=NULL;

	frame= gtk_frame_new(_("Difficulty"));

	vbox= gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
	gtk_container_add(GTK_CONTAINER(frame), vbox);

	for(i=0; i < NUM_DIFFICULTY; ++i) {
		radio= gtk_radio_button_new_with_label_from_widget(
			GTK_RADIO_BUTTON(radio), difficulty_name[i]);
		gtk_box_pack_start(GTK_BOX(vbox), radio, FALSE, FALSE, 0);
		g_signal_connect(G_OBJECT(radio), "toggled",
					 G_CALLBACK(difficulty_radio_cb), dialog_data);
		dialog_data->diff_button[i]= radio;
	}

	/* set default setting */
	i= dialog_data->diff_index;
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(dialog_data->diff_button[i]), TRUE);

	return frame;
}


/*
 * Fill gameinfo from dialog_data
 */
static void
extract_game_info(const struct dialog_data *dialog_data, struct gameinfo *info)
{
	info->type= index2tiletype[dialog_data->tile_index];
	info->size= tilesize_get_size(dialog_data);
	info->diff_index= dialog_data->diff_index;
}



/*
 * Setup initial dialog_data
 */
static void
setup_dialog_data(const struct gameinfo *info, struct dialog_data *dialog_data)
{
	int i;

	/* copy initial size cache */
	memcpy(dialog_data->size_index_cache, initial_size_index,
		   NUMBER_TILE_TYPE*sizeof(int));
	/* set initial cache of custom sizes */
	for(i=0; i < NUMBER_TILE_TYPE; ++i) {
		if (allow_custom_size[i])
			dialog_data->custom_cache[i]= index2size[initial_size_index[i]];
		else dialog_data->custom_cache[i]= 0;
	}

	/* find and set current tile type */
	for(i=0; i < NUMBER_TILE_TYPE; ++i) {
		if (index2tiletype[i] == info->type) {
			dialog_data->tile_index= i;
			break;
		}
	}
	g_assert(i < NUMBER_TILE_TYPE);

	/* set current dialog size settings */
	if (allow_custom_size[i]) {
		dialog_data->custom_size= info->size;
		for(i=0; i < NUM_SIZE_BUTTONS - 1; ++i)
			if (info->size == index2size[i]) break;
		dialog_data->size_index= i;	// last one set if not found (i.e. custom)
	} else {
		dialog_data->size_index= info->size;
	}

	/* set current difficulty */
	dialog_data->diff_index= info->diff_index;

	dialog_data->cb_lock= FALSE;		// callback lock disabled
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

	/* game type frame */
	widget= build_tile_type_frame(&dialog_data);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);

	/* game size frame */
	widget= build_tile_size_frame(&dialog_data);
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
	g_message("tile:%d ; diff:%d ; size:%d", dialog_data.tile_index,
			  dialog_data.diff_index, info->size);

	/* destroy dialog */
	gtk_widget_destroy(dialog);

	return response == GTK_RESPONSE_YES;
}
