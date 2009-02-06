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
#include <math.h>
#include <stdio.h>

#include "geometry.h"

#define NUM_TILES_PER_SIDE	10
#define NUM_TILES			NUM_TILES_PER_SIDE*NUM_TILES_PER_SIDE



/*
 * Is shape (given by 4 points) 'area' inside box (given by two corners) 'box'.
 * All 8 sides of box + area must be projected along all 8 directions defined
 * by the 8 sides. If just one of the 8 projections, shoes no overlap then
 * the shapes don't intersect.
 * Since box is a square, total number of projections is 6.
 * 	area: struct point[4]
 * 	box:  struct point[2]
 */
gboolean
is_area_inside_box(struct point *area, struct point *box, int debug)
{
	int i, j;
	double box_s[2];		// box shadow
	struct point area_s[2];		// area shadow
	double p;
	double angle;
	double sin_angle, cos_angle;
	
	/*
	 * project on the two directions defined by box (i.e. x and y axis)
	 */
	area_s[0].x= area_s[1].x= area[0].x;
	area_s[0].y= area_s[1].y= area[0].y;
	for (i=1; i < 4; ++i) {
		if (area[i].x < area_s[0].x) area_s[0].x= area[i].x;
		if (area[i].x > area_s[1].x) area_s[1].x= area[i].x;
		if (area[i].y < area_s[0].y) area_s[0].y= area[i].y;
		if (area[i].y > area_s[1].y) area_s[1].y= area[i].y;
	}
	if (area_s[1].x < box[0].x || area_s[0].x > box[1].x)
		return FALSE;
	if (area_s[1].y < box[0].y || area_s[0].y > box[1].y)
		return FALSE;
	if (debug) {
		printf("area: (%lf,%lf) ; (%lf,%lf) ; (%lf, %lf) ; (%lf, %lf)\n",
		       area[0].x, area[0].y, area[1].x, area[1].y,
		       area[2].x, area[2].y, area[3].x, area[3].y);
		printf("x -> area_s: %lf,%lf\t;\tbox: %lf, %lf\n",
		       area_s[0].x, area_s[1].x,
		       box[0].x, box[1].x);
		printf("y -> area_s: %lf,%lf\t;\tbox: %lf, %lf\n",
		       area_s[0].y, area_s[1].y,
		       box[0].y, box[1].y);
	}

	/*
	 * project on 4 directions defined by area
	 */
	for(i=0; i < 4; ++i) {
		j= (i + 1) % 4;
		/* angle between side and x axis 
		 * All points will be rotated by -angle so side (i..j) aligns
		 * with x axis. 
		 * The projection on y axis will give. */
		angle= -atan2(area[j].y - area[i].y, area[j].x - area[i].x);
		sin_angle= sin(angle);
		cos_angle= cos(angle);
		
		/* Apply rotation to all points in box (just want y coord) */
		box_s[0]= box_s[1]= box[0].y*cos_angle + box[0].x*sin_angle;
		p= box[0].y*cos_angle + box[1].x*sin_angle;
		if (p < box_s[0]) box_s[0]= p;
		if (p > box_s[1]) box_s[1]= p;
		p= box[1].y*cos_angle + box[1].x*sin_angle;
		if (p < box_s[0]) box_s[0]= p;
		if (p > box_s[1]) box_s[1]= p;
		p= box[1].y*cos_angle + box[0].x*sin_angle;
		if (p < box_s[0]) box_s[0]= p;
		if (p > box_s[1]) box_s[1]= p;
		
		/* Apply rotation to all points in area (just want x coord) */
		area_s[0].y= area_s[1].y=
			area[0].y*cos_angle + area[0].x*sin_angle;
		for(j=1; j < 4; ++j) {
			p= area[j].y*cos_angle + area[j].x*sin_angle;
			if (p < area_s[0].y) area_s[0].y= p;
			if (p > area_s[1].y) area_s[1].y= p;
		}
		if (area_s[1].y < box_s[0] || area_s[0].y > box_s[1])
			return FALSE;
	}
	return TRUE;
}


/*
 * Determine if 'point' is inside area defined by four points 'area[4]'
 */
gboolean
is_point_inside_area(struct point *point, struct point *area)
{
	struct point center;
	int i, i2;
	double a, b;
	double denom;
	
	/* find central point */
	center.x= area[0].x;
	center.y= area[0].y;
	for(i=1; i < 4; ++i) { 
		center.x+= area[i].x;
		center.y+= area[i].y;
	}
	center.x/= 4.;
	center.y/= 4.;

	/* calculate the intersection of center point with all sides
	 * of area. If intersection is between corners -> outside
	 * 	L1: P0 + a (P1 - P0)		# P0,P1 are corners
	 * 	L2: center + b (point - center)
	 * 	Solving for L1==L2, gives a and b
	 * 	  a=(xp-xc)(y0-yc)-(yp-yc)(x0-xc)/((yp-yc)(x1-x0)-(xp-xc)(y1-y0))
	 * 	  b=(x1-x0)(y0-yc)-(y1-y0)(x0-xc)/((yp-yc)(x1-x0)-(xp-xc)(y1-y0))
	 * 	Point is outside if a<1 for any corner of area
	 */
	for(i=0; i < 4; ++i) {
		i2= (i + 1) % 4;
		denom= ((double)point->y - center.y)*(area[i2].x - area[i].x) -
			((double)point->x - center.x)*(area[i2].y - area[i].y);
		a= ( ((double)point->x - center.x)*(area[i].y - center.y) -
		     ((double)point->y - center.y)*(area[i].x - center.x) )/denom;
		b= ( ((double)area[i2].x - area[i].x)*(area[i].y - center.y) -
		     ((double)area[i2].y - area[i].y)*(area[i].x - center.x) )/denom;
		
		if (a >= 0.0 && a <= 1.0 && b >= 0.0 && b <= 1.0) return FALSE;
	}
	return TRUE;
}


