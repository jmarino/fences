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

#include "gamedata.h"


/* defined in gamedata.c */
extern struct board board;


/*
 * Hack to display tile cache as a green grid
 */
static void
draw_tiles(cairo_t *cr)
{
	int i, j;
	int width=board.tile_cache->tile_size;
	
	cairo_set_source_rgb(cr, 0., 150/256., 0.);
	cairo_set_line_width (cr, 10);
	for(i=0; i < 10; ++i) {
		for(j=0; j < 10; ++j) {
			cairo_rectangle (cr, i*width, j*width, width, width);
			cairo_stroke(cr);
		}
	}
}
			 

/*
 * Draw game on board
 */
static void
draw_board(GtkWidget *drawarea, cairo_t *cr, struct game *gamedata)
{
	struct dot *dot1, *dot2;
	struct line *line;
	struct square *sq;
	int i,j;
	double x, y;
	int w=drawarea->allocation.width;
	int h=drawarea->allocation.height;
	cairo_text_extents_t extent[4];
	char *nums[]={"0", "1", "2", "3"};
	double font_scale;
	
	/* white background */
	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_rectangle (cr, 0, 0, w, h);
	cairo_fill(cr);
	
	//printf("%d, %d\n", w, h);
	cairo_scale (cr, w/(double)board.board_size, h/(double)board.board_size);
	//debug
	//draw_tiles(cr);

	/* Draw lines */
	double dash[]={50};
	cairo_set_source_rgb(cr, 150/256., 150/256., 150/256.);
	cairo_set_line_width (cr, 10);
	cairo_set_dash(cr, dash, 1, 100);
	line= gamedata->lines;
	for(i=0; i<gamedata->nlines; ++i) {
		dot1= gamedata->dots + line->dots[0];
		dot2= gamedata->dots + line->dots[1];
		if (line->state == LINE_OFF || line->state == LINE_CROSSED) {
			cairo_set_source_rgb(cr, 150/256., 150/256., 150/256.);
			cairo_set_line_width (cr, 10);
			cairo_set_dash(cr, dash, 1, 100);
			cairo_move_to(cr, dot1->x, dot1->y);
			cairo_line_to(cr, dot2->x, dot2->y);
			cairo_stroke(cr);
			if (line->state == LINE_CROSSED) { // draw cross
				cairo_set_source_rgb(cr, 1., 0., 0.);
				cairo_set_line_width (cr, 50);
				cairo_set_dash(cr, dash, 0, 100);
				x= (dot1->x + dot2->x)/2.;
				y= (dot1->y + dot2->y)/2.;
				cairo_move_to(cr, x-120, y-120);
				cairo_line_to(cr, x+120, y+120);
				cairo_move_to(cr, x-120, y+120);
				cairo_line_to(cr, x+120, y-120);
				cairo_stroke(cr);
			}
		} else if (line->state == LINE_ON) {
			cairo_set_source_rgb(cr, 0., 0., 1.);
			cairo_set_line_width (cr, 100);
			cairo_set_dash(cr, dash, 0, 100);
			cairo_move_to(cr, dot1->x, dot1->y);
			cairo_line_to(cr, dot2->x, dot2->y);
			cairo_stroke(cr);
		} else {
			printf("draw.c: line state invalid: %d\n", line->state);
		}
		++line;
	}
	//cairo_stroke(cr);
	
	/* Draw dots */
	dot1= gamedata->dots;
	cairo_set_source_rgb(cr, 0, 0, 0);
	for(i=0; i<gamedata->ndots; ++i) {
		cairo_arc (cr, dot1->x, dot1->y, 75, 0, 2 * M_PI);
		cairo_new_sub_path(cr);
		++dot1;
	}
	cairo_fill(cr);
	
	/* Text in squares */
	/* calibrate font */
	cairo_set_font_size(cr, 100.);
	cairo_text_extents(cr, nums[0], extent);
	font_scale= 100./extent[0].height;
	
	cairo_set_font_size(cr, gamedata->sq_height*font_scale/2.);
	for(i=0; i<4; ++i) 
		cairo_text_extents(cr, nums[i], extent + i);
	sq= gamedata->squares;
	cairo_set_source_rgb(cr, 0, 0, 0);
	
	for(i=0; i<gamedata->nsquares; ++i) {
		if (sq->number != -1) {		// square has a number
			x= y= 0;
			for(j=0; j<4; ++j) {
				dot1= gamedata->dots + sq->dots[j];
				x+= dot1->x;
				y+= dot1->y;
			}
			x/= 4;
			y/= 4;
			cairo_move_to(cr, x - extent[sq->number].width/2, 
						  y + extent[sq->number].height/2);
			cairo_show_text (cr, nums[sq->number]);
		}
		++sq;
	}
}


