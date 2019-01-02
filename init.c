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
#include "util.h"

int init_vars()
{

    ply = 0;
    game_flags = 0;
    know_stack[0].ms = move_stack;
    know_stack[1].ms = move_stack;
    max_search_depth = 50;

    return 0;
}

int init_board()
{
    int i;

    for (i = 0; i < 256; i++) {
        board.all_squares[i] = OFFBOARD;
    }
    board.square = &board.all_squares[64];
    board.plist = &board.piece_list[64];
    for (i = 0; i < 256; i++) {
        board.piece_list[i].n = 0;
        board.piece_list[i].p = 0;
    }
    for (i = 0; i < 128; i++) {
	if (!(i & 0x88)) {
            board.square[i] = EMPTY;
	}
    }
    for (i = 0; i < 16; i++) {
        board.plist[0xb0 + i].n = 0x80;
    }

    board.castle = 0;
    board.side = 0;
    board.xside = 1;
    board.enpassant = 0xc0;
    board.fifty = 0;
    board.gameply = 0;
    board.hash = 0;

    return 0;
}

int init_hash()
{
    int size;

    size = DEF_HASH_SIZE * 1048576;
    if ((tt = malloc(size)) == 0) {
        printf("ERROR: Couldn't allocate memory for hash table\n");
        exit(1);
    }
    hash_entries = size / sizeof(HashEntry);
    clear_hash();

    return 0;
}

int min_dist(int i, int j)
{
    int d1 = abs(RANK(i) - RANK(j));
    int d2 = abs(COLUMN(i) - COLUMN(j));

    if (d1 < d2)
	return d1;
    else
	return d2;
}

int max_dist(int i, int j)
{
    int d1 = abs(RANK(i) - RANK(j));
    int d2 = abs(COLUMN(i) - COLUMN(j));

    if (d1 > d2)
	return d1;
    else
	return d2;
}

int diag_dist(int i, int j)
{
    int d1 = abs(RANK(i) - RANK(j));
    int d2 = abs(COLUMN(i) - COLUMN(j));

    return abs(d1-d2);
}

int init_distance()
{
    int i, j;

    for (i = 0; i < 128; i = (i + 9) & ~8) {
        for (j = 0; j < 128; j = (j + 9) & ~8) {
            distance[abs(i-j)] = max_dist(i, j);
        }
    }

    return 0;
}

int init_attack_opportunity()
{
    int i, j;
    int *move_vec;

    memset(attack_opportunity_, 0, sizeof(attack_opportunity_));
    attack_opportunity = &attack_opportunity_[128];

    for (move_vec = knight_move_vec; *move_vec; move_vec++) {
	attack_opportunity[*move_vec].type |= KNIGHT_THREAT;
    }
    for (move_vec = king_move_vec; *move_vec; move_vec++) {
	attack_opportunity[*move_vec].type |= KING_THREAT;
    }

    // remember this is FromSquare - ToSquare
    attack_opportunity[15].type |= WPAWN_THREAT;
    attack_opportunity[17].type |= WPAWN_THREAT;
    attack_opportunity[-15].type |= BPAWN_THREAT;
    attack_opportunity[-17].type |= BPAWN_THREAT;

    for (i = A1; i <= H8; i++) {
    for (move_vec = bishop_move_vec; *move_vec; move_vec++) {
        for (j = *move_vec + i; !(j & 0x88); j += *move_vec  ) {
            attack_opportunity[i-j].type |= (BISHOP_THREAT | QUEEN_THREAT);
            attack_opportunity[i-j].vector = *move_vec; 
        }
    }

    for (move_vec = rook_move_vec; *move_vec; move_vec++) {
        for (j = *move_vec + i; !(j & 0x88); j += *move_vec) {
            attack_opportunity[i-j].type |= (ROOK_THREAT | QUEEN_THREAT);
            attack_opportunity[i-j].vector = *move_vec; 
        }
    }
    }

    return 0;
}

int init_diablo()
{
    init_vars();
    init_board();
    init_hash();
    init_distance();
    init_attack_opportunity();
    init_quicksee();
    set_position(START_POSITION);
    return 0;
}
