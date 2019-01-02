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

int order_moves(int primo_move)
{
    Move *mp;
    int  f, fp;
    int  t, tp;
    Knowledge *know = know_stack + ply;

    for (mp = ML_FIRST; mp < ML_LAST; mp++) {
	f = FROM(mp->move);
	t = TO(mp->move);

	if (mp->move == primo_move) {
	    mp->score = 1000000;
	}
	else if (mp->move & CAPTURE) {
	    fp = Square(f);
	    tp = Square(t);

	    if (piece_value[tp] > piece_value[fp]) {
	        mp->score = 800000 + piece_value[tp] - piece_value[fp];
	    }
	    else if (piece_value[tp] == piece_value[fp]) {
	        mp->score = 750000 + piece_value[tp];
	    }
	    else {
		if (quicksee(mp->move) > 0)
		    mp->score = 600000;
		else
		    mp->score = 550000;
	        mp->score += piece_value[tp] - piece_value[fp];
	    }

	}
	else if (mp->move == know->mate_killer) {
	    mp->score = 900000;
	}
	else if (mp->move == know->killer1) {
	    mp->score = 500000;
	}
	else if (mp->move == know->killer2) {
	    mp->score =  400000;
	}
	else {
	    mp->score = (history[board.side][f][t] << MAX_HIST_SHIFT) / (maxhist + 1);
	}
    }
    know->ml_last = ML_LAST - 1;
    return 0;
}

int order_q()
{
    Move *mp;
    int  f, fp;
    int  t, tp;

    for (mp = ML_FIRST; mp < ML_LAST; mp++) {
	f = FROM(mp->move);
	t = TO(mp->move);
	mp->score = 0;
	if (mp->move & CAPTURE) {
	    fp = board.square[f];
	    tp = board.square[t];

	    mp->score += 100000;
	    mp->score += piece_value[tp] - piece_value[fp];

	    if ((piece_value[fp] - piece_value[tp]) > 0)  {
		 if (quicksee(mp->move) < 0)
			 mp->score = -1;
	    }
	}
    }

    (know_stack+ply)->ml_last = ML_LAST - 1;
    return 0;
}


int next_move()
{
    int bestscore = -1;
    Move *best = 0;
    Move *mp;
    Knowledge *know = know_stack + ply;

    for (mp = know->ms; mp <= know->ml_last; mp++) {
	if (mp->score > bestscore) {
	    bestscore = mp->score;
	    best = mp;
	}
    }

    if (best) {
	Move tmp_move;

	tmp_move = *best;
	*best = *know->ml_last;
	*know->ml_last = tmp_move;

	know->ml_last->score = -1;
	know->ml_last--;
	return(tmp_move.move);
    }
    else {
	return 0;
    }
}

int update_ordering(int move, int depth, int value)
{
    int f, t;
    Knowledge *know = know_stack + ply;

    if (!(move & CAPTURE )) {
	if (value > (INFINITY-100)) {
	    know->mate_killer = move;
	}
	else {
	    know->killer2 = know->killer1;
	    know->killer1 = move;
	}

	f = FROM(move);
	t = TO(move);
	history[board.side][f][t] += depth;

	if (history[board.side][f][t] > maxhist)
	    maxhist = history[board.side][f][t];

    }

    return 0;
}
