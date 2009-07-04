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

#include <sys/time.h>
#include <glib.h>

/* global variables to keep track of benchmarks */
static gboolean started=FALSE;
static struct timeval start_time;



/*
 * Start benchmark
 */
inline void
fences_benchmark_start(void)
{
	started= TRUE;
	gettimeofday (&start_time, NULL);
}


/*
 * Stop benchmark and return time
 */
inline double
fences_benchmark_stop(void)
{
	struct timeval end_time;

	gettimeofday (&end_time, NULL);
	if (!started)
		return 0.0;
	started= FALSE;
	return ((double)(end_time.tv_sec - start_time.tv_sec))*1000000. \
		+ ((double)(end_time.tv_usec - start_time.tv_usec));
}
