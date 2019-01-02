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
#include <assert.h>

#include "diablo.h"

typedef struct {
    int count;
    int square[16];
} AttackList;

AttackList atks;
AttackList defs;

int attack_list(int side, int from, int to, AttackList *al);
int swap_off(AttackList *atks, AttackList *defs, int s, int sv);


int see(int move)
{
   int f = FROM(move);
   int t = TO(move);
   int side = COLOR(Square(f));
   int value;

   attack_list(side^1, 128, t, &atks);
   attack_list(side, f, t, &defs);
   value = piece_value[Square(t)];
   value -= swap_off(&atks, &defs, f, piece_value[Square(f)]);

   return value;
}

int swap_off(AttackList *atks, AttackList *defs, int s, int sv)
{
    int value = 0;
    int ai; // attack index
    int di; // def index
    int av; // attack value
    int dv; // def value

    if (!atks->count) {
	return 0;
    }

    if (!defs->count) {
	return sv;
    }

    ai = atks->count - 1;
    di = defs->count - 1;
    av = piece_value[Square(atks->square[ai])];
    dv = piece_value[Square(defs->square[di])];

    if (((ai > di) && (av == dv)) || (av < dv)) {
	atks->count--;
	value = sv;
	value -= swap_off(defs, atks, s, av);
    }

    // printf("returning value %d\n", value);
    return value;
}

int attack_list(int side, int from, int to, AttackList *al)
{

    int *mvec;
    int s = to;
    int f = 0;

    al->count = 0;

    for (mvec = king_move_vec; *mvec; mvec++) {
        f = s + *mvec;
        if (board.square[f] == (KING | (side << 3)))   {
	    if (f != from) {
	        al->square[al->count] = f;
	        al->count++;
	    }
	}
    }
    for (mvec = queen_move_vec; *mvec; mvec++) {
        for (f = s + *mvec; board.square[f] == EMPTY; f += *mvec);
        if (Square(f) == (QUEEN | (side << 3)))  {
	    if (f != from) {
	        al->square[al->count] = f;
	        al->count++;
	    }
	}
    }
    for (mvec = rook_move_vec; *mvec; mvec++) {
        for (f = s + *mvec; board.square[f] == EMPTY; f += *mvec);
        if (Square(f) == (ROOK | (side << 3)))  {
	    if (f != from) {
	        al->square[al->count] = f;
	        al->count++;
	    }
	}
    }
    for (mvec = bishop_move_vec; *mvec; mvec++) {
        for (f = s + *mvec; board.square[f] == EMPTY; f += *mvec);
        if (Square(f) == (BISHOP | (side << 3)))  {
	    if (f != from) {
	        al->square[al->count] = f;
	        al->count++;
	    }
	}
    }

    for (mvec = knight_move_vec; *mvec; mvec++) {
        f = s + *mvec;
        if (board.square[f] == (KNIGHT | (side << 3)))   {
	    if (f != from) {
	        al->square[al->count] = f;
	        al->count++;
	    }
	}
	   
    }

    if (side == BLACK) {
	if (board.square[s+15] == BPAWN) {
	    if (from != s+15) {
	        al->square[al->count] = s+15;
	        al->count++;
	    }
	}
	if (board.square[s+17] == BPAWN) {
	    if (from != s+17) {
	        al->square[al->count] = s+17;
	        al->count++;
	    }
	}
    }
    else {
	if (board.square[s-15] == WPAWN) {
	    if (from != s-15) {
	        al->square[al->count] = s-15;
	        al->count++;
	    }
	}
	if (board.square[s-17] == WPAWN) {
	    if (from != s-17) {
	        al->square[al->count] = s-17;
	        al->count++;
	    }
	}
    }

    return 0;
}

