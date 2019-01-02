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
#include <assert.h>

#include "diablo.h"
#include "attack.h"

//
// Is the side in check
//
int in_check(int side)
{
    return color_attacks_square(side ^ 1, KSQ(side));
}

//
// Is the move a check.
//     This function isn't completely accurate. It fails in at least one case, where a castling
//     move gives a check but this is such a rare case I'm not worrying about it for now.
//
//     Note - this function was could use cleaning up to be made faster.
//
int isa_check(int move)
{
    int ks = KSQ(board.xside);
    int t = TO(move);
    int f = FROM(move);
    int p;
    int vec;
    int i;

    if (move & PROMOTION) {
	p = promoted[(move >> 16) & 0xf];
    }
    else {
	p = Square(f);
    }
    // check from the moved piece
    if (attack_opportunity[ks - t].type & threattype[p]) {
	// printf("move %x is the right type %x \n", move, threattab[ks][t].type);
	switch(p & 7) {
	    case PAWN:
	    case KNIGHT:
	    case KING:
		return 1;

	    case BISHOP:
	    case ROOK:
	    case QUEEN:
		vec = (int)attack_opportunity[t - ks].vector; // reverse the vector
		// printf("vec %d  ks %x  t %x\n", vec, ks, t);
		for (i = t + vec; (Square(i) == EMPTY) || (i == f);  i += vec) {
		}
		if (Square(i) == (KING | (board.xside << 3)))
		    return 1;
		break;
	}
    }

    // discovered check
    if (attack_opportunity[ks - f].vector == attack_opportunity[ks - t].vector)
        return 0;

    if (attack_opportunity[ks - f].type & BISHOP_THREAT) {
	vec = attack_opportunity[ks - f].vector;
	// printf("vec %d\n", vec);
	for (i = ks + vec; (Square(i) == EMPTY) || (i == f);  i += vec) {
	}
	if ((Square(i) == (BISHOP | (board.side << 3))) || 
	     (Square(i) == (QUEEN | (board.side << 3))))
	    return 1;
    }
    if (attack_opportunity[ks - f].type & ROOK_THREAT) {
	vec = attack_opportunity[ks - f].vector;
	//  printf("vec %d\n", vec);
	for (i = ks + vec; (Square(i) == EMPTY) || (i == f) ;  i += vec) {
	}
	if ((Square(i) == (ROOK | (board.side << 3))) || 
	     (Square(i) == (QUEEN | (board.side << 3))))
	    return 1;
    }

    return 0;
}


//
// Does the side(color) attack a square?
//     for most pieces, use the attack_opportunity table to see if the piece may attack
//     the square. For sliding pieces, use the vectore to step from square to the piece.
//      
int color_attacks_square(int color, int sq)
{
    int from;
    int vector;
    int i;

    for (from = PL_FIRST(KNIGHT | (color << 3)); from < 128; from = PL_NEXT(from)) {
        if (attack_opportunity[from - sq].type == KNIGHT_THREAT)
            return 1;
    }
    for (from = PL_FIRST(BISHOP | (color << 3)); from < 128; from = PL_NEXT(from)) {
        if (attack_opportunity[sq - from].type & BISHOP_THREAT) {
	    vector = attack_opportunity[sq - from].vector;
	    for (i = sq + vector; (Square(i) == EMPTY);  i += vector);
            if (Square(i) == (BISHOP | (color<<3)))
                return 1;
        }
    }
    for (from = PL_FIRST(ROOK | (color << 3)); from < 128; from = PL_NEXT(from)) {
        if (attack_opportunity[sq - from].type & ROOK_THREAT) {
	    vector = attack_opportunity[sq - from].vector;
	    for (i = sq + vector; (Square(i) == EMPTY);  i += vector);
            if (Square(i) == (ROOK | (color<<3)))
                return 1;
        }
    }
    for (from = PL_FIRST(QUEEN | (color << 3)); from < 128; from = PL_NEXT(from)) {
        if (attack_opportunity[sq - from].type & QUEEN_THREAT) {
	    vector = attack_opportunity[sq - from].vector;
	    for (i = sq + vector; (Square(i) == EMPTY);  i += vector);
            if (Square(i) == (QUEEN | (color<<3)))
                return 1;
        }
    }
    if (color == BLACK) {
	if (Square(sq+15) == BPAWN) return(1);
	if (Square(sq+17) == BPAWN) return(1);
    }
    else {
	if (Square(sq-15) == WPAWN) return(1);
	if (Square(sq-17) == WPAWN) return(1);
    }

    from = KSQ(color);
    if (distance[abs(from - sq)] <= 1)
	return 1;

    return 0;
}

