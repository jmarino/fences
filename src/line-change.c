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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib.h>
#include <stdio.h>

#include "gamedata.h"
#include "game-solver.h"


/*
 * Check if game is complete
 */
static gboolean
is_game_finished(struct board *board)
{
	struct game *game=board->game;
	int i;

	g_assert(game != NULL);
	if (game->nlines_on != game->solution_nlines_on)
		return FALSE;
	/* check solution is correct */
	for(i=0; i < board->geo->nlines; ++i) {
		if (game->states[i] == LINE_ON || game->solution[i] == LINE_ON) {
			if (game->states[i] != game->solution[i]) return FALSE;
		}
	}
	return TRUE;
}


/*
 * Check if vertices that touch line have more than two lines ON
 */
static void
linechange_check_vertices(struct board *board, struct line *line_changed)
{
	struct game *game=board->game;
	struct vertex *vertex;
	struct line *lin;
	int i, j;
	int num_on;
	int old_state;
	struct clipbox clip;

	for(i=0; i < 2; ++i) {
		vertex= line_changed->ends[i];
		/* count number of ON lines */
		num_on= 0;
		for(j=0; j < vertex->nlines; ++j) {
			lin= vertex->lines[j];
			if (game->states[lin->id] == LINE_ON) ++num_on;
		}
		old_state= vertex->display_state;
		if (num_on > 2) {
			if (vertex->display_state != DISPLAY_ERROR) {
				vertex->display_state= DISPLAY_ERROR;
			}
		} else {
			vertex->display_state= DISPLAY_NORMAL;
		}
		if (old_state != vertex->display_state) {
			clip.x= vertex->pos.x - board->geo->tile_width/4;
			clip.y= vertex->pos.y - board->geo->tile_height/4;
			clip.w= board->geo->tile_width/2;
			clip.h= board->geo->tile_height/2;
			geometry_update_clip(board->geo, &clip);
		}
	}
}


/*
 * Check if tiles that touch line have too many lines ON
 */
static void
linechange_check_tiles(struct board *board, struct line *line_changed)
{
	struct game *game=board->game;
	struct tile *tile;
	struct line *lin;
	int i, j;
	int tile_number;
	int num_on;
	int old_state;
	struct clipbox clip;

	for(i=0; i < line_changed->ntiles; ++i) {
		tile= line_changed->tiles[i];
		tile_number= game->numbers[tile->id];
		if (tile_number == -1) continue;
		/* count number of ON lines around tile */
		num_on= 0;
		for(j=0; j < tile->nsides; ++j) {
			lin= tile->sides[j];
			if (game->states[lin->id] == LINE_ON) ++num_on;
		}
		old_state= tile->display_state;
		if (num_on > tile_number) {
			if (tile->display_state != DISPLAY_ERROR) {
				tile->display_state= DISPLAY_ERROR;
			}
		} else {
			tile->display_state= DISPLAY_NORMAL;
		}
		if (old_state != tile->display_state) {
			clip.x= tile->center.x - board->geo->tile_width;
			clip.y= tile->center.y - board->geo->tile_height;
			clip.w= board->geo->tile_width*2;
			clip.h= board->geo->tile_height*2;
			geometry_update_clip(board->geo, &clip);
		}
	}
}


/*
 * Perform change in game state
 */
inline void
make_line_change(struct board *board, struct line_change *change)
{
	struct line *line_changed;
	struct clipbox clip;

	board->game->states[change->id]= change->new_state;
	if (change->old_state == LINE_ON) --board->game->nlines_on;
	else if (change->new_state == LINE_ON) ++board->game->nlines_on;

	/* set clip box */
	line_changed= board->geo->lines + change->id;
	clip.x= line_changed->inf_box[0].x;
	clip.y= line_changed->inf_box[0].y;
	clip.w= line_changed->inf_box[1].x;
	clip.h= line_changed->inf_box[1].y;
	geometry_set_clip(board->geo, &clip);

	/* did we just solve the game? */
	if (is_game_finished(board)) {
		printf("Game Finished!!\n");
		return;
	}

	/* check for errors */
	linechange_check_vertices(board, line_changed);
	linechange_check_tiles(board, line_changed);
}
