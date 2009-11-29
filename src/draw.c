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
#include <math.h>
#include <stdlib.h>
#include <sys/time.h>

#include "gamedata.h"


/* defined in gamedata.c */
extern struct board board;


/* HACK: only used by debug draw routines */
#define OFF_LINE_WIDTH		0.5/500.0*board.geo->board_size
#define DOT_RADIUS		3.5/500.0*board.geo->board_size



/*
 * DEBUG: Hack to display click mesh as a green grid
 */
static void
draw_tiles(cairo_t *cr)
{
	int i, j;
	double width=board.click_mesh->tile_size;

	//g_message("draw_tiles");
	cairo_set_source_rgba(cr, 0., 1., 0., 0.2);
	cairo_set_line_width (cr, OFF_LINE_WIDTH);
	for(i=0; i < 10; ++i) {
		for(j=0; j < 10; ++j) {
			cairo_rectangle (cr, i*width, j*width, width, width);
			cairo_stroke(cr);
		}
	}
}


/*
 * DEBUG: Hack to display lines area of influence
 */
static void
draw_areainf(cairo_t *cr)
{
	int i, j;
	struct line *lin;

	cairo_set_source_rgba(cr, 0., 1., 0., 0.2);
	cairo_set_line_width (cr, OFF_LINE_WIDTH);
	lin= board.geo->lines;
	for(i=0; i < board.geo->nlines; ++i) {
		cairo_move_to(cr, lin->inf[0].x, lin->inf[0].y);
		for(j=1; j < 4; ++j) {
			cairo_line_to(cr, lin->inf[j].x, lin->inf[j].y);

		}
		cairo_line_to(cr, lin->inf[0].x, lin->inf[0].y);
		++lin;
	}
	cairo_stroke(cr);
}


/*
 * DEBUG: Hack to display tile centers
 */
static void
draw_tile_centers(cairo_t *cr)
{
	int i;
	struct tile *tile;

	cairo_set_source_rgba(cr, 0., 1., 0., 0.2);
	tile= board.geo->tiles;
	for(i=0; i < board.geo->ntiles; ++i) {
		cairo_new_sub_path(cr);
		cairo_arc (cr, tile->center.x, tile->center.y, 2*DOT_RADIUS,
			   0, 2 * M_PI);
		++tile;
	}
	cairo_fill(cr);
}


/*
 * DEBUG: Hack to display which tiles are associated with each line
 */
static void
draw_linetiles(cairo_t *cr)
{
	int i;
	struct line *lin;
	double x, y;

	cairo_set_source_rgba(cr, 0., 1., 0., 0.4);
	cairo_set_line_width (cr, OFF_LINE_WIDTH);
	lin= board.geo->lines;
	for(i=0; i < board.geo->nlines; ++i) {
		/* find middle of line */
		x= (lin->ends[0]->pos.x + lin->ends[1]->pos.x)/2.;
		y= (lin->ends[0]->pos.y + lin->ends[1]->pos.y)/2.;
		cairo_move_to(cr, x, y);
		cairo_line_to(cr, lin->tiles[0]->center.x, lin->tiles[0]->center.y);
		if (lin->ntiles == 2) {
			cairo_move_to(cr, x, y);
			cairo_line_to(cr, lin->tiles[1]->center.x, lin->tiles[1]->center.y);
		}
		++lin;
	}
	cairo_stroke(cr);
}


/*
 * DEBUG: Hack to display center of screen and penrose clip circle
 */
static void
draw_bounds(cairo_t *cr)
{
	/* debug: draw dot in the middle of board */
	cairo_set_source_rgba(cr, 0, 1., 0, 0.2);
	cairo_arc(cr, board.geo->board_size/2., board.geo->board_size/2., DOT_RADIUS,
		  0, 2 * M_PI);
	cairo_fill(cr);

	/* debug: draw circle showing clipping */
	cairo_set_line_width (cr, OFF_LINE_WIDTH);
	cairo_set_source_rgba(cr, 0, 1., 0, 0.4);
	cairo_arc(cr, board.geo->board_size/2., board.geo->board_size/2., board.geo->game_size/2.,
		  0, 2 * M_PI);
	cairo_stroke(cr);
}


/*
 * DEBUG: Hack to display margin in game board
 */
static void
draw_margin(cairo_t *cr)
{
	/* debug: draw rectangle for margin */
	cairo_set_line_width (cr, OFF_LINE_WIDTH);
	cairo_set_source_rgba(cr, 0, 1., 0, 0.4);
	cairo_rectangle(cr, board.geo->board_margin, board.geo->board_margin,
					board.geo->game_size, board.geo->game_size);
	cairo_stroke(cr);
}


/*
 * Select color according to FX status and frame
 */
