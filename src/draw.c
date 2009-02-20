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


#define ON_LINE_WIDTH		2.5/500.0*board.geo->board_size
#define OFF_LINE_WIDTH		0.5/500.0*board.geo->board_size
#define DASH_OFFSET		10/500.0*board.geo->board_size
#define DASH_LENGTH		2.25/500.0*board.geo->board_size
#define CROSS_LINE_WIDTH	1./500.0*board.geo->board_size
#define CROSS_RADIUS		5.5/500.0*board.geo->board_size
#define DOT_RADIUS		3.5/500.0*board.geo->board_size



/*
 * DEBUG: Hack to display tile cache as a green grid
 */
static void
draw_tiles(cairo_t *cr)
{
	int i, j;
	double width=board.tile_cache->tile_size;
	
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
 * DEBUG: Hack to display square centers
 */
static void
draw_square_centers(cairo_t *cr)
{
	int i;
	struct square *sq;
	
	cairo_set_source_rgba(cr, 0., 1., 0., 0.2);
	sq= board.geo->squares;
	for(i=0; i < board.geo->nsquares; ++i) {
		cairo_new_sub_path(cr);
		cairo_arc (cr, sq->center.x, sq->center.y, 2*DOT_RADIUS, 
			   0, 2 * M_PI);
		++sq;
	}
	cairo_fill(cr);
}


/*
 * DEBUG: Hack to display which squares are associated with each line
 */
static void
draw_linesquares(cairo_t *cr)
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
		cairo_line_to(cr, lin->sq[0]->center.x, lin->sq[0]->center.y);
		if (lin->nsquares == 2) {
			cairo_move_to(cr, x, y);
			cairo_line_to(cr, lin->sq[1]->center.x, lin->sq[1]->center.y);
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
 */
void
draw_board(cairo_t *cr, int width, int height)
{
	struct vertex *vertex1, *vertex2;
	struct line *line;
	struct square *sq;
	int i, j;
	double x, y;
	struct geometry *geo=board.geo;
	int lines_on;	// how many ON lines a vertex has
	struct game *game=board.game;
	int number;
	
	/* white background */
	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_paint(cr);
	
	/* set scale so we draw in board_size space */
	cairo_scale (cr, width/(double)geo->board_size, 
		     height/(double)geo->board_size);

	// debug
	//draw_tiles(cr);
	//draw_areainf(cr);
	//draw_square_centers(cr);
	//draw_linesquares(cr);
	//draw_bounds(cr);

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
	
	/* Text in squares */
	sq= geo->squares;
	cairo_set_font_size(cr, geo->font_size);
	cairo_set_source_rgb(cr, 0, 0, 0);
	printf("font_size: %lf\n", geo->font_size);
	
	for(i=0; i<geo->nsquares; ++i) {
		number= game->numbers[sq->id];
		if (number != -1) {	// square has a number
			cairo_move_to(cr, sq->center.x - geo->numpos[number].x,
				      sq->center.y + geo->numpos[number].y);
			cairo_show_text (cr, geo->numbers + 2*number);
		}
		++sq;
	}
}


/*
 * Calculate extents (width & height) of all possible square numbers
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

	/* scale font size so number 0 fits in sq_height/2. */
	cairo_set_font_size(cr, geo->board_size/2.);
	cairo_text_extents(cr, geo->numbers + 0, &extent);
	geo->font_size= (geo->sq_height/2.) * (geo->board_size/2./extent.height);
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
		draw_board(cr, drawarea->allocation.width, 
			   drawarea->allocation.height);
		cairo_destroy(cr);
	}
	gettimeofday (&end_time, NULL);
	result= ((double)(end_time.tv_sec - start_time.tv_sec))*1000000. \
		+ ((double)(end_time.tv_usec - start_time.tv_usec));

	printf("Benchmark (%d): total= %7.2lf ms ; iter=%5.2lf ms\n", iters, 
	       result/1000., result/iters/1000.);
}
