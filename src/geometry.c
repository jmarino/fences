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

#include "geometry.h"



/*
 * Recursively fill in 'in' and 'out' lists of lines for each line
 */
static void
join_lines_recursive(struct geometry *geo, int line_id, int *line_handled)
{
	struct line **list;
	struct vertex *v;
	int i;
	
	/* NOTE: now we do the check before we call: faster */
	/* check if this line has been already handled */
	/*if (line_handled[line_id]) {
		g_debug("***rec: line %d already handled", line_id);
		return;
	}*/
	
	/* find lines touching ends[0] */
	list= geo->lines[line_id].in;
	v= geo->lines[line_id].ends[0];
	for(i=0; i < v->nlines; ++i) {
		if (v->lines[i]->id != line_id) {
			*list= v->lines[i];
			++list;
		}
	}
	
	/* find lines touching ends[1] */
	list= geo->lines[line_id].out;
	v= geo->lines[line_id].ends[1];
	for(i=0; i < v->nlines; ++i) {
		if (v->lines[i]->id != line_id) {
			*list= v->lines[i];
			++list;
		}
	}
	
	/* mark current line as handled */
	line_handled[line_id]= 1;
	
	/* call recursively on all lines that current line touches */
	list= geo->lines[line_id].in;
	for(i=0; i < geo->lines[line_id].nin; ++i) {
		if (line_handled[list[i]->id] == 0) // only lines not handled yet
			join_lines_recursive(geo, list[i]->id, line_handled);
	}
	list= geo->lines[line_id].out;
	for(i=0; i < geo->lines[line_id].nout; ++i) {
		if (line_handled[list[i]->id] == 0) // only lines not handled yet
			join_lines_recursive(geo, list[i]->id, line_handled);
	}
}



/*
 * Count how many squares touch each vertex
 */
static void
geometry_count_squares_touching_vertex(struct geometry *geo)
{
	int i, j;
	struct square *sq;
	
	/* iterate over squares increasing the square count of each of its vertices */
	sq= geo->squares;
	for(i=0; i < geo->nsquares; ++i) {
		for(j=0; j < sq->nvertex; ++j) 
			++(sq->vertex[j]->nsquares);
		++sq;
	}
}


/*
 * Count how many squares touch each vertex
 */
static void
geometry_count_lines_touching_vertex(struct geometry *geo)
{
	int i;
	struct line *lin;
	
	/* iterate over lines and count how many lines touch each vertex */
	lin= geo->lines;
	for(i=0; i < geo->nlines; ++i) {
		++(lin->ends[0]->nlines);
		++(lin->ends[1]->nlines);
		++lin;
	}
}


/*
 * Build networks. 
 * Connect lines to each other (fill the in and out fields), vertices to squares,
 * and vertices to lines
 */
static void
geometry_build_networks(struct geometry *geo)
{
	struct line *lin;
	struct square *sq;
	struct vertex *vertex;
	int line_handled[geo->nlines];
	int i, j;
	
	/* count how many lines touch each vertex */
	geometry_count_lines_touching_vertex(geo);
	
	/* count how many squares touch each vertex */
	geometry_count_squares_touching_vertex(geo);
	
	/* allocate space for each line's in & out arrays */
	lin= geo->lines;
	for(i=0; i < geo->nlines; ++i) {
		/* store space for ins (lines touching vertex - 1) */
		lin->nin= lin->ends[0]->nlines - 1;
		lin->in= (struct line **)g_malloc0(lin->nin*sizeof(void*));
		/* store space for outs (lines touching vertex - 1) */
		lin->nout= lin->ends[1]->nlines - 1;
		lin->out= (struct line **)g_malloc0(lin->nout*sizeof(void*));
		++lin;
	}
	
	/* allocate space for each vertex's list of lines and squares */
	vertex= geo->vertex;
	for(i=0; i < geo->nvertex; ++i) {
		/* DEBUG: check that all vertices at least have one line */
		if (vertex->nlines == 0) 
			g_debug("vertex %d is isolated, has no lines associated", i);
		if (vertex->nsquares == 0) 
			g_debug("vertex %d is isolated, has no squares associated", i);
		vertex->lines= (struct line **)g_malloc(vertex->nlines*sizeof(void*));
		vertex->sq= (struct square **)g_malloc(vertex->nsquares*sizeof(void*));
		vertex->nsquares= 0;	// we use this as a counter in the loop below
		vertex->nlines= 0;
		++vertex;
	}
	
	/* iterate over lines, adding them to the vertices they touch 
	   and allocate space for ins and outs */
	lin= geo->lines;
	for(i=0; i < geo->nlines; ++i) {
		/* first end (in end) */
		vertex= lin->ends[0];
		vertex->lines[vertex->nlines]= lin;
		++(vertex->nlines);
		/* second end (out end) */
		vertex= lin->ends[1];
		vertex->lines[vertex->nlines]= lin;
		++(vertex->nlines);
		++lin;
	}
	
	/* iterate over squares adding them to their vertex list */
	sq= geo->squares;
	for(i=0; i < geo->nsquares; ++i) {
		for(j=0; j < sq->nvertex; ++j) {
			vertex= sq->vertex[j];
			vertex->sq[vertex->nsquares]= sq;
			++(vertex->nsquares);
		}
		++sq;
	}
	
	/* connect lines, i.e., fill 'in' & 'out' lists */
	memset(line_handled, 0, sizeof(int) * geo->nlines);
	join_lines_recursive(geo, 0, line_handled);
	
	/* DEBUG: check all lines were handled */
	for(i=0; i < geo->nlines; ++i) {
		if (line_handled[i] == 0) 
			g_debug("after connect lines: line %d not handled\n", i);
	}
}


