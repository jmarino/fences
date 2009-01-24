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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <math.h>

#include "gamedata.h"
#include "square-tile.h"
#include "penrose-tile.h"
#include "tile_cache.h"


/* holds info about board */
struct board board;


/* Game area size */
//#define FIELD_SIZE		10000



/*
 * Recursively fill in 'in' and 'out' lists of lines for each line
 */
static void
join_lines_recursive(struct game *game, int line_id, int *line_handled)
{
	struct line **list;
	struct dot *d;
	int i;
	
	/* NOTE: now we do the check before we call: faster */
	/* check if this line has been already handled */
	/*if (line_handled[line_id]) {
		g_debug("***rec: line %d already handled", line_id);
		return;
	}*/
	
	/* find lines touching ends[0] */
	list= game->lines[line_id].in;
	d= game->lines[line_id].ends[0];
	for(i=0; i < d->nlines; ++i) {
		if (d->lines[i]->id != line_id) {
			*list= d->lines[i];
			++list;
		}
	}
	
	/* find lines touching ends[1] */
	list= game->lines[line_id].out;
	d= game->lines[line_id].ends[1];
	for(i=0; i < d->nlines; ++i) {
		if (d->lines[i]->id != line_id) {
			*list= d->lines[i];
			++list;
		}
	}
	
	/* mark current line as handled */
	line_handled[line_id]= 1;
	
	/* call recursively on all lines that current line touches */
	list= game->lines[line_id].in;
	for(i=0; i < game->lines[line_id].nin; ++i) {
		if (line_handled[list[i]->id] == 0) // only lines not handled yet
			join_lines_recursive(game, list[i]->id, line_handled);
	}
	list= game->lines[line_id].out;
	for(i=0; i < game->lines[line_id].nout; ++i) {
		if (line_handled[list[i]->id] == 0) // only lines not handled yet
			join_lines_recursive(game, list[i]->id, line_handled);
	}
}


/*
 * Build line network by connecting lines to each other (fill the in and out fields)
 */
void
build_line_network(struct game *game)
{
	struct line *lin;
	struct line **list;
	struct dot *d;
	int line_handled[game->nlines];
	int i, j;
	
	/* iterate over lines and count how many lines touch each dot */
	lin= game->lines;
	for(i=0; i < game->nlines; ++i) {
		++(lin->ends[0]->nlines);
		++(lin->ends[1]->nlines);
		++lin;
	}
	
	/* allocate space for each dot's list of lines */
	d= game->dots;
	for(i=0; i < game->ndots; ++i) {
		/* DEBUG: check that all dots at least have one line */
		if (d->nlines == 0) 
			printf("DEBUG (join_lines): dot %d has no lines\n", i);
		d->lines= (struct line **)g_malloc0(d->nlines*sizeof(void*));
		if (d->lines == NULL) {
			printf("join_lines: mem error (d->lines)\n");
		}
		++d;
	}
	
	/* iterate over lines, adding them to the dots they touch 
	   and allocate space for ins and outs */
	lin= game->lines;
	for(i=0; i < game->nlines; ++i) {
		/* first end (in end) */
		d= lin->ends[0];
		/* store line in dot's list of lines */
		list= d->lines;
		for(j=0; j < d->nlines && *list != NULL; ++j) ++list;
		if (j == d->nlines) printf("DEBUG (join_lines): shouldn't get here: end 0 line %d\n", i);
		*list= lin;
		/* store space for ins */
		lin->nin= d->nlines - 1;
		lin->in= (struct line **)g_malloc0(lin->nin*sizeof(void*));
		if (lin->in == NULL) {
			printf("join_lines: mem error (lin->in)\n");
		}
		
		/* second end (out end) */
		d= lin->ends[1];
		/* store line in dot's list of lines */
		list= d->lines;
		for(j=0; j < d->nlines && *list != NULL; ++j) ++list;
		if (j == d->nlines) printf("DEBUG (join_lines): shouldn't get here: end 1 line %d\n", i);
		*list= lin;
		/* store space for outs */
		lin->nout= d->nlines - 1;
		lin->out= (struct line **)g_malloc0(lin->nout*sizeof(void*));
		if (lin->out == NULL) {
			printf("join_lines: mem error (lin->out)\n");
		}
		++lin;
	}
	
	/* connect lines, i.e., fill 'in' & 'out' lists */
	memset(line_handled, 0, sizeof(int) * game->nlines);
	join_lines_recursive(game, 0, line_handled);
	
	/* DEBUG: check all lines were handled */
	for(i=0; i < game->nlines; ++i) {
		if (line_handled[i] == 0) 
			printf("DEBUG: line %d not handled\n", i);
	}
}


