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


/* minimum distance square to be considered the same point */
static double DISTANCE_RESOLUTION_SQUARED=0.0;


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
 * Go around tile that touches line, connecting vertices and lines to tile.
 * Add vertices and lines to tile.
 * Add tile to vertices.
 */
static void
geometry_go_around_tile(struct tile *tile, struct line *lin)
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
		/* connect lines to tile */
		tile->sides[tile->nsides]= lin;
		++tile->nsides;
		/* connect vertex & tile */
		tile->vertex[tile->nvertex]= vertex;
		++(tile->nvertex);
		vertex->tile[vertex->ntiles]= tile;
		++(vertex->ntiles);

		/* find next line that touches tile */
		for(i=0; i < ndir; ++i) {
			lin= dir[i];
			if (lin->tile[0] == tile
				|| (lin->ntiles == 2 && lin->tile[1] == tile)) break;
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
 * Connect each tile to the vertex and lines it touches. Also connect
 * each vertex to the tiles it touches.
 * Before this point only lines had references to which tiles they touch.
 * This function builds tracks which lines each tile touches, which vertex
 * each tile touches, and which tiles each vertex touches.
 */
static void
geometry_connect_tiles(struct geometry *geo)
{
	int i, j;
	struct line *lin;
	struct vertex *vertex;
	struct tile *tile;
	int direction;

	/* count number of vertex and number of lines touching each tile
	   NOTE: notice that tile->nvertex and vertex->ntiles are counted twice.
	   We're over-counting them by a factor of 2. */
	lin= geo->lines;
	for(i=0; i < geo->nlines; ++i) {
		vertex= lin->ends[0];
		for(j=0; j < lin->ntiles; ++j) {
			tile= lin->tile[j];
			++(tile->nvertex);
			++(tile->nsides);
			++(vertex->ntiles);
		}
		vertex= lin->ends[1];
		for(j=0; j < lin->ntiles; ++j) {
			tile= lin->tile[j];
			++(tile->nvertex);
			++(tile->nsides);
			++(vertex->ntiles);
		}
		++lin;
	}

	/* allocate space for tiles in each vertex.
	   Compensate for over-counting ntiles (factor 2) */
	vertex= geo->vertex;
	for(i=0; i < geo->nvertex; ++i) {
		vertex->tile= (struct tile **)g_malloc0((vertex->ntiles/2)*sizeof(void*));
		vertex->ntiles= 0;	/* to be used as counter below */
		++vertex;
	}

	/* allocate space for vertices and lines in each tile.
	   Compensate for over-counting nvertex (factor 2) */
	tile= geo->tiles;
	for(i=0; i < geo->ntiles; ++i) {
		tile->vertex= (struct vertex **)g_malloc0((tile->nvertex/2)*sizeof(void*));
		tile->nvertex= 0;			/* to be used as counter below */
		tile->sides= (struct line **)g_malloc0((tile->nsides/2)*sizeof(void*));
		tile->nsides= 0;			/* to be used as counter below */
		++tile;
	}

	/* try each line and follow it around the tiles it touches
	   while doing this, set tile's sides & vertex, and set
	   vertex's tiles
	   NOTE: tile->fx_status is reused (shamelessly) as a mask to know
	   which tiles have been already handled. */
	lin= geo->lines;
	for(i=0; i < geo->nlines; ++i) {
		tile= lin->tile[0];
		if (tile->fx_status == 0) {
			geometry_go_around_tile(tile, lin);
			tile->fx_status= 1;
		}
		if (lin->ntiles == 2) {
			tile= lin->tile[1];
			if (tile->fx_status == 0) {
				geometry_go_around_tile(tile, lin);
				tile->fx_status= 1;
			}
		}
		++lin;
	}

	/* reset fx_status, which was used as a mask */
	tile= geo->tiles;
	for(i=0; i < geo->ntiles; ++i) {
		g_assert(tile->fx_status == 1);
		tile->fx_status= 0;
		++tile;
	}
}


/*
 * Calculate the center point of each tile
 */
static void
geometry_calculate_tile_centers(struct geometry *geo)
{
	int i, j;
	struct tile *tile;

	tile= geo->tiles;
	for(i=0; i < geo->ntiles; ++i) {
		/* calculate position of center of tile */
		tile->center.x= tile->vertex[0]->pos.x;
		tile->center.y= tile->vertex[0]->pos.y;
		for(j=1; j < tile->nvertex; ++j) {
			tile->center.x+= tile->vertex[j]->pos.x;
			tile->center.y+= tile->vertex[j]->pos.y;
		}
		tile->center.x= tile->center.x/(double)tile->nvertex;
		tile->center.y= tile->center.y/(double)tile->nvertex;
		++tile;
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
	struct tile *tile;

	lin= geo->lines;
	for(i=0; i<geo->nlines; ++i) {
		v1= lin->ends[0];
		lin->inf[0].x= v1->pos.x;
		lin->inf[0].y= v1->pos.y;
		tile= lin->tile[0];
		lin->inf[1].x= tile->center.x;
		lin->inf[1].y= tile->center.y;
		v1= lin->ends[1];
		lin->inf[2].x= v1->pos.x;
		lin->inf[2].y= v1->pos.y;
		if (lin->ntiles == 2) {
			tile= lin->tile[1];
			lin->inf[3].x= tile->center.x;
			lin->inf[3].y= tile->center.y;
		} else {	// edge line, must manufacture 4th point
			/* make up point across line:
			 along line joining center of tile to center of line */
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
 * Measure smallest width and height of all tiles
 */
static void
geometry_measure_tiles(struct geometry *geo)
{
	struct tile *tile;
	int i, j, j2;
	double tilew, tileh, tmp;

	/* go around all tiles to measure smallest w and h */
	tile= geo->tiles;
	for(i=0; i<geo->ntiles; ++i) {
		tilew= tileh= 0.;
		for(j=0; j < tile->nvertex; ++j) {
			j2= (j + 1) % tile->nvertex;
			tmp= fabs(tile->vertex[j]->pos.x -
				  tile->vertex[j2]->pos.x);
			if (tmp > tilew) tilew= tmp;
			tmp= fabs(tile->vertex[j]->pos.y -
				  tile->vertex[j2]->pos.y);
			if (tmp > tileh) tileh= tmp;
		}
		if (i == 0) {
			geo->tile_width= tilew;
			geo->tile_height= tileh;
		} else {
			if (tilew < geo->tile_width) geo->tile_width= tilew;
			if (tileh < geo->tile_height) geo->tile_height= tileh;
		}
		++tile;
	}
}


/*
 * Adds vertex to list in skeleton geometry.
 * Returns a pointer to already exisiting vertex if it is already in the list.
 * Returns a pointer to a newly added vertex otherwise.
 */
static struct vertex*
geometry_add_vertex(struct geometry *geo, struct point *point)
{
	int i;
	struct vertex *vertex;
	double x, y;

	/* search backwards to optimize */
	vertex= geo->vertex + (geo->nvertex - 1);
	for(i=0; i < geo->nvertex; ++i) {
		x= point->x - vertex->pos.x;
		y= point->y - vertex->pos.y;
		if ( x*x + y*y < DISTANCE_RESOLUTION_SQUARED)
			break;
		--vertex;
	}
	if (i == geo->nvertex) {		/* not found, create new */
		vertex= geo->vertex + geo->nvertex;
		vertex->id= geo->nvertex;
		vertex->pos.x= point->x;
		vertex->pos.y= point->y;
		vertex->nlines= 0;
		vertex->lines= NULL;
		vertex->ntiles= 0;
		vertex->tile= NULL;
		++geo->nvertex;
	}

	return vertex;
}


/*
 * Adds line to list in skeleton geometry.
 * Returns a pointer to already exisiting line if it is already in the list.
 * Returns a pointer to a newly added line otherwise.
 */
static struct line*
geometry_add_line(struct geometry *geo, struct vertex *v1, struct vertex *v2)
{
	int i;
	struct line *lin;

	/* search backwards to optimize */
	lin= geo->lines + (geo->nlines - 1);
	for(i=0; i < geo->nlines; ++i) {
		if ((lin->ends[0] == v1 && lin->ends[1] == v2) ||
			(lin->ends[1] == v1 && lin->ends[0] == v2))
			break;
		--lin;
	}
	if (i == geo->nlines) {		/* not found, create new */
		lin= geo->lines + geo->nlines;
		lin->id= geo->nlines;
		lin->ends[0]= v1;
		lin->ends[1]= v2;
		lin->ntiles= 0;
		lin->tile[0]= NULL;
		lin->tile[1]= NULL;
		lin->nin= 0;
		lin->in= NULL;
		lin->nout= 0;
		lin->out= NULL;
		lin->fx_status= 0;
		lin->fx_frame= 0;
		++geo->nlines;
	}

	return lin;
}


/*
 * Add tile to list of tiles in skeleton geometry.
 * Add vertices and lines as required (avoiding repetitions).
 * IMPORTANT: the points given must wrap around the tile in order,
 * either clockwise or counter-clockwise.
 * Lines are created with info about which tiles and vertices they touch.
 * Only lines are connected in this manner because in a skeleton geometry
 * it's only the lines that have room to store this data (they always
 * touch 2 tiles and vertices).
 */
void
geometry_add_tile(struct geometry *geo, struct point *pts, int npts)
{
	struct tile *tile;
	struct vertex *vertex;
	struct vertex *vertex_first;
	struct vertex *vertex_prev=NULL;
	struct line *lin;
	int i;

	tile= geo->tiles + geo->ntiles;
	tile->id= geo->ntiles;
	tile->nvertex= 0;
	tile->vertex= NULL;
	tile->nsides= 0;
	tile->sides= NULL;
	tile->fx_status= 0;
	tile->fx_frame= 0;
	++geo->ntiles;

	/* add first vertex */
	vertex_first= geometry_add_vertex(geo, pts);
	vertex_prev= vertex_first;

	/* add rest of vertices */
	for (i=1; i < npts; ++i) {
		vertex= geometry_add_vertex(geo, pts + i);

		/* add line connecting last two points*/
		lin= geometry_add_line(geo, vertex_prev, vertex);
		/* store tile in line */
		g_assert(lin->ntiles < 2);	/* no more than 2 tiles touching line */
		lin->tile[lin->ntiles]= tile;
		++lin->ntiles;

		vertex_prev= vertex;
	}

	/* connect last point to first */
	lin= geometry_add_line(geo, vertex, vertex_first);
	g_assert(lin->ntiles < 2);		/* no more than 2 tiles touching line */
	lin->tile[lin->ntiles]= tile;
	++lin->ntiles;
}


/*
 * Set distance resolution to use.
 * Two points that are closer than this distance are considered the same point.
 * We store its tile for efficiency.
 */
void
geometry_set_distance_resolution(double distance)
{
	DISTANCE_RESOLUTION_SQUARED= distance * distance;
}


/*
 * A fully connected geometry is formed from a simple skeleton geometry
 * The skeleton geometry has:
 *  - All tiles defined:
 *    * only id is set
 *  - All vertex defined
 *    * id and pos are set
 *  - All lines defined
 *    * id, ends and tile fields are set
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

	/* connect tiles to lines and vertices */
	geometry_connect_tiles(geo);

	/* find tile centers */
	geometry_calculate_tile_centers(geo);

	/* define area of influence of each line */
	geometry_define_line_infarea(geo);

	/* measure minimum tile dimensions */
	geometry_measure_tiles(geo);
}


/*
 * Create new geometry
 */
struct geometry*
geometry_create_new(int ntiles, int nvertex, int nlines, int max_numlines)
{
	struct geometry *geo;
	int i;

	/* Allocate memory for geometry data */
	geo= (struct geometry*)g_malloc(sizeof(struct geometry));
	geo->ntiles= ntiles;
	geo->nvertex= nvertex;
	geo->nlines= nlines;
	geo->tiles= (struct tile*)g_malloc(geo->ntiles*sizeof(struct tile));
	geo->vertex= (struct vertex*)g_malloc(geo->nvertex*sizeof(struct vertex));
	geo->lines= (struct line*)g_malloc(geo->nlines*sizeof(struct line));
	geo->numpos= (struct point *)g_malloc(max_numlines*sizeof(struct point));
	geo->numbers= (char *)g_malloc(2*max_numlines*sizeof(char));
	for(i=0; i < max_numlines; ++i)
		snprintf(geo->numbers + 2*i, 2, "%1d", i);
	geo->tile_width= 0.;
	geo->tile_height= 0.;
	geo->on_line_width= 0.;
	geo->off_line_width= 0.;
	geo->cross_line_width= 0.;
	geo->cross_radius= 0.;
	geo->font_size= 0.;
	geo->font_scale= 1.;
	geo->max_numlines= max_numlines;
	geo->board_size= 0.;
	geo->board_margin= 0.;
	geo->game_size= 0.;

	return geo;
}


/*
 * Free memory used by a geometry structure
 */
void
geometry_destroy(struct geometry *geo)
{
	int i;

	for(i=0; i < geo->ntiles; ++i) {
		g_free(geo->tiles[i].vertex);
		g_free(geo->tiles[i].sides);
	}
	for(i=0; i < geo->nvertex; ++i) {
		g_free(geo->vertex[i].lines);
		g_free(geo->vertex[i].tile);
	}
	for(i=0; i < geo->nlines; ++i) {
		g_free(geo->lines[i].in);
		g_free(geo->lines[i].out);
	}
	g_free(geo->tiles);
	g_free(geo->vertex);
	g_free(geo->lines);
	g_free(geo->numbers);
	g_free(geo->numpos);
	g_free(geo);
}
