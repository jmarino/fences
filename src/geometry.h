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

#ifndef __INCLUDED_GEOMETRY_H__
#define __INCLUDED_GEOMETRY_H__


/*
 * Holds coordinates of a point
 */
struct point {
	double x, y;
};


/*
 * Holds info about a vertex
 */
struct vertex {
	int id;			// id of vertex
	struct point pos;	// x,y coordinates of dot
	int nlines;		// number of lines touching dot
	struct line **lines;	// lines touching dot
};


/*
 * Holds info about a square
 */
struct square {
	int id;			// id of square
	int nvertex;		// number of vertices of 'square'
	struct vertex **vertex;	// vertices of 'square'
	int nsides;		// number of sides of 'square'
	struct line **sides;	// lines around 'square'
	struct point center;	// coords of center of square
	int fx_status;		// is it being animated? which animation?
	int fx_frame;		// frame in FX animation
};


/*
 * Holds info about a line
 */
struct line {
	int id;			// id of line (line number)
	struct vertex *ends[2];	// coords of ends of line
	int nsquares;		// Number of squares touching this line (either 1 or 2)
	struct square *sq[2];	// squares at each side of line
	int nin;		// number of lines in
	int nout;		// number of lines out
	struct line **in;	// lines in
	struct line **out;	// lines out
	struct point inf[4]; 	// coords of 4 points defining area of influence
	struct point inf_box[2];// [x,y] & [w,h] of box that contains line
	int fx_status;		// is it being animated? which animation?
	int fx_frame;		// frame in FX animation
};


/*
 * Describes game geometry (how lines, squares and dots connect to each other)
 */
struct geometry {
	int nvertex;		// Number of dots
	struct vertex *vertex;	// List of dots
	int nsquares;		// Number of squares
	struct square *squares;	// List of squares
	int nlines;
	struct line *lines;
	double sq_width;
	double sq_height;
};



/*
 * Functions
 */
void geometry_build_line_network(struct geometry *geo);
void geometry_define_line_infarea(struct geometry *geo);


#endif