/*
 * Measure smallest width and height of a square with a number in it
 * Used to decide what font size to use
 */
void
find_smallest_numbered_square(struct game *game)
{
	struct square *sq;
	int i, j, j2;
	double sqw, sqh, tmp;
	
	/* go around all squares to measure smallest w and h */
	game->sq_width= board.board_size;
	game->sq_height= board.board_size;
	sq= game->squares;
	for(i=0; i<game->nsquares; ++i) {
		if (sq->number != -1) {		// square has a number
			sqw= sqh= 0.;
			for(j=0; j < sq->nvertex; ++j) {
				j2= (j + 1) % sq->nvertex;
				tmp= fabs(sq->vertex[j]->pos.x - 
					 sq->vertex[j2]->pos.x);
				if (tmp > sqw) sqw= tmp;
				tmp= fabs(sq->vertex[j]->pos.y - 
					 sq->vertex[j2]->pos.y);
				if (tmp > sqh) sqh= tmp;
			}
			if (sqw < game->sq_width) game->sq_width= sqw;
			if (sqh < game->sq_height) game->sq_height= sqh;
		}
		++sq;
	}
}


/*
 * Define area of influence for each line (4 points)
 */
void
define_line_infarea(struct game *game)
{
	int i;
	struct line *lin;
	struct dot *d1;
	struct square *sq;
	
	lin= game->lines;
	for(i=0; i<game->nlines; ++i) {
		d1= lin->ends[0];
		lin->inf[0].x= d1->pos.x;
		lin->inf[0].y= d1->pos.y;
		sq= lin->sq[0];
		lin->inf[1].x= sq->center.x;
		lin->inf[1].y= sq->center.y;
		d1= lin->ends[1];
		lin->inf[2].x= d1->pos.x;
		lin->inf[2].y= d1->pos.y;
		if (lin->nsquares == 2) {
			sq= lin->sq[1];
			lin->inf[3].x= sq->center.x;
			lin->inf[3].y= sq->center.y;
		} else {	// edge line, must manufacture 4th point
			/* make up point across line: 
			 along line joining center of square to center of line */
			lin->inf[3].x= lin->inf[0].x + lin->inf[2].x - lin->inf[1].x;
			lin->inf[3].y= lin->inf[0].y + lin->inf[2].y - lin->inf[1].y;
		}
		/* define box that contains line as [x,y];[w,h]
		 inf[0].xy & inf[2].xy are both ends of the line */
		lin->inf_box[0].x= 
			(lin->inf[0].x < lin->inf[2].x) ? lin->inf[0].x : lin->inf[2].x;
		lin->inf_box[1].x= lin->inf[2].x - lin->inf[0].x;
		if (lin->inf_box[1].x < 0) lin->inf_box[1].x= -lin->inf_box[1].x;
		lin->inf_box[0].y= 
			(lin->inf[0].y < lin->inf[2].y) ? lin->inf[0].y : lin->inf[2].y;
		lin->inf_box[1].y= lin->inf[2].y - lin->inf[0].y;
		if (lin->inf_box[1].y < 0) lin->inf_box[1].y= -lin->inf_box[1].y;
		/* pad inf_box a bit just in case */
		lin->inf_box[0].x-= 0.025;
		lin->inf_box[0].y-= 0.025;
		lin->inf_box[1].x+= 0.050;
		lin->inf_box[1].y+= 0.050;
		++lin;
	}
}



/*
 * Initialize board
 */
void
initialize_board(void)
{
	/* Setup coordinate size of board */
	board.board_size= 1;//11000;
	board.board_margin= 0.05;//500;
	board.game_size= board.board_size - 2*board.board_margin; //10000
	board.tile_cache= NULL;

	/* make up example game */
	//board.game= generate_example_game();
	board.game= build_penrose_board();

	/* generate tile cache for lines */
	setup_tile_cache();
}
