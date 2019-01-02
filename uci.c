/*
    Diablo chess engine, Copyright 2005, Marcus Prewarski
          
    This program is free software. It may be copied or modified under the terms
    of the GNU General Public License version 2 as published by the Free Software 
    Foundation.  This program is distributed without any warranties whatsoever.
    See the file COPYING included with the distribution for details of the GNU general 
    public license.
*/
#ifndef WIN32
#include <unistd.h>
#include <sys/time.h>
#else
#include <windows.h>
extern int strcasecmp(const char *p1,const char *p2);
extern int strncasecmp(const char *p1,const char *p2,unsigned int n);
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>

#include "diablo.h"
#include "hash.h"
#include "util.h"
#include "search.h"
#include "attack.h"

static void uci_setoption(char *);
static void uci_position(char *);
static void uci_go(char *);
static void non_uci(char *, char *);


int uci_command(char *line)
{
    char cmd[128];

    sscanf (line, "%s %[^\n]", cmd, line);
    if (!strcasecmp(cmd , "uci")) {
	game_flags |= UCI_FLAG;
	printf("id name %s %s \n", NAME, VERSION);
	printf("id author Marcus Prewarski\n");
	printf("option name Hash type spin default 8 min 4 max 128\n");
	// printf("option name OwnBook type check default true\n");
	printf("option name Ponder type check default true\n");
	printf("uciok\n");
    }
    else if (!strcasecmp(cmd , "isready")) {
	printf("readyok\n");
    }
    else if  (!strcasecmp(cmd , "debug")) {
    }
    else if  (!strcasecmp(cmd , "setoption")) {
	sscanf (line, "%s %[^\n]", cmd, line);
	if (!strcasecmp(cmd , "name"))
	    uci_setoption(line);
    }
    else if  (!strcasecmp(cmd , "position")) {
	uci_position(line);
    }
    else if  (!strcasecmp(cmd , "ucinewgame")) {
	// uci_newgame(line);
    }
    else if  (!strcasecmp(cmd , "go")) {
	uci_go(line);
    }
    else if  (!strcasecmp(cmd , "stop")) {
    }
    else if  (!strcasecmp(cmd , "ponderhit")) {
    }
    else if  (!strcasecmp(cmd , "quit")) {
	printf("Bye\n");
	exit(0);
    }
    else {
	non_uci(cmd, line);
    }

    return 0;
}

// non uci commands, used mainly for debugging
static void non_uci(char *cmd, char *line)
{
    // non uci commands
    if  (!strcasecmp(cmd , "board")) {
	show_board();
    }
    else if  (!strcasecmp(cmd , "lists")) {
	show_lists();
    }
    else if  (!strcasecmp(cmd , "eval")) {
	init_search();
	evaluate();
    }
    else if  (!strcasecmp(cmd , "hint")) {
	start_time = gtime();
	stop_time = start_time + 5000;
	search_flags = 0;
	think();
    }
    else if  (!strcasecmp(cmd , "think")) {
	int time = atoi(line);
	start_time = gtime();
	stop_time = start_time + (time * 1000);
	search_flags = 0;
	think();
    }
    else if  (!strcasecmp(cmd , "bench")) {
	benchmark();
    }
    else if  (!strcasecmp(cmd , "perft")) {
	int depth;
	unsigned int start_t;
	unsigned int used_t;

	sscanf(line, "%d", &depth);
	printf("perft for depth %d\n", depth);
	start_t = gtime();
	ply = 0;
	know_stack[ply].check = 0;
	nodes = 0;
	perft(depth);
	used_t = gtime() - start_t;
	if (used_t == 0) used_t = 1;
	printf("Nodes: %d   time: %d.%d   Nodes/sec: %d\n", 
			nodes , used_t / 1000, used_t % 1000, (nodes / used_t) * 1000);
    }
    else if  (!strcasecmp(cmd , "attacks")) {
	int i, j;

	evaluate(); // must call evaluate to generate attack tables
	printf("white attacks\n");
	for (i = 7; i >= 0; i--) {
	    for (j = 0; j < 8; j++) {
		printf("%02x ", attack_tab[0][(i*16)+j]);
	    }
	    printf("\n");
	}
	printf("\n");
	printf("black attacks\n");
	for (i = 7; i >= 0; i--) {
	    for (j = 0; j < 8; j++) {
		printf("%02x ", attack_tab[1][(i*16)+j]);
	    }
	    printf("\n");
	}
    }
    else if  (!strcasecmp(cmd , "hung")) {
	evaluate();
	printf("hung value: %d\n", hung_value(board.side));
    }
    else if  (!strcasecmp(cmd , "moves")) {
	Move *mp;

	know_stack[ply].ms = move_stack;
	generate_moves();
	for (mp = know_stack[ply].ms; mp < know_stack[ply+1].ms; mp++) {
             printf("%s ", lan_move(mp->move));
	    if (isa_check(mp->move)) {
	        printf("+");
	    }
	    printf("\n");
	}
    }
    else if  (!strcasecmp(cmd , "evasions")) {
	Move *mp;

	ply=0;
	printf("generating evasions ... \n");
	generate_evasions();
	for (mp = know_stack[ply].ms; mp < know_stack[ply+1].ms; mp++) {
            printf("%s \n", lan_move(mp->move));
	}

    }
    else if  (!strcasecmp(cmd , "captures")) {
	Move *mp;
	int  v;

	know_stack[ply].ms = move_stack;
	printf("generating captures ... \n");
	evaluate();
	generate_captures();
	for (mp = know_stack[ply].ms; mp < know_stack[ply+1].ms; mp++) {
	    v = quicksee(mp->move);
	    printf(" %s (%d)\n", lan_move(mp->move), v);
	}
    }
    else if  (!strcasecmp(cmd , "move")) {
	int move;

	know_stack[ply].ms = move_stack;
	if ((move = parse_move(line)))
	    make_move(move);
	else
	    printf("illegal move: %s\n", line);
    }
    else if  (!strcasecmp(cmd , "setboard")) {
	set_position(line);
    }
    else if  (!strcasecmp(cmd , "undo")) {
	int themove = move_hist[board.gameply].move;
	printf("unmaking move: 0x%x\n", themove);
	unmake_move(themove);
    }
    else if  (!strcasecmp(cmd , "test")) {
#if 0
        int i, j;

        extern short maxdist[];

        for (i = 0; i < 128; i++) {
            for (j = 0; j < 128; j++) {

                if (i & 0x88) continue;
                if (j & 0x88) continue;

                if (distance[i][j].max != maxdist[abs(i-j)])
                    printf(" error on square %d %d %s,  dist:%d  maxdist:%d\n",
                           i, j, lan_move((i << 8) | j), distance[i][j].max, maxdist[abs(i-j)] );
            }
        }
#endif
    }
    else {
	int move;

	if ((move = parse_move(line)))
	    make_move(move);
	else
	    printf("illegal command: %s\n", line);
    }
}

