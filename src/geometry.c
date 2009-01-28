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
 * Build line network by connecting lines to each other (fill the in and out fields)
 */
void
geometry_build_line_network(struct geometry *geo)
{
	struct line *lin;
	struct line **list;
	struct vertex *v;
	int line_handled[geo->nlines];
	int i, j;
	
	/* iterate over lines and count how many lines touch each vertex */
	lin= geo->lines;
	for(i=0; i < geo->nlines; ++i) {
		++(lin->ends[0]->nlines);
		++(lin->ends[1]->nlines);
		++lin;
	}
	
	/* allocate space for each vertex's list of lines */
	v= geo->vertex;
	for(i=0; i < geo->nvertex; ++i) {
		/* DEBUG: check that all vertices at least have one line */
		if (v->nlines == 0) 
			g_debug("vertex %d is isolated, has no lines associated", i);
		v->lines= (struct line **)g_malloc0(v->nlines*sizeof(void*));
		++v;
	}
	
	/* iterate over lines, adding them to the vertices they touch 
	   and allocate space for ins and outs */
	lin= geo->lines;
	for(i=0; i < geo->nlines; ++i) {
		/* first end (in end) */
		v= lin->ends[0];
		/* store line in vertex's list of lines */
		list= v->lines;
		for(j=0; j < v->nlines && *list != NULL; ++j) ++list;
		if (j == v->nlines) 
			g_debug("line %d being assoc. to vertex:%d (line's end 0): too many lines in vertex\n", i, v->id);
		*list= lin;
		/* store space for ins */
		lin->nin= v->nlines - 1;
		lin->in= (struct line **)g_malloc0(lin->nin*sizeof(void*));
		
		/* second end (out end) */
		v= lin->ends[1];
		/* store line in vertex's list of lines */
		list= v->lines;
		for(j=0; j < v->nlines && *list != NULL; ++j) ++list;
		if (j == v->nlines) 
			g_debug("line %d being assoc. to vertex:%d (line's end 1): too many lines in vertex\n", i, v->id);
		*list= lin;
		/* store space for outs */
		lin->nout= v->nlines - 1;
		lin->out= (struct line **)g_malloc0(lin->nout*sizeof(void*));
		++lin;
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
void
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
