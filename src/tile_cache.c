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


#define NUM_TILES_PER_SIDE	10
#define NUM_TILES			NUM_TILES_PER_SIDE*NUM_TILES_PER_SIDE


/* defined in gamedata.c */
extern struct board board;


/*
 * Deallocate old tile cache
 */
static void
clear_tile_cache(void)
{
	int i;
	int numtiles;		// total number of tiles (ntiles^2)

	numtiles= board.tile_cache->ntiles;
	for(i=0; i < numtiles; ++i) {
		/* free list for tile i */
		g_slist_free(board.tile_cache->tiles[i]);
	}
	g_free(board.tile_cache->tiles);	/* free tiles array */
	g_free(board.tile_cache);		/* free cache struct */
	board.tile_cache= NULL;
}


/*
 * Initialize tile cache (list of lines in each tile):
 * The board is divided in NxN squares
 * on the game board
 */
void
setup_tile_cache(void)
{
	int l, b;
	struct line *lin;
	struct point edge[2];
	gboolean inside;
	struct tile_cache *cache;
	GSList **tiles;

	/* empty tile_cache first if necessary */
	if (board.tile_cache != NULL)
		clear_tile_cache();

	/* new tile_cache */
	cache= (struct tile_cache*)g_malloc(sizeof(struct tile_cache));
	cache->ntiles_side= NUM_TILES_PER_SIDE;
	cache->ntiles= cache->ntiles_side * cache->ntiles_side;
	cache->tile_size= board.geo->board_size / cache->ntiles_side;
	board.tile_cache= cache;

	/* Initialize line status vector */
	tiles= (GSList **) g_malloc0(cache->ntiles * sizeof(GSList *));
	// **TODO** handle mem error
	cache->tiles= tiles;

	/* Iterate through the tiles: build list of lines in each tile */
	for(b=0; b < NUM_TILES; ++b) {
		edge[0].x= (b % cache->ntiles_side) * cache->tile_size;
		edge[0].y= (b / cache->ntiles_side) * cache->tile_size;
		edge[1].x= edge[0].x + cache->tile_size;
		edge[1].y= edge[0].y + cache->tile_size;
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
