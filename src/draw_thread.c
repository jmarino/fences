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
 
#include <gtk/gtk.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include "draw.h"

static int currently_drawing=0;		// are we drawing now?
static int expose_disabled=0;		// expose of drawarea disabled?

/* cairo variables */
static cairo_surface_t *csurf=NULL;	// cairo surf where we draw (gtk independent)
static cairo_t *cr=NULL;		// cairo context (gtk independent)


static pthread_t thread_info;		// thread info



/*
 * Main draw routine run in a thread
 */
static void*
draw_thread(void *drawarea)
{
	siginfo_t info;
	sigset_t sigset;
	static int old_width=0;
	static int old_height=0;
	
	/* look for SIGALRM */
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGALRM);
	
	while(1) {
		/* sit here waiting for our SIGALRM to start drawing */
		while (sigwaitinfo(&sigset, &info) > 0) {
			printf("draw_thread: about to draw\n");
			g_atomic_int_set(&currently_drawing, 1);
			
			int width, height;
			gdk_threads_enter();
			get_pixmap_size(&width, &height);
			gdk_threads_leave();
			
			/* size changed, reallocate cairo stuff */
			if (old_width != width || old_height != height) {
				if (cr != NULL) cairo_destroy(cr);
				if (csurf != NULL) cairo_surface_destroy(csurf);
				csurf= cairo_image_surface_create
					(CAIRO_FORMAT_ARGB32, width, height);
				cr= cairo_create(csurf);
			}
			
			/* draw board */
			draw_board(cr, width, height);
			
			/* copy newly drawn cairo surface on gtk pixmap */
			gdk_threads_enter();
			copy_board_on_pixmap(csurf);
			
			/* allow expose before we send expose request */
			g_atomic_int_set(&expose_disabled, 0);
			
			/* schedule expose on window to refresh window */
			gtk_widget_queue_draw_area(GTK_WIDGET(drawarea), 
						   0, 0, width, height);
			gdk_threads_leave();	
			
			/* done drawing */
			g_atomic_int_set(&currently_drawing, 0);
			printf("draw_thread: drawing done\n");
		}
	}
	
	
}


/*
 * Function called when timer expires (to keep animation frame rate)
 */
gboolean
timer_function(GtkWidget *drawarea){
	/* use atomic function to avoid multithreading issues */
	int drawing_status = g_atomic_int_get(&currently_drawing);
	
	/* If not drawing, signal thread to draw (with SIGALRM) */
	if (drawing_status == 0) {
		pthread_kill(thread_info, SIGALRM);
		//printf("timer_function: signal sent\n");
	}
	
	/* schedule expose on window to refresh window */
	//int width, height;
	//get_pixmap_size(&width, &height);
	//gtk_widget_queue_draw_area(drawarea, 0, 0, width, height);
	
	return TRUE;	/* keep it running */
}


/*
 * Signal a redraw
 */
void
request_draw(GtkWidget *drawarea)
{
	/* if already busy drawing, ignore */
	if (g_atomic_int_get(&currently_drawing) == 1)
		return;
	
	/* notify thread to draw */
	pthread_kill(thread_info, SIGALRM);
	/* disable expose (to avoid flickering while resizing, for example)
	  thread will reenable after it's done */
	g_atomic_int_set(&expose_disabled, 1);
}


/*
 * Start separate draw thread, must be called from main.c as part of the 
 * initialization.
 */
void
start_draw_thread(GtkWidget *drawarea)
{
	static int first_time=1;
	int iret;
	
	/* make sure this is only run once */
	if (!first_time) return;
	
	/* block SIGALRM in the main thread */
	sigset_t sigset;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGALRM);
	pthread_sigmask(SIG_BLOCK, &sigset, NULL);
	
	/* create the drawing thread */
	iret = pthread_create(&thread_info, NULL, draw_thread, drawarea);
	first_time= 0;
	
	printf("draw thread started: %d\n", iret);
}


/*
 * Stop drawing thread and free resources
 */
void
end_draw_thread(void)
{
	
}


/*
 * Is expose disabled? To be used from outside this file, i.e., expose callback
 */
inline int
is_expose_disabled(void)
{
	return g_atomic_int_get(&expose_disabled);
}