unsigned char check_tab[128];

//
// Generates a check table used by the generate_evasions function. And also counts the number of
// checkers. 
//     If there is only one checker then any non king move to a non zero position in 
//     the check table is an evasion whether it is a capture or moving a piece in front 
//     of the check. If there is more than one checker then the king must be moved to
//     a non check square.
//
//     Note: looking at this code now it seems extremely inefficient. Did I write this before
//           had settled on attack tables?  Seems like could use the attack table info better
//           and use the attack_opportunity table to better fill out the check_tab.
//
int find_checks()
{
    int *mvec;
    int s, f, i;
    int side = board.side;
    int xside = board.xside;
    int checkers = 0;

    memset(check_tab, 0, sizeof(check_tab));
    s = KSQ(side);

    // knights
    for (mvec = knight_move_vec; *mvec; mvec++) {
        f = s + *mvec;
        if (Square(f) == (KNIGHT | (xside << 3)))  {
	    check_tab[f]++;
	    checkers++;
	}
    }
    // rooks or queen
    for (mvec = rook_move_vec; *mvec; mvec++) {
        for (f = s + *mvec; Square(f) == EMPTY; f += *mvec);
        if ((Square(f) == (ROOK | (xside << 3))) || 
	    (Square(f) == (QUEEN | (xside << 3))) ) {
	    checkers++;
	    for (i = s + *mvec; i != f; i += *mvec) {
                check_tab[i]++;
            }
            check_tab[i]++;
	}
    }
    // bishops or queens
    for (mvec = bishop_move_vec; *mvec; mvec++) {
        for (f = s + *mvec; Square(f) == EMPTY; f += *mvec);
        if ((Square(f) == (BISHOP | (xside << 3))) || 
	    (Square(f) == (QUEEN | (xside << 3))) ) {
	    checkers++;
	    for (i = s + *mvec; i != f; i += *mvec) {
                check_tab[i]++;
            }
            check_tab[i]++;
	}
    }
    // pawn
    if (side == WHITE) {
	if (Square(s+15) == BPAWN) {
	    checkers++;
            check_tab[s+15]++;
	}
	if (Square(s+17) == BPAWN) {
	    checkers++;
            check_tab[s+17]++;
	}
    }
    else {
	if (Square(s-15) == WPAWN) {
	    checkers++;
            check_tab[s-15]++;
	}
	if (Square(s-17) == WPAWN) {
	    checkers++;
            check_tab[s-17]++;
	}
    }

    return checkers;
}

//
// 
//
//
int hung_value(int side)
{
    int piece;
    int value = 0;
    int xside = side ^ 1;
    int s;

    piece = QUEEN | (side << 3);
    for (s = PL_FIRST(piece); s < 128; s = PL_NEXT(s)) {
	value += SWAP(QUEEN, attack_tab[xside][s], attack_tab[side][s]);
	// printf("Queen on square %d  value %d \n", s, value);
    }

    piece = ROOK | (side << 3);
    for (s = PL_FIRST(piece); s < 128; s = PL_NEXT(s)) {
	value += SWAP(ROOK, attack_tab[xside][s], attack_tab[side][s]);
	// printf("Rook on square %d  value %d \n", s, value);
    }

    piece = BISHOP | (side << 3);
    for (s = PL_FIRST(piece); s < 128; s = PL_NEXT(s)) {
	value += SWAP(BISHOP, attack_tab[xside][s], attack_tab[side][s]);
	// printf("Bishop on square %d  value %d \n", s, value);
    }
    
    piece = KNIGHT | (side << 3);
    for (s = PL_FIRST(piece); s < 128; s = PL_NEXT(s)) {
	value += SWAP(BISHOP, attack_tab[xside][s], attack_tab[side][s]);
	// printf("Knight on square %d  value %d \n", s, value);
    }

    piece = PAWN | (side << 3);
    for (s = PL_FIRST(piece); s < 128; s = PL_NEXT(s)) {
	value += SWAP(PAWN, attack_tab[xside][s], attack_tab[side][s]);
	// printf("Pawn on square %d  value %d \n", s, value);
    }

    return (value*100);
}

