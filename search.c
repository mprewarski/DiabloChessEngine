/*
    Diablo chess engine, Copyright 2006, Marcus Prewarski
          
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
#include "order.h"
#include "util.h"
#include "data.h"
#include "attack.h"

static int q_search(int, int, int);
static int search(int, int, int, int);
static int init_node();
static int extensions(int, int, int, int);

int best_root_value = -INFINITY;
int pv[MAX_DEPTH][MAX_DEPTH];
int pv_length[MAX_DEPTH];

int root_search(int depth, int alpha, int beta) 
{
    int move;
    int value = -INFINITY;
    int ext = 0;
    int side = board.side;
    int xside = board.xside;

    best_root_value = -INFINITY;
    root_moves = 0;
    init_node();
    evaluate();
    generate_moves();
    order_moves(pv[0][0]);

    while ((move = next_move())) {
	make_move(move);
	if (!in_check(side)) {
	    root_moves++;

	    if ((know_stack[ply].check = in_check(xside)))
		ext = PLY;
	    else
	        ext = 0;

	    if (root_moves == 1) {
	    	value = -search(depth-PLY+ext, -beta, -alpha, 1);
	    }
	    else {
	        value = -search(depth-PLY+ext, -alpha-1, -alpha, 1);
	        if ((value > alpha) && (value < beta) && !(stop_search)) {
		    value = -search(depth-PLY+ext, -beta, -alpha, 1);
	        }
	    }

	}
	unmake_move(move);

	if (stop_search)
	    break;

	if (value > best_root_value) {
	        best_root_value = value;
		bestmove = move;
	        if (value > alpha) {
		    alpha = value;
		    if ((idepth > 2) && (bestmove != pv[0][0])) {
		    	update_pv(bestmove);
		        show_pv(best_root_value);
		    }
		    else
		    	update_pv(bestmove);

		    if (value >= beta) {
		        break;
		    }
	        }
	}
    }

    return best_root_value;
}

int search(int depth, int alpha, int beta, int do_null)
{
    int move;
    int incheck;
    int givescheck;
    int single_reply = 0;
    int hash_move = 0;
    int hash_flag = UPPER_BOUND;
    int value = -INFINITY;
    int bestvalue = -INFINITY;
    int best_move = 0;
    int ext = 0;
    int mate_threat = 0;
    int moves_searched = 0;
    int pv_node =  ((beta - alpha) > 1);
    int try_futility_cut = 0;
    int futility_margin = 0;
    Knowledge *know = know_stack + ply;
    int ev;

    if (!(nodes & 0x1fff))
	checkup();
    if (stop_search)
	return 0;
    if (repetition()) 
	return DRAW_SCORE;
    if ((depth < PLY) || (ply > MAX_SEARCHDEPTH))
	return q_search(depth, alpha, beta);

    init_node();

    switch(probe_hash(depth, &alpha, &beta, &hash_move, &do_null)) {
	case EXACT:       return alpha;
	case LOWER_BOUND: return beta;
	case UPPER_BOUND: return alpha;
    }

    ev = evaluate();
    incheck = attack_tab[board.xside][KSQ(board.side)];


    if (do_null && !incheck && (know->pieces[board.side]) && (ev >= beta)) {
	    int r, nv;

	if ((depth <= (1*PLY))) {
	    if ((!pv_node) && !know->extend && (know->material > ENDGAME_MAT) && 
		((ev - hung_value(board.side)) >= beta))
                return (beta);
	}
	else {

	    make_null_move();
	    know->check = 0; 
	    r = (know->pieces[board.side] > 2) ? (3*PLY)+2 : (3*PLY);
	    nv = -search(depth-r, -beta, -beta+1, 0 );
	    unmake_null_move();

	    /* if ((depth >= (5*PLY)) && (nv >= beta) && (know->pieces[board.side] <= 2)) {
	        nv = search(depth-(5*PLY), alpha, beta, 0 );
	        if (nv >= beta) {
	            return nv;
	        }
	    } */
	    if (nv >= beta) {
	        return nv;
	    }
	    if (nv < (-INFINITY + 200))
	        mate_threat = 1;

	    SET_ATTACK_TAB();
	}
    }

    if (incheck) {
        generate_evasions();
        if (((know+1)->ms - know->ms) == 1)
            single_reply = 1;
    }
    else {
        generate_moves();
    }

    // internal iterative deepening
    if (!hash_move && (depth >= (3*PLY))  && pv_node) {
	int v;
	search_stats.iid_cnt++;
	v = search(depth-(2*PLY), alpha, beta, 1 );
        // FIXME - check how well this works
	if (v <= alpha)
	    search(depth-(2*PLY), -INFINITY, beta, 1 );

	SET_ATTACK_TAB();
	hash_move = pv[ply][ply];
    }

    // test if we want to try a futility cut and assign a margin based on depth
    if ((depth < (3*PLY)) && !incheck && !pv_node) {
        try_futility_cut = 1;
	if (know->material < ENDGAME_MAT)
    	    futility_margin = (depth < (2*PLY)) ? 200 : 400;
	else
    	    futility_margin = (depth < (2*PLY)) ? 150 : 300;
    }

    order_moves(hash_move);
    while ((move = next_move())) {
        givescheck = isa_check(move);
        make_move(move);
        if (!in_check(board.xside)) {
            moves_searched++;
            ext = extensions(move, givescheck, mate_threat, single_reply);
	    (know_stack+ply)->extend = ext;

            // futility cut
            if (try_futility_cut && !ext && !(move & (CAPTURE | PROMOTION))) {
                if (ev < (alpha - futility_margin)) {
                    unmake_move(move);
                    continue;
                }
            }

            if (moves_searched == 1) {
                value = -search(depth-PLY+ext, -beta, -alpha, 1);
            }
            else  {
                value = -search(depth-PLY+ext, -alpha-1, -alpha, 1);
                if (value > alpha && value < beta) {
                    value = -search(depth-PLY+ext, -beta, -alpha, 1);
                }
            }
        }
        unmake_move(move);

	if (stop_search)
	    return(bestvalue);

	if (value > bestvalue) {
	        bestvalue = value;
		best_move = move;
	        if (value > alpha) {
		    alpha = value;
		    update_pv(move);
		    hash_flag = EXACT;
		    if (value >= beta) {
			update_ordering(move, depth, value);
    			store_hash(depth , bestvalue, LOWER_BOUND, best_move);
			return bestvalue;
		    }
	        }
	}
    }

    if (!moves_searched) {
        if (incheck)
            return(-INFINITY + ply);
        else
            return(DRAW_SCORE);
    }

    // FIXME experiment with this
    if (hash_flag == UPPER_BOUND)
        best_move = 0;

    store_hash(depth , bestvalue, hash_flag, best_move);

    return bestvalue;
}

