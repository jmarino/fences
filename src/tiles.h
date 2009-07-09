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

#ifndef __INCLUDED_TILES_H__
#define __INCLUDED_TILES_H__


/* types of game tile */
enum {
	TILE_TYPE_SQUARE,
	TILE_TYPE_PENROSE,
	TILE_TYPE_TRIANGULAR
};


/*
 * info about game: tile type, size, ...
 */
struct gameinfo {
	int type;			// type of tile
	int size;			// size of game
	int diff_index;		// difficulty index: 0:Beginner,1:Easy,....
	double difficulty;	// actual difficulty: from 0 to 10
};



/* square-tile.c */
struct geometry* build_square_tile_geometry(const struct gameinfo *info);


/* penrose-tile.c */
struct geometry* build_penrose_tile_geometry(const struct gameinfo *info);


#endif