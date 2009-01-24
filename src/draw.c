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


#define ON_LINE_WIDTH		5/500.0*board.board_size
#define OFF_LINE_WIDTH		0.5/500.0*board.board_size
#define DASH_OFFSET		10/500.0*board.board_size
#define DASH_LENGTH		2.25/500.0*board.board_size
#define CROSS_LINE_WIDTH	2.25/500.0*board.board_size
#define CROSS_RADIUS		5.5/500.0*board.board_size
#define DOT_RADIUS		3.5/500.0*board.board_size



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
	lin= board.game->lines;
	for(i=0; i < board.game->nlines; ++i) {
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
	struct dot *dot1, *dot2;
	struct line *line;
	struct square *sq;
	int i, j;
	double x, y;
	cairo_text_extents_t extent[4];
	const char *nums[]={"0", "1", "2", "3"};
	double font_scale;
	struct game *game=board.game;
	int lines_on;	// how many ON lines a dot has
	
	/* white background */
	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_paint(cr);
	
	/* set scale so we draw in board_size space */
	cairo_scale (cr, width/(double)board.board_size, 
		     height/(double)board.board_size);

	//debug
	//draw_tiles(cr);
	draw_areainf(cr);

	/* Draw OFF lines first */
	cairo_set_source_rgb(cr, 150/256., 150/256., 150/256.);
	cairo_set_line_width (cr, OFF_LINE_WIDTH);
	line= game->lines;
	for(i=0; i<game->nlines; ++i) {
		dot1= line->ends[0];
		dot2= line->ends[1];
		if (line->state != LINE_ON) {	
			cairo_move_to(cr, dot1->pos.x, dot1->pos.y);
			cairo_line_to(cr, dot2->pos.x, dot2->pos.y);
		}
		++line;
	}
	cairo_stroke(cr);
	
	/* Draw lines */
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
	line= game->lines;
	for(i=0; i<game->nlines; ++i) {
		dot1= line->ends[0];
		dot2= line->ends[1];
		if (line->state == LINE_CROSSED) { // draw cross
			cairo_set_source_rgb(cr, 1., 0., 0.);
			cairo_set_line_width (cr, CROSS_LINE_WIDTH);
			x= (dot1->pos.x + dot2->pos.x)/2.;
			y= (dot1->pos.y + dot2->pos.y)/2.;
			cairo_move_to(cr, x-CROSS_RADIUS, y-CROSS_RADIUS);
			cairo_line_to(cr, x+CROSS_RADIUS, y+CROSS_RADIUS);
			cairo_move_to(cr, x-CROSS_RADIUS, y+CROSS_RADIUS);
			cairo_line_to(cr, x+CROSS_RADIUS, y-CROSS_RADIUS);
			cairo_stroke(cr);
		} else if (line->state == LINE_ON) {
			fx_setcolor(cr, line);
			//cairo_set_source_rgb(cr, 0., 0., 1.);
			cairo_set_line_width (cr, ON_LINE_WIDTH);
			cairo_move_to(cr, dot1->pos.x, dot1->pos.y);
			cairo_line_to(cr, dot2->pos.x, dot2->pos.y);
			cairo_stroke(cr);
			//fx_nextframe(line);
		} else if (line->state != LINE_OFF) {
			g_debug("draw_line: line (%d) state invalid: %d", line->id, line->state);
		}
		++line;
	}
	//cairo_stroke(cr);
	
	/* Draw dots */
	if (0) {
		dot1= game->dots;
		for(i=0; i<game->ndots; ++i) {
			/* count how many lines on are touching this */
			lines_on= 0;
			for(j=0; j < dot1->nlines; ++j) {
				if (dot1->lines[j]->state == LINE_ON)
					++lines_on;
			}
			/* draw dot */
			if (lines_on == 2) cairo_set_source_rgb(cr, 0, 0, 1);
			else cairo_set_source_rgb(cr, 0, 0, 0);
			cairo_arc (cr, dot1->pos.x, dot1->pos.y, DOT_RADIUS, 
				   0, 2 * M_PI);
			cairo_fill(cr);
			++dot1;
		}
	}
	
	/* Text in squares */
	/* calibrate font */
	cairo_set_font_size(cr, board.board_size/100.);
	cairo_text_extents(cr, nums[0], &extent[0]);
	font_scale= board.board_size/100./extent[0].height;
	
	cairo_set_font_size(cr, game->sq_height*font_scale/2.);
	for(i=0; i < 4; ++i) 
		cairo_text_extents(cr, nums[i], &extent[i]);
	sq= game->squares;
	cairo_set_source_rgb(cr, 0, 0, 0);
	
	for(i=0; i<game->nsquares; ++i) {
		if (sq->number != -1) {		// square has a number
			cairo_move_to(cr, sq->center.x - extent[sq->number].width/2, 
				      sq->center.y + extent[sq->number].height/2);
			cairo_show_text (cr, nums[sq->number]);
		}
		++sq;
	}
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