static int q_search(int depth, int alpha, int beta)
{
    int value;
    int bestvalue;
    int move;
    int incheck;
    int legal_moves = 0;

    if (!(nodes & 0x1fff))
	checkup();
    if (stop_search)
	return 0;

    init_node();

    if (ply > MAX_SEARCHDEPTH)
	return(evaluate());

    if (repetition()) 
	return DRAW_SCORE;

    value = evaluate();

    if (value >= beta)
	return value;
    if (value > alpha)
	alpha = value;

    bestvalue = alpha;
    incheck = attack_tab[board.xside][KSQ(board.side)];

    if (incheck) {
        generate_evasions();
        if ((know_stack[ply+1].ms - know_stack[ply].ms) == 0)
            return(-INFINITY + ply);
        order_moves(0);
    }
    else {
        generate_captures();
        order_q();
    }

    while ((move = next_move())) {

	make_move(move);
	if (!in_check(board.xside)) {
	    legal_moves++;
	    value = -q_search(depth-PLY, -beta, -alpha);
	}
	unmake_move(move);

	if (value > bestvalue) {
	    bestvalue = value;
	    if (value > alpha) {
		alpha = value;
		if (value >= beta)
		    break;
	    }
	}
    }


    return alpha;
}

int update_pv(int move)
{
    int i;

    for (i = ply + 1;  i < pv_length[ply+1]; i++)
        pv[ply][i] = pv[ply + 1][i];

    pv[ply][ply] = move;
    pv_length[ply] = pv_length[ply+1];
    return 0;
}
static int init_node()
{
    nodes++;
    pv_length[ply] = ply;
    return 0;
}

static int extensions(int move, int givescheck, int mate_threat, int single_reply)
{
    int ext = 0;
    Knowledge *know = know_stack + ply;

    if (givescheck) {
        ext += PLY; search_stats.check++;
    }
    if (mate_threat) {
        ext += 3; search_stats.mthreat++;
    }
    if (move & QUEEN_PROM) {
        ext += 3; search_stats.prom++;
    }
    if (move & PAWNMOVE) {
        if (pawn_rank[board.xside][TO(move)] == 6) {
                ext += 3; search_stats.pawn7++;
        }
    }
    if (single_reply) {
        ext += 3; search_stats.single++;
    }

    if (know->material < ENDGAME_MAT) {
        if ((move & CAPTURE) &&
            (((know-1)->pieces[board.side] + ((know-1)->pieces[board.xside])) == 1) &&
            (piece_value[(know-1)->undo.capture] > PAWN_VAL) ) {
            ext += PLY;
            search_stats.simple++;
        }
    }

    if (ext > PLY)
        ext = PLY;

    (know-1)->extend = ext;

    return ext;
}