static void
fx_setcolor(cairo_t *cr, struct line *line)
{
	switch(line->fx_status) {
		case 0: //FX_OFF:
			cairo_set_source_rgb(cr, 0., 0., 1.);
		break;
		case 1://FX_LOOP:
			cairo_set_source_rgb(cr,
					     0.2 + 0.8*sin(line->fx_frame/20.0*M_PI),
					     0., 1.);
		break;
		default:
			g_debug("line %d, unknown FX: %d", line->id, line->fx_status);
	}
}


/*
 * Increase frame number for FX animation
 */
static void
fx_nextframe(struct line *line)
{
	switch(line->fx_status) {
		case 0: //FX_OFF
			return;
		break;
		case 1://FX_LOOP:
			gdk_threads_enter();
			line->fx_frame= (line->fx_frame + 1)%20;
			gdk_threads_leave();
		break;
		default:
			g_debug("line %d, unknown FX: %d", line->id, line->fx_status);
	}
}


/*
 * Draw game on board
 * Cairo context is assumed to be properly scaled to board units,
 * i.e., we draw in 'board_size' units.
 */
void
draw_board(cairo_t *cr, struct geometry *geo, struct game *game)
{
	struct vertex *vertex1, *vertex2;
	struct line *line;
	struct tile *tile;
	int i, j;
	double x, y;
	int lines_on;	// how many ON lines a vertex has
	int number;

	/* white background */
	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_paint(cr);

	// debug
	//draw_tiles(cr);
	//draw_areainf(cr);
	//draw_tile_centers(cr);
	//draw_linetiles(cr);
	//draw_bounds(cr);
	//draw_margin(cr);

	/* Draw OFF lines first */
	cairo_set_source_rgb(cr, 150/256., 150/256., 150/256.);
	cairo_set_line_width (cr, geo->off_line_width);
	line= geo->lines;
	for(i=0; i<geo->nlines; ++i) {
		vertex1= line->ends[0];
		vertex2= line->ends[1];
		if (game->states[line->id] != LINE_ON) {
			cairo_move_to(cr, vertex1->pos.x, vertex1->pos.y);
			cairo_line_to(cr, vertex2->pos.x, vertex2->pos.y);
		}
		++line;
	}
	cairo_stroke(cr);

	/* Draw lines */
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
	line= geo->lines;
	for(i=0; i<geo->nlines; ++i) {
		vertex1= line->ends[0];
		vertex2= line->ends[1];
		if (game->states[line->id] == LINE_CROSSED) { // draw cross
			cairo_set_source_rgb(cr, 1., 0., 0.);
			cairo_set_line_width (cr, geo->cross_line_width);
			x= (vertex1->pos.x + vertex2->pos.x)/2.;
			y= (vertex1->pos.y + vertex2->pos.y)/2.;
			cairo_move_to(cr, x-geo->cross_radius, y-geo->cross_radius);
			cairo_line_to(cr, x+geo->cross_radius, y+geo->cross_radius);
			cairo_move_to(cr, x-geo->cross_radius, y+geo->cross_radius);
			cairo_line_to(cr, x+geo->cross_radius, y-geo->cross_radius);
			cairo_stroke(cr);
		} else if (game->states[line->id] == LINE_ON) {
			fx_setcolor(cr, line);
			//cairo_set_source_rgb(cr, 0., 0., 1.);
			cairo_set_line_width (cr, geo->on_line_width);
			cairo_move_to(cr, vertex1->pos.x, vertex1->pos.y);
			cairo_line_to(cr, vertex2->pos.x, vertex2->pos.y);
			cairo_stroke(cr);
			//fx_nextframe(line);
		} else if (game->states[line->id] != LINE_OFF) {
			g_debug("draw_line: line (%d) state invalid: %d",
				line->id, game->states[line->id]);
		}
		++line;
	}
	//cairo_stroke(cr);

	/* Draw vertexs */
	if (0) {
		vertex1= geo->vertex;
		for(i=0; i < geo->nvertex; ++i) {
			/* count how many lines on are touching this */
			lines_on= 0;
			for(j=0; j < vertex1->nlines; ++j) {
				if (game->states[vertex1->lines[j]->id] == LINE_ON)
					++lines_on;
			}
			/* draw vertex */
			if (lines_on == 2) cairo_set_source_rgb(cr, 0, 0, 1);
			else cairo_set_source_rgb(cr, 0, 0, 0);
			cairo_arc (cr, vertex1->pos.x, vertex1->pos.y, DOT_RADIUS,
				   0, 2 * M_PI);
			cairo_fill(cr);
			++vertex1;
		}
	}

	/* Text in tiles */
	tile= geo->tiles;
	cairo_set_font_size(cr, geo->font_size);
	for(i=0; i<geo->ntiles; ++i) {
		number= game->numbers[tile->id];
		if (number != -1) {	// tile has a number
			if (tile->display_state == DISPLAY_NORMAL) {
				cairo_set_source_rgb(cr, 0, 0, 0);
			} else if (tile->display_state == DISPLAY_HANDLED) {
				cairo_set_source_rgb(cr, 0, 1, 0);
			} else {
				cairo_set_source_rgb(cr, 1, 0, 0);
			}
			cairo_move_to(cr, tile->center.x - geo->numpos[number].x,
				      tile->center.y + geo->numpos[number].y);
			cairo_show_text (cr, geo->numbers + 2*number);
		}
		++tile;
	}

	/* Vertex display state */
	vertex1= geo->vertex;
	for(i=0; i < geo->nvertex; ++i) {
		if (vertex1->display_state == DISPLAY_ERROR) {
			cairo_set_source_rgb(cr, 1, 0, 0);
			cairo_arc (cr, vertex1->pos.x, vertex1->pos.y,
					   geo->tile_width / 5.0, 0, 2 * M_PI);
			cairo_fill(cr);
		}
		++vertex1;
	}
}


