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
#include "search.h"

#ifndef _WINDOWS_

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#endif

int readline(char *s, int size, FILE *fp)
{
    int c;
    int len = 0;

    while(1) {
        c = fgetc(fp);
        if (c == EOF)
            exit(1);
        if (c == '\n') {
            break;
        }
        if (len < size-1) { 
            s[len++] = c;
        }
    }
    s[len] = 0;
    return len;
}

#ifndef _WINDOWS_
int is_input()
{
    fd_set readfds;
    struct timeval  tv;

    tv.tv_sec = 0;
    tv.tv_usec = 0;

    FD_ZERO (&readfds);
    FD_SET (fileno (stdin), &readfds);
    select (16, &readfds, 0, 0, &tv);

    return (FD_ISSET (fileno (stdin), &readfds));
}
#else

#include <windows.h>
#define frame 0
int is_input() {
  
    static int init = 0, pipe;
    static HANDLE inh;
    DWORD dw;

    if (!init) {
    	init = 1;
    	inh = GetStdHandle (STD_INPUT_HANDLE);
    	pipe = !GetConsoleMode (inh, &dw);
    	if (!pipe) {
      	    SetConsoleMode(inh, dw & ~(ENABLE_MOUSE_INPUT|ENABLE_WINDOW_INPUT));
      	    FlushConsoleInputBuffer (inh);
    	}
    }
    if (pipe) {
	if (!PeekNamedPipe (inh, NULL, 0, NULL, &dw, NULL))
      	    return 1;
    	return dw;

    } else {
    	GetNumberOfConsoleInputEvents (inh, &dw);
    	return dw <= 1 ? 0 : dw;
    }
    return (0);
}
#endif

char piece_symbol[16] = 
    {0, 'P', 'N', 'B', 'R', 'Q', 'K', 0, 0, 'p', 'n', 'b', 'r', 'q', 'k', 0};

int show_board()
{
    int j,k;
    int p;

    printf("The Board ...\n");
    for (j = 7; j >= 0; j--) {
	printf("\n");
	for (k = 0; k < 8; k++) {
	    p = board.square[(j*16)+k];
	    if (p < 15)
	        printf("%3c ", piece_symbol[p]);
	    else
		printf("  . ");
	}
    }
    printf("\n");

    printf("Castle: %x\n", board.castle);
    printf("fifty: %x\n", board.fifty);
    printf("enpassant: %x\n", board.enpassant);
    printf("side: %d\n", board.side);
    printf("total ply: %d\n", board.gameply);
    printf("hash key: %08x%08x \n", (int) (board.hash >> 32), (int) board.hash);
    printf("\n\n");

    return 0;
}

int show_lists()
{
    int j,k;
    printf("Next List ...\n");
    for (j = 15; j >= 0; j--) {
	printf("\n");
	for (k = 0; k < 16; k++)
	    printf("%3x ", board.piece_list[(j*16)+k].n);
    }
    printf("\n");
    printf("Prev List ...\n");
    for (j = 15; j >= 0; j--) {
	printf("\n");
	for (k = 0; k < 16; k++)
	    printf("%3x ", board.piece_list[(j*16)+k].p);
    }
    printf("\n");
    return 0;
}

int show_update()
{
    int msecs;  // milliseconds
    int nps;    // nodes per seconds

    msecs = gtime() - start_time;
    if (msecs == 0)
	msecs = 1;

    nps = (nodes / msecs) * 1000;
    if (game_flags & UCI_FLAG) {
        printf("info nodes %d nps %d\n", nodes, nps);
    }

    return 0;
}

int show_pv(int score)
{
    int i;
    int ctime;
    int msecs;  // milliseconds
    int mate;

    ctime = gtime();
    msecs = ctime - start_time;

    if (game_flags & UCI_FLAG) {
	if (abs(score) > (INFINITY-200)) {
	    if (score > 0)
		mate = ((INFINITY - abs(score)) + 1)/ 2;
	    else
		mate = ((abs(score) - INFINITY) + 1)/ 2;
    	    printf("info depth %d score mate %d time %d nodes %d pv ", 
		idepth, mate, msecs, nodes);
	}
	else {
    	    printf("info depth %d score cp %d time %d nodes %d pv ", 
		idepth, score, msecs, nodes);
	}
    	for (i = 0; i < idepth; i++) {
	    if (pv[0][i] == 0) break;
            printf("%s ", lan_move(pv[0][i]));
        }
        printf("\n");
    }
    else {
	printf("%3d  %6d    %6d  %8d    ", idepth, score, msecs, nodes);
    	for (i = 0; (i < idepth) && (i < 10); i++) {
	    if (pv[0][i] == 0) break;
            printf("%s ", lan_move(pv[0][i]));
        }
        printf("\n");
    }

    return 0;
}

int show_search_stats()
{
    int time_used;

    if (!(game_flags & UCI_FLAG)) {
    	time_used = gtime() - start_time;

    	if (!time_used)
	    time_used = 1;

    	printf("Time: %d.%03d  Nodes: %d  Nps: %d \n",
		time_used/1000, time_used%1000, nodes, 
		(nodes / time_used) * 1000);
    	printf("check %d  prom %d  p7 %d  single %d  mt %d  kt %d simple %d \n",
	    search_stats.check , search_stats.prom, search_stats.pawn7,
	    search_stats.single, search_stats.mthreat, 
	    search_stats.kt, search_stats.simple);
	printf("max hist: %d  iid_cnt: %d\n", maxhist, search_stats.iid_cnt);

    }

    return 0;
}