// These 2 tables were created by Tord Romstad as posted in the winboard forum.
// I never bothered to create my own since they would have looked nearly identical 
// anyways.  They are based on attack tables as presented by Ed Schroder on his chess
// programmers page.
unsigned char smallest_attacker[256] = {
  10, 10, 10, 10, 10, 10, 10, 10, 1, 1, 1, 1, 1, 1, 1, 1,
  3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1, 1, 1,
  5, 5, 5, 5, 5, 5, 5, 5, 1, 1, 1, 1, 1, 1, 1, 1,
  3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1, 1, 1,
  10, 10, 10, 10, 10, 10, 10, 10, 1, 1, 1, 1, 1, 1, 1, 1,
  3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1, 1, 1,
  5, 5, 5, 5, 5, 5, 5, 5, 1, 1, 1, 1, 1, 1, 1, 1,
  3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1, 1, 1,
  100, 100, 100, 100, 100, 100, 100, 100, 1, 1, 1, 1, 1, 1, 1, 1,
  3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1, 1, 1,
  5, 5, 5, 5, 5, 5, 5, 5, 1, 1, 1, 1, 1, 1, 1, 1,
  3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1, 1, 1,
  10, 10, 10, 10, 10, 10, 10, 10, 1, 1, 1, 1, 1, 1, 1, 1,
  3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1, 1, 1,
  5, 5, 5, 5, 5, 5, 5, 5, 1, 1, 1, 1, 1, 1, 1, 1,
  3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1, 1, 1 };

unsigned char smallest_removed[256] = {
  0, 0, 1, 2, 3, 4, 5, 6, 0, 0, 9, 10, 11, 12, 13, 14, 0, 0, 17, 18, 19, 20,
  21, 22, 15, 16, 17, 26, 27, 28, 29, 30, 0, 0, 33, 34, 35, 36, 37, 38, 31, 32,
  33, 42, 43, 44, 45, 46, 31, 32, 33, 50, 51, 52, 53, 54, 47, 48, 49, 50, 59,
  60, 61, 62, 0, 0, 65, 66, 67, 68, 69, 70, 63, 64, 65, 74, 75, 76, 77, 78, 63,
  64, 65, 82, 83, 84, 85, 86, 79, 80, 81, 82, 91, 92, 93, 94, 63, 64, 65, 98,
  99, 100, 101, 102, 95, 96, 97, 98, 107, 108, 109, 110, 95, 96, 97, 98, 115,
  116, 117, 118, 111, 112, 113, 114, 115, 124, 125, 126, 0, 0, 129, 130, 131,
  132, 133, 134, 127, 128, 129, 138, 139, 140, 141, 142, 127, 128, 129, 146,
  147, 148, 149, 150, 143, 144, 145, 146, 155, 156, 157, 158, 127, 128, 129,
  162, 163, 164, 165, 166, 159, 160, 161, 162, 171, 172, 173, 174, 159, 160,
  161, 162, 179, 180, 181, 182, 175, 176, 177, 178, 179, 188, 189, 190, 127,
  128, 129, 194, 195, 196, 197, 198, 191, 192, 193, 194, 203, 204, 205, 206,
  191, 192, 193, 194, 211, 212, 213, 214, 207, 208, 209, 210, 211, 220, 221,
  222, 191, 192, 193, 194, 227, 228, 229, 230, 223, 224, 225, 226, 227, 236,
  237, 238, 223, 224, 225, 226, 227, 244, 245, 246, 239, 240, 241, 242, 243,
  244, 253 };

int see_value[18] = {0, 1, 3, 3, 5, 10, 100, 0, 0, 1, 3, 3, 5, 10, 100, 0, 0, 0};
int piece_attack[8] = 
    {0, PAWN_ATK, KNIGHT_ATK, BISHOP_ATK, ROOK_ATK, QUEEN_ATK, KING_ATK, 0 };

int quicksee(int move)
{
   int f = FROM(move);
   int t = TO(move);
   int side = COLOR(Square(f));
   int value;
   int atks;
   int defs;

   atks = attack_tab[side][t] & 0xff;
   defs = attack_tab[side^1][t] & 0xff;
   value = see_value[Square(t)];
   assert(atks != 0);

   if (defs) {
	atks--;
        atks &=  ~piece_attack[Square(f) & 7];
        value -= SWAP((Square(f) & 7), defs, atks);
   }
   return value;
}

int quickswap(int atks, int defs, int v)
{
    int value = 0;
    int av;

    if ((atks & 7) == 0)
	return 0;

    if ((defs & 7) == 0)
	return v;

    av = smallest_attacker[atks];
    atks = smallest_removed[atks];

    value = v;
    value += -quickswap(defs, atks, av);

    if (value > 0)
	return value;
    else
	return 0;
}


int init_quicksee()
{
    int i, j;
    int atks;
    int defs;
    int value = 0;

    for (j = PAWN; j <= KING; j++) {
        for (i = 0; i <= 0xffff; i++) {
            atks = (i >> 8) & 0xff;
            defs = i & 0xff;
            value = quickswap(atks, defs, see_value[j]);
            quicksee_tab[j-1][i] = value;;
        }
    }

    return 0;
}