/*
 * Define area of influence for each line (4 points)
 */
static void
geometry_define_line_infarea(struct geometry *geo)
{
	int i;
	struct line *lin;
	struct vertex *v1;
	struct square *sq;
	
	lin= geo->lines;
	for(i=0; i<geo->nlines; ++i) {
		v1= lin->ends[0];
		lin->inf[0].x= v1->pos.x;
		lin->inf[0].y= v1->pos.y;
		sq= lin->sq[0];
		lin->inf[1].x= sq->center.x;
		lin->inf[1].y= sq->center.y;
		v1= lin->ends[1];
		lin->inf[2].x= v1->pos.x;
		lin->inf[2].y= v1->pos.y;
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
		if (lin->inf[0].x < lin->inf[2].x) {
			lin->inf_box[0].x= lin->inf[0].x;
			lin->inf_box[1].x= lin->inf[2].x;
		} else {
			lin->inf_box[0].x= lin->inf[2].x;
			lin->inf_box[1].x= lin->inf[0].x;
		}
		if (lin->inf[0].y < lin->inf[2].y) {
			lin->inf_box[0].y= lin->inf[0].y;
			lin->inf_box[1].y= lin->inf[2].y;
		} else {
			lin->inf_box[0].y= lin->inf[2].y;
			lin->inf_box[1].y= lin->inf[0].y;
		}
		lin->inf_box[1].x-= lin->inf_box[0].x;
		lin->inf_box[1].y-= lin->inf_box[0].y;

		/* pad inf_box a bit (board_size * 2%) just in case */
		lin->inf_box[0].x-= geo->board_size*0.02;
		lin->inf_box[0].y-= geo->board_size*0.02;
		lin->inf_box[1].x+= geo->board_size*0.04;
		lin->inf_box[1].y+= geo->board_size*0.04;
		++lin;
	}
}


/*
 * Measure smallest width and height of all squares
 */
static void
geometry_measure_squares(struct geometry *geo)
{
	struct square *sq;
	int i, j, j2;
	double sqw, sqh, tmp;
	
	/* go around all squares to measure smallest w and h */
	sq= geo->squares;
	for(i=0; i<geo->nsquares; ++i) {
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
		if (i == 0) {
			geo->sq_width= sqw;
			geo->sq_height= sqh;
		} else {
			if (sqw < geo->sq_width) geo->sq_width= sqw;
			if (sqh < geo->sq_height) geo->sq_height= sqh;
		}
		++sq;
	}
}


/*
 * Create new geometry
 */
struct geometry*
geometry_create_new(int nsquares, int nvertex, int nlines, int max_numlines)
{
	struct geometry *geo;
	int i;
	
	/* Allocate memory for geometry data */
	geo= (struct geometry*)g_malloc(sizeof(struct geometry));
	geo->nsquares= nsquares;
	geo->nvertex= nvertex;
	geo->nlines= nlines;
	geo->squares= (struct square*)g_malloc(geo->nsquares*sizeof(struct square));
	geo->vertex= (struct vertex*)g_malloc(geo->nvertex*sizeof(struct vertex));
	geo->lines= (struct line*)g_malloc(geo->nlines*sizeof(struct line));
	geo->numpos= (struct point *)g_malloc(max_numlines*sizeof(struct point));
	geo->numbers= (char *)g_malloc(2*max_numlines*sizeof(char));
	for(i=0; i < max_numlines; ++i)
		snprintf(geo->numbers + 2*i, 2, "%1d", i);
	geo->sq_width= 0.;
	geo->sq_height= 0.;
	geo->on_line_width= 0.;
	geo->off_line_width= 0.;
	geo->cross_line_width= 0.;
	geo->cross_radius= 0.;
	geo->max_numlines= max_numlines;
	geo->board_size= 0.;
	geo->board_margin= 0.;
	geo->game_size= 0.;
	
	return geo;
}


/*
 * Initialize lines in geometry structure
 */
void
geometry_initialize_lines(struct geometry *geo)
{
	int i;
	struct line *lin;

	lin= geo->lines;
	for (i= 0; i < geo->nlines; ++i) {
		lin->id= i;
		lin->nsquares= 0;
		lin->fx_status= 0;
		lin->fx_frame= 0;
		++lin;
	}
}


/*
 * Free memory used by a geometry structure
 */
void
geometry_destroy(struct geometry *geo)
{
	int i;
	
	for(i=0; i < geo->nsquares; ++i) {
		g_free(geo->squares[i].vertex);
		g_free(geo->squares[i].sides);
	}
	for(i=0; i < geo->nvertex; ++i) {
		g_free(geo->vertex[i].lines);
		g_free(geo->vertex[i].sq);
	}
	for(i=0; i < geo->nlines; ++i) {
		g_free(geo->lines[i].in);
		g_free(geo->lines[i].out);
	}
	g_free(geo->squares);
	g_free(geo->vertex);
	g_free(geo->lines);
	g_free(geo->numbers);
	g_free(geo->numpos);
	g_free(geo);
}


/*
 * Finalize geometry data. Connect all elements together.
 */
void
geometry_connect_elements(struct geometry *geo)
{
	/* interconnect all the lines, vertices and squares */
	geometry_build_networks(geo);
	
	/* define area of influence of each line */
	geometry_define_line_infarea(geo);
	
	/* measure minimum square dimensions */
	geometry_measure_squares(geo);
}
