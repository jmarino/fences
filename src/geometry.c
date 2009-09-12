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
 * Print tree recursively (used for debug)
 * Initial example call: avltree_print(line_root, 0);
 */
static void
avltree_print(struct avl_node *root, int level)
{
	if (root == NULL) return;
	printf(" * %d(%d) [lev %d]\t", root->value.i, ((struct line*)root->data)->id, level);
	if (root->left)
		printf("( %d(%d) )", root->left->value.i, ((struct line *)root->left->data)->id);
	else
		printf("( nil  )");
	printf(" - ");
	if (root->right)
		printf("( %d(%d) )\n", root->right->value.i, ((struct line *)root->right->data)->id);
	else
		printf("( nil  )\n");

	if (root->left) avltree_print(root->left, level + 1);
	if (root->right) avltree_print(root->right, level + 1);
}


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
	struct line **ptr;

	/* iterate over lines and count how many lines touch each vertex */
	lin= geo->lines;
	for(i=0; i < geo->nlines; ++i) {
		++(lin->ends[0]->nlines);
		++(lin->ends[1]->nlines);
		++lin;
	}

	/* allocate space for each vertex's list of lines.
	   To optimize we allocate one single chunk of memory and point each vertex
	   to a position in it. Mem required is (2*nlines) since each line touches
	   two vertices.
	   NOTE: 'lines' pointer of first vertex points to start of memory chunk
	*/
	ptr= (struct line **)g_malloc((geo->nlines*2)*sizeof(void*));
	vertex= geo->vertex;
	for(i=0; i < geo->nvertex; ++i) {
		/* sanity check: all vertices must have at least two lines */
		if (vertex->nlines < 2) {
			g_message("CRITICAL: vertex %d has %d lines associated (needs at least 2)",
					  i, vertex->nlines);
		}
		vertex->lines= ptr;
		ptr= ptr + vertex->nlines;
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
	struct line **in_ptr, **out_ptr;
	int in_num=0;
	int out_num=0;

	/* count total number of in & out lines */
	lin= geo->lines;
	for(i=0; i < geo->nlines; ++i) {
		/* in lines for this line */
		in_num+= lin->ends[0]->nlines - 1;
		/* out lines for this line */
		out_num+= lin->ends[1]->nlines - 1;
		++lin;
	}

	/* store two big chunks of memory, then set pointers on it */
	in_ptr= (struct line **)g_malloc0(in_num*sizeof(void*));
	out_ptr= (struct line **)g_malloc0(out_num*sizeof(void*));
	/* populate the in & out arrays of each line */
	lin= geo->lines;
	for(i=0; i < geo->nlines; ++i) {
		/* store space for ins (lines touching vertex - 1) */
		vertex= lin->ends[0];
		lin->nin= vertex->nlines - 1;
		lin->in= in_ptr;
		for(j=0; j < vertex->nlines; ++j) {
			if (vertex->lines[j] == lin) continue;
			*in_ptr= vertex->lines[j];
			++in_ptr;
		}
		/* store space for outs (lines touching vertex - 1) */
		vertex= lin->ends[1];
		lin->nout= vertex->nlines - 1;
		lin->out= out_ptr;
		for(j=0; j < vertex->nlines; ++j) {
			if (vertex->lines[j] == lin) continue;
			*out_ptr= vertex->lines[j];
			++out_ptr;
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
		vertex->tiles[vertex->ntiles]= tile;
		++(vertex->ntiles);

		/* find next line that touches tile */
		for(i=0; i < ndir; ++i) {
			lin= dir[i];
			if (lin->tiles[0] == tile
				|| (lin->ntiles == 2 && lin->tiles[1] == tile)) break;
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
	int num_ele=0;
	struct tile **ptr_t;
	struct vertex **ptr_v;
	struct line **ptr_s;

	/* count number of vertex and number of lines touching each tile
	   NOTE: notice that tile->nvertex and vertex->ntiles are counted twice.
	   We're over-counting them by a factor of 2. */
	lin= geo->lines;
	for(i=0; i < geo->nlines; ++i) {
		vertex= lin->ends[0];
		for(j=0; j < lin->ntiles; ++j) {
			tile= lin->tiles[j];
			++(tile->nvertex);
			++(tile->nsides);
			++(vertex->ntiles);
			++num_ele;
		}
		vertex= lin->ends[1];
		for(j=0; j < lin->ntiles; ++j) {
			tile= lin->tiles[j];
			++(tile->nvertex);
			++(tile->nsides);
			++(vertex->ntiles);
			++num_ele;
		}
		++lin;
	}

	/* Compensate for over-counting elements (factor 2) */
	num_ele= num_ele/2;

	/* allocate space for tiles in each vertex. */
	ptr_t= (struct tile **)g_malloc0(num_ele*sizeof(void*));
	vertex= geo->vertex;
	for(i=0; i < geo->nvertex; ++i) {
		vertex->tiles= ptr_t;
		ptr_t+= vertex->ntiles/2;
		vertex->ntiles= 0;	/* to be used as counter below */
		++vertex;
	}

	/* allocate space for vertices and lines in each tile */
	tile= geo->tiles;
	ptr_v= (struct vertex **)g_malloc0(num_ele*2*sizeof(void*));
	ptr_s= (struct line **)(ptr_v + num_ele);
	for(i=0; i < geo->ntiles; ++i) {
		tile->vertex= ptr_v;
		ptr_v+= tile->nvertex/2;
		tile->nvertex= 0;			/* to be used as counter below */
		tile->sides= ptr_s;
		ptr_s+= tile->nsides/2;
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
		tile= lin->tiles[0];
		if (tile->fx_status == 0) {
			geometry_go_around_tile(tile, lin);
			tile->fx_status= 1;
		}
		if (lin->ntiles == 2) {
			tile= lin->tiles[1];
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
		tile= lin->tiles[0];
		lin->inf[1].x= tile->center.x;
		lin->inf[1].y= tile->center.y;
		v1= lin->ends[1];
		lin->inf[2].x= v1->pos.x;
		lin->inf[2].y= v1->pos.y;
		if (lin->ntiles == 2) {
			tile= lin->tiles[1];
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
 * Compare the value using the double field of the union (see avl_value)
 * returns 0 -> equal ; -1 -> test < value ; +1 -> test > value
 */
static int
value_cmp_double(avl_value *test, avl_value *value)
{
	double diff;

	diff= test->d - value->d;
	if (fabs(diff) < DISTANCE_RESOLUTION_SQUARED) {
		return 0;
	}
	if (diff < 0) return -1;
	return 1;
}


/*
 * Compare the value using the int field of the union (see avl_value)
 * returns 0 -> equal ; -1 -> test < value ; +1 -> test > value
 */
static int
value_cmp_int(avl_value *test, avl_value *value)
{
	if (test->i == value->i) return 0;
	if (test->i < value->i) return -1;
	return 1;
}


/*
 * Test if a given point has the same position as a given vertex
 * Returns: 0 -> equal (same position) ; !0 -> different
 */
static int
vertex_cmp(struct point *point, struct vertex *vertex)
{
	double x;
	double y;

	x= point->x - vertex->pos.x;
	y= point->y - vertex->pos.y;
	if ( x*x + y*y < DISTANCE_RESOLUTION_SQUARED)
		return 0;
	return 1;
}


/*
 * Adds vertex to list in skeleton geometry.
 * Returns a pointer to already exisiting vertex if it is already in the list.
 * Returns a pointer to a newly added vertex otherwise.
 */
static struct vertex*
geometry_add_vertex(struct geometry *geo, struct point *point)
{
	struct vertex *vertex;
	avl_value value;
	struct avl_node *parent;

	/* find existing vertex that represents 'point' */
	value.d= point->x*point->x + point->y*point->y;
	vertex= avltree_find(geo->vertex_root, &value, point, value_cmp_double,
						 AVLTREE_DATACMP(vertex_cmp), &parent);

	if (vertex == NULL) {		/* not found, create new */
		vertex= geo->vertex + geo->nvertex;
		vertex->id= geo->nvertex;
		vertex->pos.x= point->x;
		vertex->pos.y= point->y;
		vertex->nlines= 0;
		vertex->lines= NULL;
		vertex->ntiles= 0;
		vertex->tiles= NULL;
		++geo->nvertex;

		/* insert new vertex in AVL tree */
		geo->vertex_root= avltree_insert_node_at(parent, &value, vertex,
												 value_cmp_double);
	}

	return vertex;
}


/*
 * Test if a given line is equal
 * Returns: 0 -> equal (same position) ; !0 -> different
 */
static int
line_cmp(struct vertex *vertex, struct line *line)
{
	if (line->ends[0] == vertex || line->ends[1] == vertex)
		return 0;
	return 1;
}


/*
 * Adds line to list in skeleton geometry.
 * Returns a pointer to already exisiting line if it is already in the list.
 * Returns a pointer to a newly added line otherwise.
 */
static struct line*
geometry_add_line(struct geometry *geo, struct vertex *v1, struct vertex *v2)
{
	struct line *lin;
	avl_value value;
	struct avl_node *parent;

	/* find existing vertex that represents 'point' */
	if (v1->id < v2->id) {
		value.i= v1->id;
		lin= avltree_find(geo->line_root, &value, v2, value_cmp_int,
						  AVLTREE_DATACMP(line_cmp), &parent);
	} else {
		value.i= v2->id;
		lin= avltree_find(geo->line_root, &value, v1, value_cmp_int,
						  AVLTREE_DATACMP(line_cmp), &parent);
	}

	if (lin == NULL) {		/* not found, create new */
		lin= geo->lines + geo->nlines;
		lin->id= geo->nlines;
		lin->ends[0]= v1;
		lin->ends[1]= v2;
		lin->ntiles= 0;
		lin->tiles[0]= NULL;
		lin->tiles[1]= NULL;
		lin->nin= 0;
		lin->in= NULL;
		lin->nout= 0;
		lin->out= NULL;
		lin->fx_status= 0;
		lin->fx_frame= 0;
		++geo->nlines;

		/* insert new vertex in AVL tree */
		geo->line_root= avltree_insert_node_at(parent, &value, lin,	value_cmp_int);
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
 * center: center point of tile, if NULL is auto-calculated as centre of gravity
 */
void
geometry_add_tile(struct geometry *geo, struct point *pts, int npts,
				  struct point *center)
{
	struct tile *tile;
	struct vertex *vertex=NULL;
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

	/* calculate center of tile if needed */
	if (center == NULL) {
		tile->center.x= pts[0].x;
		tile->center.y= pts[0].y;
		for(i=1; i < npts; ++i) {
			tile->center.x+= pts[i].x;
			tile->center.y+= pts[i].y;
		}
		tile->center.x/= (double)npts;
		tile->center.y/= (double)npts;
	} else {
		tile->center.x= center->x;
		tile->center.y= center->y;
	}

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
		lin->tiles[lin->ntiles]= tile;
		++lin->ntiles;
		vertex_prev= vertex;
	}

	/* connect last point to first */
	lin= geometry_add_line(geo, vertex, vertex_first);
	g_assert(lin->ntiles < 2);		/* no more than 2 tiles touching line */
	lin->tiles[lin->ntiles]= tile;
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
	/* first free AVL trees, since they're not needed anymore */
	if (geo->vertex_root) {
		avltree_destroy(geo->vertex_root);
		geo->vertex_root= NULL;
	}
	if (geo->line_root) {
		avltree_destroy(geo->line_root);
		geo->line_root= NULL;
	}
	printf("ntiles: %d\n", geo->ntiles);
	printf("nvertex: %d\n", geo->nvertex);
	printf("nlines: %d\n", geo->nlines);

	/* connect vertices and lines */
	geometry_connect_vertex_lines(geo);

	/* create and populate in & out arrays in each line */
	geometry_fill_inout(geo);

	/* connect tiles to lines and vertices */
	geometry_connect_tiles(geo);

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
	geo->vertex_root= NULL;
	geo->line_root= NULL;

	return geo;
}


/*
 * Free memory used by a geometry structure
 */
void
geometry_destroy(struct geometry *geo)
{
	g_free(geo->tiles[0].vertex);
	g_free(geo->vertex[0].lines);
	g_free(geo->vertex[0].tiles);
	g_free(geo->lines[0].in);
	g_free(geo->lines[0].out);
	g_free(geo->tiles);
	g_free(geo->vertex);
	g_free(geo->lines);
	g_free(geo->numbers);
	g_free(geo->numpos);
	if (geo->vertex_root) avltree_destroy(geo->vertex_root);
	if (geo->line_root) avltree_destroy(geo->line_root);
	g_free(geo);
}
