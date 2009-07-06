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
	void *info;			// properties depending on type of tile
};



/* info structure for square tile geometries */
struct square_tile_info {
	int width;
	int height;
};

/* info structure for penrose tile geometries */
struct penrose_tile_info {
	int size_index;
};



/* square-tile.c */
struct geometry* build_square_tile_geometry(const struct square_tile_info *info);
struct gameinfo* build_square_gameinfo(int width, int height);

/* penrose-tile.c */
struct geometry* build_penrose_tile_geometry(const struct penrose_tile_info *info);
struct gameinfo* build_penrose_gameinfo(int size_index);


#endif
