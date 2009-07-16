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
 * Connect each vertex to the lines it touches.
 * Before this point only lines had references to which vertices they touch.
 * This function builds the opposite references: tracks which lines each
 * vertex touches.
 */
static void
geometry_connect_vertex_lines(struct geometry *geo)
{
	int i;
	struct line *lin;
	struct vertex *vertex;

	/* iterate over lines and count how many lines touch each vertex */
	lin= geo->lines;
	for(i=0; i < geo->nlines; ++i) {
		++(lin->ends[0]->nlines);
		++(lin->ends[1]->nlines);
		++lin;
	}

	/* allocate space for each vertex's list of lines */
	vertex= geo->vertex;
	for(i=0; i < geo->nvertex; ++i) {
		/* sanity check: all vertices must have at least two lines */
		if (vertex->nlines < 2) {
			g_message("CRITICAL: vertex %d has %d lines associated (needs at least 2)",
					  i, vertex->nlines);
		}
		vertex->lines= (struct line **)g_malloc(vertex->nlines*sizeof(void*));
		vertex->nlines= 0;		/* reset to be used as a counter in loop below */
		++vertex;
	}

	/* iterate over lines, adding themselves to the vertices they touch */
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
}


/*
 * Create and populate in & out arrays for each line.
 */
static void
geometry_fill_inout(struct geometry *geo)
{
	struct line *lin;
	struct vertex *vertex;
	int i, j;

	/* populate the in & out arrays of each line */
	lin= geo->lines;
	for(i=0; i < geo->nlines; ++i) {
		/* store space for ins (lines touching vertex - 1) */
		vertex= lin->ends[0];
		lin->nin= 0;
		lin->in= (struct line **)g_malloc0((vertex->nlines - 1)*sizeof(void*));
		for(j=0; j < vertex->nlines; ++j) {
			if (vertex->lines[j] == lin) continue;
			lin->in[lin->nin]= vertex->lines[j];
			++lin->nin;
		}
		/* store space for outs (lines touching vertex - 1) */
		vertex= lin->ends[1];
		lin->nout= 0;
		lin->out= (struct line **)g_malloc0((vertex->nlines - 1)*sizeof(void*));
		for(j=0; j < vertex->nlines; ++j) {
			if (vertex->lines[j] == lin) continue;
			lin->out[lin->nout]= vertex->lines[j];
			++lin->nout;
		}
		++lin;
	}
}


/*
 * Go around square that touches line, connecting vertices and lines to square.
 * Add vertices and lines to square.
 * Add square to vertices.
 */
static void
geometry_go_around_square(struct square *sq, struct line *lin)
{
	struct vertex *start_vertex;
	struct vertex *vertex;
	struct line **dir=NULL;
	int ndir;
	int i;

	start_vertex= lin->ends[1];
	vertex= lin->ends[1];
	ndir= lin->nout;
	dir= lin->out;

	do {
		/* connect lines to square */
		sq->sides[sq->nsides]= lin;
		++sq->nsides;
		/* connect vertex & square */
		sq->vertex[sq->nvertex]= vertex;
		++(sq->nvertex);
		vertex->sq[vertex->nsquares]= sq;
		++(vertex->nsquares);

		/* find next line that touches square */
		for(i=0; i < ndir; ++i) {
			lin= dir[i];
			if (lin->sq[0] == sq
				|| (lin->nsquares == 2 && lin->sq[1] == sq)) break;
		}
		g_assert(i < ndir);	/* we must always find a line */

		/* find common vertex */
		if (lin->ends[0] == vertex) {
			ndir= lin->nout;
			dir= lin->out;
			vertex= lin->ends[1];
		} else {
			g_assert(lin->ends[1] == vertex); /* sanity check */
			ndir= lin->nin;
			dir= lin->in;
			vertex= lin->ends[0];
		}
	} while(vertex != start_vertex);
}


/*
 * Connect each square to the vertex and lines it touches. Also connect
 * each vertex to the squares it touches.
 * Before this point only lines had references to which squares they touch.
 * This function builds tracks which lines each square touches, which vertex
 * each square touches, and which squares each vertex touches.
 */
