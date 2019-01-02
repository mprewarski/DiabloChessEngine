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
#include "util.h"

int set_position(char *s)
{
    int  rank = 7;
    int  file = 0;
    int  piece = 0;
    int  color = 0;
    int  count = 0;
    int  i, sq;

    init_board();
    while ((rank >= 0)  &&  *s) {
	count = 1;
        switch (*s) {
            case 'p': piece = BPAWN;   color = BLACK; break;
            case 'r': piece = BROOK;   color = BLACK; break;
            case 'n': piece = BKNIGHT; color = BLACK; break;
            case 'b': piece = BBISHOP; color = BLACK; break;
            case 'k': piece = BKING;   color = BLACK; break;
            case 'q': piece = BQUEEN;  color = BLACK; break;
            case 'P': piece = WPAWN;   color = WHITE; break;
            case 'R': piece = WROOK;   color = WHITE; break;
            case 'N': piece = WKNIGHT; color = WHITE; break;
            case 'B': piece = WBISHOP; color = WHITE; break;
            case 'K': piece = WKING;   color = WHITE; break;
            case 'Q': piece = WQUEEN;  color = WHITE; break;

            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
                piece = EMPTY;
                count = *s - '0';
                break;

            case '/':
            case ' ':
                rank--;
                file = 0;
		s++;
                continue;
                break;

            default:
                printf("FEN error \n");
                return -1;
        }
        for (i = 0; i < count; i++, file++) {
            sq = (rank * 16) + file;
            board.square[sq] = piece;
	    if (piece != EMPTY) {
	    	PL_INSERT(piece, sq);
	    }
        }
	s++;
    }

    board.side = (s[0] == 'b');
    board.xside = board.side ^ 1;
    s += 2;

    board.castle = 0;
    for (i = 0; i < 4; i++) {
        if (*s == ' ') {
            break;
        }
        switch(*s) {
	    case 'K':
                board.castle |= WK_CASTLE; break;
	    case 'Q':
                board.castle |= WQ_CASTLE; break;
	    case 'k':
                board.castle |= BK_CASTLE; break;
	    case 'q':
                board.castle |= BQ_CASTLE; break;
	    case '-':
                board.castle = 0; break;
        }
	s++;
    }
    s++;
    if (s[0] == '-') {
	board.enpassant = 0xc0;
	s += 1;
    }
    else {
	file = s[0] - 'a';
	rank = s[1] - '1';
	board.enpassant = (rank * 16) + file;
	s += 2;
    }
    gen_hashkey();
    return 0;
}


int parse_move(char *s)
{
    int move;
    Move *mp;

    if ((s[0] >= 'a') && (s[0] <= 'h') && (s[1] >= '1') && (s[1] <= '8')) {
	move = (s[0] - 'a') + ((s[1] - '1') * 16);
	move = move << 8;
    }
    else
	return 0;

    if ((s[2] >= 'a') && (s[2] <= 'h') && (s[3] >= '1') && (s[3] <= '8')) {
	move |= (s[2] - 'a') + ((s[3] - '1') * 16);
    }
    else
	return 0;

    switch (s[4]) {
	case 'q':
	case 'Q':
	    move |= QUEEN_PROM; break;
	case 'r':
	case 'R':
	    move |= ROOK_PROM; break;
	case 'b':
	case 'B':
	    move |= BISHOP_PROM; break;
	case 'n':
	case 'N':
	    move |= KNIGHT_PROM; break;
    }

    generate_moves();
    for (mp = know_stack[ply].ms; mp < know_stack[ply+1].ms; mp++) {
	if (move == (mp->move & (PROMOTION | 0xffff )))
	    return mp->move;
    }

    return 0;
}

char * lan_move(int move)
{
    char *ms;
    int f = FROM(move);
    int t = TO(move);

    ms = move_str;
    *ms++ = COLUMN(f) + 'a';
    *ms++ = RANK(f) + '1';
    *ms++ = COLUMN(t) + 'a';
    *ms++ = RANK(t) + '1';

    if (move & QUEEN_PROM)
        *ms++ = 'q';
    else if (move & ROOK_PROM)
        *ms++ = 'r';
    else if (move & BISHOP_PROM)
        *ms++ = 'b';
    else if (move & KNIGHT_PROM)
        *ms++ = 'n';

    *ms++ = 0;
    return (move_str);
}

int gen_hashkey()
{
    HashKey hk = 0;
    int s;
    int side;


    for (side = WHITE; side <= BLACK; side++) {
        for (s = PL_FIRST(PAWN | (side << 3)); s < 128; s = PL_NEXT(s)) {
	    hk ^= zobrist_board[(PAWN | (side << 3))][s];
        }

        for (s = PL_FIRST(KNIGHT | (side << 3)); s < 128; s = PL_NEXT(s)) {
	    hk ^= zobrist_board[(KNIGHT | (side << 3))][s];
        }

        for (s = PL_FIRST(BISHOP | (side << 3)); s < 128; s = PL_NEXT(s)) {
	    hk ^= zobrist_board[(BISHOP | (side << 3))][s];
        }

        for (s = PL_FIRST(ROOK | (side << 3)); s < 128; s = PL_NEXT(s)) {
	    hk ^= zobrist_board[(ROOK | (side << 3))][s];
        }

        for (s = PL_FIRST(QUEEN | (side << 3)); s < 128; s = PL_NEXT(s)) {
	    hk ^= zobrist_board[(QUEEN | (side << 3))][s];
        }

        for (s = PL_FIRST(KING | (side << 3)); s < 128; s = PL_NEXT(s)) {
	    hk ^= zobrist_board[(KING | (side << 3))][s];
        }
    }

    hk ^= zobrist_castle[board.castle];

    if (board.side == BLACK)
        hk ^= zobrist_side[0];

    board.hash = hk;

    return 0;
}

int repetition()
{
    int  hi;
    int  i;

    hi = board.gameply;
    for (i = hi - board.fifty; i < hi; i++) {
        if (board.hash == move_hist[i].hash_key) {
	    return 1;
        }
    }
    return 0;
}
