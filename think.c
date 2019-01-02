/*
    Diablo chess engine, Copyright 2005, Marcus Prewarski
          
    This program is free software. It may be copied or modified under the terms
    of the GNU General Public License version 2 as published by the Free Software 
    Foundation.  This program is distributed without any warranties whatsoever.
    See the file COPYING included with the distribution for details of the GNU general 
    public license.
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

#include "diablo.h"
#include "hash.h"
#include "search.h"
#include "attack.h"


int think()
{
    int i;
    int alpha = -INFINITY;
    int beta  =  INFINITY;
    int depth;
    int value = 0;
    int lastvalue = 0;

    init_search();

    for (i = 1; i <= max_search_depth; i++) {
	depth = PLY * i;
	idepth = i;
	value = root_search(depth, alpha, beta);

	if (stop_search) {
	    if (root_moves < 2)
		value = lastvalue;
	    break;
	}

	if ((value <= alpha) || (value >= beta))  {
            show_pv(value);
	    lastvalue = value;
	    value = root_search(depth, -INFINITY, INFINITY);
	    if (stop_search) {
	        if (root_moves < 2)
		    value = lastvalue;
		break;
	    }
	}

	if ((root_moves == 1) && (i > 3) && (in_check(board.side)) 
             && !(search_flags & (FLAG_ANALYZE | FLAG_PONDERING)))
	    break;

	lastvalue = value;

	alpha = value - 100;
	beta = value + 100;
	if (alpha < -INFINITY)
	    alpha = -INFINITY;

	if (beta > INFINITY)
	    beta = INFINITY;

	if (i > 2)
            show_pv(value);
    }
    show_pv(value);
    show_search_stats();

    return 0;
}


int init_search()
{
    int i;

    stop_search = 0;
    ply = 0;
    nodes = 0;
    maxhist = 0;
    age_hash();
    know_stack[0].material = 0;

    search_stats.ext = 0;
    search_stats.prom = 0;
    search_stats.pawn7 = 0;
    search_stats.single = 0;
    search_stats.mthreat = 0;
    search_stats.kt = 0;
    search_stats.simple = 0;
    search_stats.iid_cnt = 0;

    for (i = 0; i < 128; i++) {
	if (!(i & 0x88) && (Square(i) != EMPTY) && ((Square(i) & 7) != KING)) {
            know_stack[ply].material += piece_value[Square(i)];
	}
    }

    memset(pv, 0, sizeof(pv));
    memset(pv_length, 0, sizeof(pv_length));
    memset(history, 0, sizeof(history));
    start_time = gtime();

    return 0;
}

static unsigned int last_time;

int checkup()
{
    unsigned int cur_time;

    cur_time = gtime();
    if (is_input()) {
        if (stop_think()) {
            stop_search = 1;
        }
    }
    if ( !(search_flags & FLAG_ANALYZE)  && !(search_flags & FLAG_PONDERING)) {
        if (cur_time >= stop_time) {
            stop_search = 1;
        }
    }
    if (cur_time > (last_time + 5000)) {
	last_time = cur_time;
	show_update();
    }

    return 0;
}