static void
geometry_connect_squares(struct geometry *geo)
{
	int i, j;
	struct line *lin;
	struct vertex *vertex;
	struct square *sq;
	int direction;

	/* count number of vertex and number of lines touching each square
	   NOTE: notice that sq->nvertex and vertex->nsquares are counted twice.
	   We're over-counting them by a factor of 2. */
	lin= geo->lines;
	for(i=0; i < geo->nlines; ++i) {
		vertex= lin->ends[0];
		for(j=0; j < lin->nsquares; ++j) {
			sq= lin->sq[j];
			++(sq->nvertex);
			++(sq->nsides);
			++(vertex->nsquares);
		}
		vertex= lin->ends[1];
		for(j=0; j < lin->nsquares; ++j) {
			sq= lin->sq[j];
			++(sq->nvertex);
			++(sq->nsides);
			++(vertex->nsquares);
		}
		++lin;
	}

	/* allocate space for squares in each vertex.
	   Compensate for over-counting nsquares (factor 2) */
	vertex= geo->vertex;
	for(i=0; i < geo->nvertex; ++i) {
		vertex->sq= (struct square **)g_malloc0((vertex->nsquares/2)*sizeof(void*));
		vertex->nsquares= 0;	/* to be used as counter below */
		++vertex;
	}

	/* allocate space for vertices and lines in each square.
	   Compensate for over-counting nvertex (factor 2) */
	sq= geo->squares;
	for(i=0; i < geo->nsquares; ++i) {
		sq->vertex= (struct vertex **)g_malloc0((sq->nvertex/2)*sizeof(void*));
		sq->nvertex= 0;			/* to be used as counter below */
		sq->sides= (struct line **)g_malloc0((sq->nsides/2)*sizeof(void*));
		sq->nsides= 0;			/* to be used as counter below */
		++sq;
	}

	/* try each line and follow it around the squares it touches
	   while doing this, set square's sides & vertex, and set
	   vertex's squares
	   NOTE: sq->fx_status is reused (shamelessly) as a mask to know
	   which squares have been already handled. */
	lin= geo->lines;
	for(i=0; i < geo->nlines; ++i) {
		sq= lin->sq[0];
		if (sq->fx_status == 0) {
			geometry_go_around_square(sq, lin);
			sq->fx_status= 1;
		}
		if (lin->nsquares == 2) {
			sq= lin->sq[1];
			if (sq->fx_status == 0) {
				geometry_go_around_square(sq, lin);
				sq->fx_status= 1;
			}
		}
		++lin;
	}

	/* reset fx_status, which was used as a mask */
	sq= geo->squares;
	for(i=0; i < geo->nsquares; ++i) {
		g_assert(sq->fx_status == 1);
		sq->fx_status= 0;
		++sq;
	}
}


/*
 * Calculate the center point of each tile
 */
static void
geometry_calculate_tile_centers(struct geometry *geo)
{
	int i, j;
	struct square *sq;

	sq= geo->squares;
	for(i=0; i < geo->nsquares; ++i) {
		/* calculate position of center of square */
		sq->center.x= sq->vertex[0]->pos.x;
		sq->center.y= sq->vertex[0]->pos.y;
		for(j=1; j < sq->nvertex; ++j) {
			sq->center.x+= sq->vertex[j]->pos.x;
			sq->center.y+= sq->vertex[j]->pos.y;
		}
		sq->center.x= sq->center.x/(double)sq->nvertex;
		sq->center.y= sq->center.y/(double)sq->nvertex;
		++sq;
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
 * A fully connected geometry is formed from a simple skeleton geometry
 * The skeleton geometry has:
 *  - All tiles defined:
 *    * only id is set
 *  - All vertex defined
 *    * id and pos are set
 *  - All lines defined
 *    * id, ends and sq fields are set
 * The info about connections is contained in the lines. This function will
 * extract that info and generate a fully connected geometry.
 */
void
geometry_connect_skeleton(struct geometry *geo)
{
	/* connect vertices and lines */
	geometry_connect_vertex_lines(geo);

	/* create and populate in & out arrays in each line */
	geometry_fill_inout(geo);

	/* connect squares to lines and vertices */
	geometry_connect_squares(geo);

	/* find square centers */
	geometry_calculate_tile_centers(geo);

	/* define area of influence of each line */
	geometry_define_line_infarea(geo);

	/* measure minimum square dimensions */
	geometry_measure_squares(geo);
}
