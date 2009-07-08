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

#include <glib.h>

#include "gamedata.h"
#include "geometry_tools.h"


#define WIDTH_MESH	10
#define SIZE_MESH			WIDTH_MESH*WIDTH_MESH


/* defined in gamedata.c */
extern struct board board;


/*
 * Deallocate click mesh structure
 */
static void
clear_click_mesh(void)
{
	int i;
	int num_tiles;		// total number of mesh tiles

	num_tiles= board.click_mesh->ntiles;
	for(i=0; i < num_tiles; ++i) {
		/* free list for tile i */
		g_slist_free(board.click_mesh->tiles[i]);
	}
	g_free(board.click_mesh->tiles);	/* free mesh tile array */
	g_free(board.click_mesh);			/* free click_mesh struct */
	board.click_mesh= NULL;
}


/*
 * Initialize click mesh (list of lines in each mesh tile):
 * The board is divided in NxN squares
 * on the game board
 */
void
setup_click_mesh(void)
{
	int l, b;
	struct line *lin;
	struct point edge[2];
	gboolean inside;
	struct click_mesh *click_mesh;
	GSList **tiles;

	/* empty click_mesh first if necessary */
	if (board.click_mesh != NULL)
		clear_click_mesh();

	/* new click_mesh */
	click_mesh= (struct click_mesh*)g_malloc(sizeof(struct click_mesh));
	click_mesh->ntiles_side= WIDTH_MESH;
	click_mesh->ntiles= click_mesh->ntiles_side * click_mesh->ntiles_side;
	click_mesh->tile_size= board.geo->board_size / click_mesh->ntiles_side;
	board.click_mesh= click_mesh;

	/* Initialize line status vector */
	tiles= (GSList **) g_malloc0(click_mesh->ntiles * sizeof(GSList *));
	// **TODO** handle mem error
	click_mesh->tiles= tiles;

	/* Iterate through the mesh: build list of lines in each tile */
	for(b=0; b < SIZE_MESH; ++b) {
		edge[0].x= (b % click_mesh->ntiles_side) * click_mesh->tile_size;
		edge[0].y= (b / click_mesh->ntiles_side) * click_mesh->tile_size;
		edge[1].x= edge[0].x + click_mesh->tile_size;
		edge[1].y= edge[0].y + click_mesh->tile_size;
		lin= board.geo->lines;
		for(l=0; l < board.geo->nlines; ++l) {
			/* check if line's area of influence intersects the box */
			if(b == 0 && l == 22)
				inside= is_area_inside_box(lin->inf, edge, 1);
			else
				inside= is_area_inside_box(lin->inf, edge, 0);
			if (inside) {
				//printf("box %d (%lf,%lf;%lf,%lf): add line %d  (%lf,%lf)\n", b,
				//	   edge[0].x,edge[0].y, edge[1].x, edge[1].y, l,
				//	   lin->ends[0]->pos.x, lin->ends[0]->pos.y);
				tiles[b]= g_slist_prepend(tiles[b], lin);
			}
			++lin;
		}
	}
}