/*
 * Calculate extents (width & height) of all possible tile numbers
 * This has to be done after every window resize because the accuracy of
 * the extents depends on the pixel size.
 */
void
draw_measure_font(GtkWidget *drawarea, int width, int height,
		  struct geometry *geo)
{
	int i;
	cairo_t *cr;
	cairo_text_extents_t extent;

	/* set up temporary cairo context */
	cr= gdk_cairo_create (drawarea->window);
	cairo_scale (cr, width/geo->board_size, height/geo->board_size);

	/* scale font size so number 0 fits in tile_height/2. */
	cairo_set_font_size(cr, geo->board_size/2.);
	cairo_text_extents(cr, geo->numbers + 0, &extent);
	geo->font_size= (geo->tile_height/2.) * (geo->board_size/2./extent.height);
	/* further scale font to fit current tile type */
	geo->font_size*= geo->font_scale;

	/* measure extent boxes for all numbers at the new font size */
	cairo_set_font_size(cr, geo->font_size);
	for(i=0; i < geo->max_numlines; ++i) {
		cairo_text_extents(cr, geo->numbers + i*2, &extent);
		geo->numpos[i].x= extent.width/2. + extent.x_bearing;
		geo->numpos[i].y= extent.height/2. -
			(extent.height + extent.y_bearing);
	}
	cairo_destroy (cr);
}


/*
 * Benchmark speed of drawing routine
 */
void
draw_benchmark(GtkWidget *drawarea)
{
	cairo_t *cr;
	struct timeval start_time;	// Contains starting time
	struct timeval end_time;	// Contains ending time
	double result;
	int i;
	int iters=1000;

	printf("Benchmark (%d): starting ...\n", iters);
	gettimeofday (&start_time, NULL);
	for(i=0; i < iters; ++i) {
		cr= gdk_cairo_create (drawarea->window);
		/* set scale so we draw in board_size space */
		cairo_scale (cr,
					 drawarea->allocation.width/(double)board.geo->board_size,
					 drawarea->allocation.height/(double)board.geo->board_size);
		draw_board(cr, board.geo, board.game);
		cairo_destroy(cr);
	}
	gettimeofday (&end_time, NULL);
	result= ((double)(end_time.tv_sec - start_time.tv_sec))*1000000. \
		+ ((double)(end_time.tv_usec - start_time.tv_usec));

	printf("Benchmark (%d): total= %7.2lf ms ; iter=%5.2lf ms\n", iters,
	       result/1000., result/iters/1000.);
}


/*
 * Draw to file
 */
void
draw_board_to_file(struct geometry *geo, struct game *game, const char *filename)
{
	cairo_surface_t *surf;
	cairo_t *cr;

	surf= cairo_image_surface_create(CAIRO_FORMAT_RGB24, 600, 600);
	cr= cairo_create(surf);

	/* set scale so we draw in board_size space */
	cairo_scale (cr,
				 600.0/(double)board.geo->board_size,
				 600.0/(double)board.geo->board_size);
	draw_board(cr, board.geo, board.game);
	cairo_surface_write_to_png(surf, filename);
	cairo_destroy(cr);
	cairo_surface_destroy(surf);
}


/*
 * Draw board preview
 */
void
draw_board_skeleton(cairo_t *cr, struct geometry *geo)
{
	struct vertex *vertex1, *vertex2;
	struct line *line;
	int i;

	/* white background */
	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_paint(cr);

	/* Draw lines */
	cairo_set_source_rgb(cr, 0./256., 0./256., 0./256.);
	cairo_set_line_width (cr, geo->off_line_width*2);
	line= geo->lines;
	for(i=0; i<geo->nlines; ++i) {
		vertex1= line->ends[0];
		vertex2= line->ends[1];
		cairo_move_to(cr, vertex1->pos.x, vertex1->pos.y);
		cairo_line_to(cr, vertex2->pos.x, vertex2->pos.y);
		++line;
	}
	cairo_stroke(cr);
}