int stop_think()
{
    int status = 0;

    readline(inputline, sizeof(inputline), stdin);

    if (!strncasecmp(inputline, "stop", 4)) {
        search_flags &= ~FLAG_ANALYZE;
        search_flags &= ~FLAG_PONDERING;
        status = 1;
    }
    else if  (!strncasecmp(inputline, "ponderhit", 9)){
        search_flags &= ~FLAG_PONDERING;
        status = 0;
    }
    else if  (!strncasecmp(inputline, "quit", 4)){
        printf("Quitting during think, bye\n");
        exit(0);
    }
    else {
        printf("Unexpected command: %s\n", inputline);
    }

    return status;
}

static void uci_go(char *line)
{
    char *s;
    int  time;

    search_flags = 0;
    moves_to_go = 0;

    if (strstr(line, "ponder")) {
	search_flags |= FLAG_PONDERING;
    }
    if (strstr(line, "infinite")) {
	search_flags |= FLAG_ANALYZE;
    }
    if ((s = strstr(line, "wtime"))) {
	time = atoi(s+6);
	if ((board.side == WHITE) )
	    time_left = time;
	else
	    otime_left = time;
    }
    if ((s = strstr(line, "btime"))) {
	time = atoi(s+6);
	if ((board.side == BLACK) )
	    time_left = time;
	else
	    otime_left = time;
    }
    if ((s = strstr(line, "movestogo"))) {
	moves_to_go = atoi(s+10);
	printf("got %d from line %s\n", moves_to_go, (s+10));
    }
    if ((s = strstr(line, "depth"))) {
	max_search_depth = atoi(s+6);
    }

    if (strstr(line, "winc")) { }
    if (strstr(line, "binc")) { }
    if (strstr(line, "depth")) { }
    if (strstr(line, "nodes")) { }
    if (strstr(line, "mate")) { }
    if (strstr(line, "movetime")) { }

    alloc_time();
    think();

    while (search_flags & (FLAG_ANALYZE | FLAG_PONDERING)) {
	if (is_input()) {
	    stop_think();
	}
    }

    printf("bestmove %s ", lan_move(pv[0][0]));

    if (pv[0][1]) {
        printf("ponder %s\n", lan_move(pv[0][1]));
    }
    else
        printf("\n");
}

static void uci_setoption(char *s)
{
    int hash_size;

    while(isspace(*s)) s++;

    if (!strncasecmp(s, "hash", 4)) {
	s += 5;
	if (!strncasecmp(s, "value", 5)){
	    s += 6;
	    hash_size = atoi(s);
	    if (hash_size == 1) {  // hack for Fritz interface bug
		hash_size = 16;
	    }
	    if (hash_size < 4)
		hash_size = 4;
	    else if (hash_size > MAX_HASH_SIZE)
		hash_size = MAX_HASH_SIZE;

	    printf("setting hash size to %d\n", hash_size);
	    set_hash_size(hash_size);
	}
    }
    else {
	printf("unsupported option: %s\n", s);
    }
}


static void uci_position(char *s)
{
    int move;

    if (!strncasecmp(s, "startpos", 8)) {
	set_position(START_POSITION); s += 8;
    }
    else if (!strncasecmp(s, "fen", 3)){
	set_position(s+4); s += 4;

	while (*s != 'm') s++;
    }

    while(isspace(*s)) s++;

    if (!strncasecmp(s, "moves", 5)) {
	s+= 5;
    	while(isspace(*s)) s++;

	while (*s) {
            move = parse_move(s);
            if (move) {
		ply = 0;
                make_move(move);
	    }
            else {
		printf("Error, Invalid move: %s \n", s);
		break;
	    }
	    while(isalnum(*s)) s++;
	    while(isspace(*s)) s++;
	}
    }
    else {
	// printf("uci position error: %s \n", s);
    }
}
