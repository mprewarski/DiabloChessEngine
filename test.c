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
#include "util.h"
#include "attack.h"
#include "search.h"

char *test_fen[10] = {
	"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
	"rnbqk1nr/pp4b1/2p1p1B1/3pP2p/3P4/8/PPP2PPP/RNBQK2R b KQkq - 0 8",
	"r1b1kb1r/1pqppppp/p4n2/1Bp5/3nP3/2N2N2/PPPP1PPP/R1BQR1K1 w kq - 0 7",
	"rnbqkb1r/ppp2p1p/3p1n2/8/4PpNP/8/PPPP2P1/RNBQKB1R w KQkq - 0 7",
	"r3k2r/2q1bppp/p1b1pn2/1p2P3/8/P1N2B2/1PP3PP/R1BQ1R1K b kq - 0 14",
	"1n1b1kr1/r4p1p/pp1q4/2pNN2Q/8/6P1/PPP2PBP/2KR4 w - - 0 19",
	"3rr1k1/pp2bppp/1qp1n1b1/3n4/2BP4/P4NNP/1P1B1PP1/2RQR1K1 w - - 0 19",
	"4rbk1/2pb1qpp/p2p1p2/3P4/n1NQNB2/RP1P1P1P/6P1/6K1 b - - 0 26",
	"2rr2k1/pp2Bnq1/4R1p1/5p1p/B2P1P2/2P5/P5P1/4Q1K1 w - - 0 33",
	"8/8/8/3K1P2/2p4p/k2p3P/3Br3/1R6 w - - 0 67"
};

int t_nps;
int t_nodes[MAX_DEPTH];
int t_depth;

int num_tests = 10;

int benchmark()
{
    int i, j;
    int depth;
    int alpha = -INFINITY;
    int beta =   INFINITY;
    int value = 0;
    int lastvalue = 0;

    t_nps = 0;
    t_depth = 0;
    for (i = 0; i < MAX_DEPTH; i++)
	t_nodes[i] = 0;

    for (i = 0; i < num_tests; i++) {
	printf("position   %s\n", test_fen[i]);
	set_position(test_fen[i]);

	stop_time = gtime() + 10000;
	init_search();

	for (j = 1; j <= max_search_depth; j++) {
	    depth = j * PLY;
	    idepth = j;
	    value = root_search(depth, alpha, beta);

	    t_nodes[j] = nodes;

            if (stop_search) {
                if (root_moves < 2)
                    value = lastvalue;
                break;
            }
            if ((root_moves == 1) && (i == 2))
                break;

	    if (j > 5)
	        show_pv(value);
            lastvalue = value;
	}
	show_pv(value);
	show_search_stats();

	t_depth += j;
	t_nps += nodes / 10;
    }


    printf("Averages: \n");
    printf("  depth: %d.%02d \n", t_depth / num_tests, ((t_depth % num_tests) * (100 / num_tests)));
    printf("  nps: %d \n", t_nps / num_tests);


    return 0;
}


int lastnodes = 0;
int perft (int depth)
{
    Move *mp;
    int i;

    if (!depth)
        return 0;

    evaluate();

    if (in_check(board.side))
        generate_evasions();
    else
        generate_moves();

    for (mp = know_stack[ply].ms; mp < know_stack[ply+1].ms; mp++) {

        make_move(mp->move);

        if (depth >= 99) {
            for (i = 0; i < abs(5 - depth) * 3; i++)
                printf("    ");
            printf("%s \n", lan_move(mp->move));
            printf("   nodes %d   change: %d\n", nodes, nodes - lastnodes);
            lastnodes = nodes;
        }

        if (!in_check(board.side ^ 1)) {
            nodes++;
            perft(depth - 1);
        }
        unmake_move(mp->move);
    }

    return 0;
}
