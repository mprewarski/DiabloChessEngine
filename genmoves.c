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
#include "data.h"
#include "attack.h"

#define Addmove(a,b,c)  do {\
		ms->move = (((a) << 8) | (b) | (c));	\
		ms++;  \
	} while(0);

#define Addpromote(a,b,c) 	do { \
		Addmove (a, b, QUEEN_PROM | PAWNMOVE | c);     \
		Addmove (a, b, KNIGHT_PROM | PAWNMOVE | c);    \
		Addmove (a, b, ROOK_PROM | PAWNMOVE | c);      \
		Addmove (a, b, BISHOP_PROM | PAWNMOVE | c);	\
	} while (0);

int bishop_move_vec[] = { 15, 17, -15, -17, 0 };
int rook_move_vec[] = { -1, 1, 16, -16, 0 };
int queen_move_vec[] = { -1, 1, 16, -16, 15, 17, -15, -17, 0 };
int king_move_vec[] = { -1, 1, 16, -16, 15, 17, -15, -17, 0 };
int knight_move_vec[] = { 14, 31, 33, 18, -14, -31, -33, -18, 0 };
int whitepawn_move_vec[] = { 15, 17, 16, };    // Ordering important.
int blackpawn_move_vec[] = { -17, -15, -16, }; // Ordering important.

// must correspond with enum types
int *piece_vec_table[] = {
    NULL,
    whitepawn_move_vec,
    knight_move_vec,
    bishop_move_vec,
    rook_move_vec,
    queen_move_vec,
    king_move_vec,
    NULL,
    NULL,
    blackpawn_move_vec,
    knight_move_vec,
    bishop_move_vec,
    rook_move_vec,
    queen_move_vec,
    king_move_vec,
    NULL
};

int generate_moves()
{
    int i, f, t;
    int *mvec;
    int xside = board.xside;
    int side = board.side;
    Move *ms = know_stack[ply].ms;

    f = KSQ(side);
    for (mvec = king_move_vec; *mvec; mvec++) {
	t = f + *mvec;
	if (Square(t) == EMPTY) { Addmove(f, t, 0); }
	else if (COLOR(Square(t)) == xside) { Addmove(f, t, CAPTURE); }
    }

    if (board.side == WHITE) {
        if (board.castle & WK_CASTLE) {
            if ((Square(F1) == EMPTY) && (Square(G1) == EMPTY)) {
		if (!color_attacks_square(BLACK, E1) && !color_attacks_square(BLACK, F1)) {
                    Addmove(E1, G1, CASTLE); }
            }
        }
        if (board.castle & WQ_CASTLE) {
            if ((Square(B1) == EMPTY)&&(Square(C1) == EMPTY)&&(Square(D1) == EMPTY)) {
		if (!color_attacks_square(BLACK, E1) && !color_attacks_square(BLACK, D1)) {
                    Addmove(E1, C1, CASTLE); }
            }
        }
    }
    else {
        if (board.castle & BK_CASTLE) {
            if ((Square(F8) == EMPTY) && (Square(G8) == EMPTY)) {
		if (!color_attacks_square(WHITE, E8) && !color_attacks_square(WHITE, F8)) {
                    Addmove(E8, G8, CASTLE); }
                }
            }
        if (board.castle & BQ_CASTLE) {
            if ((Square(B8) == EMPTY)&&(Square(C8) == EMPTY)&&(Square(D8) == EMPTY)) {
		 if (!color_attacks_square(WHITE, E8) && !color_attacks_square(WHITE, D8)) {
                     Addmove(E8, C8, CASTLE); }
            }
        }
    }

    for (f = PL_FIRST(PAWN | (side << 3)); f < 128; f = PL_NEXT(f)) {
	mvec = piece_vec_table[PAWN | (side << 3)];
	for (i=0; i<2; i++) {
	    t = *mvec + f;
	    if (COLOR(Square(t)) == xside) {
		if (pawn_rank[side][t] < 7) {
		    Addmove(f, t, CAPTURE | PAWNMOVE); 
		}
		else { 
		    Addpromote(f, t, CAPTURE); 
		}
	    }
	    else if ((t) == board.enpassant) {
		Addmove(f, t, ENPASSANT | CAPTURE | PAWNMOVE); }
	    mvec++;
	}
	t = *mvec + f;
	if (Square(t) == EMPTY) {
	    if (pawn_rank[side][f] == 6) { 
		Addpromote(f, t, 0); }
	    else { 
		Addmove(f, t, PAWNMOVE); }
	    t += *mvec;
	    if ((pawn_rank[side][f] == 1) && (Square(t) == EMPTY)) {
		Addmove(f, t, PAWNMOVE); }
	}
    }

    // knights
    for (f = PL_FIRST(KNIGHT | (side << 3)); f < 128; f = PL_NEXT(f)) {
        t = f + 14;
	if (Square(t) == EMPTY)             { Addmove(f, t, 0); }
	else if (COLOR(Square(t)) == xside) { Addmove(f, t, CAPTURE); }
	t = f + 31;
	if (Square(t) == EMPTY)             { Addmove(f, t, 0); }
	else if (COLOR(Square(t)) == xside) { Addmove(f, t, CAPTURE); }
	t = f + 33;
	if (Square(t) == EMPTY)             { Addmove(f, t, 0); }
	else if (COLOR(Square(t)) == xside) { Addmove(f, t, CAPTURE); }
	t = f + 18;
	if (Square(t) == EMPTY)             { Addmove(f, t, 0); }
	else if (COLOR(Square(t)) == xside) { Addmove(f, t, CAPTURE); }
	t = f - 14;
	if (Square(t) == EMPTY)             { Addmove(f, t, 0); }
	else if (COLOR(Square(t)) == xside) { Addmove(f, t, CAPTURE); }
	t = f - 31;
	if (Square(t) == EMPTY)             { Addmove(f, t, 0); }
	else if (COLOR(Square(t)) == xside) { Addmove(f, t, CAPTURE); }
	t = f - 33;
	if (Square(t) == EMPTY)             { Addmove(f, t, 0); }
	else if (COLOR(Square(t)) == xside) { Addmove(f, t, CAPTURE); }
	t = f - 18;
	if (Square(t) == EMPTY)             { Addmove(f, t, 0); }
	else if (COLOR(Square(t)) == xside) { Addmove(f, t, CAPTURE); }
    }

   for (f = PL_FIRST(BISHOP | (side << 3)); f < 128; f = PL_NEXT(f)) {
       for (t = f + 15; Square(t) == EMPTY; t += 15) { Addmove(f, t, 0); }
       if (COLOR(Square(t)) == xside) { Addmove (f, t, CAPTURE); }
       for (t = f + 17; Square(t) == EMPTY; t += 17) { Addmove(f, t, 0); }
       if (COLOR(Square(t)) == xside) { Addmove (f, t, CAPTURE); }
       for (t = f - 15; Square(t) == EMPTY; t -= 15) { Addmove(f, t, 0); }
       if (COLOR(Square(t)) == xside) { Addmove (f, t, CAPTURE); }
       for (t = f - 17; Square(t) == EMPTY; t -= 17) { Addmove(f, t, 0); }
       if (COLOR(Square(t)) == xside) { Addmove (f, t, CAPTURE); }
   }

   for (f = PL_FIRST(ROOK | (side << 3)); f < 128; f = PL_NEXT(f)) {
       for (t = f - 1; Square(t) == EMPTY; t -= 1) { Addmove(f, t, 0); }
       if (COLOR(Square(t)) == xside) { Addmove (f, t, CAPTURE); }
       for (t = f + 1; Square(t) == EMPTY; t += 1) { Addmove(f, t, 0); }
       if (COLOR(Square(t)) == xside) { Addmove (f, t, CAPTURE); }
       for (t = f + 16; Square(t) == EMPTY; t += 16) { Addmove(f, t, 0); }
       if (COLOR(Square(t)) == xside) { Addmove (f, t, CAPTURE); }
       for (t = f - 16; Square(t) == EMPTY; t -= 16) { Addmove(f, t, 0); }
       if (COLOR(Square(t)) == xside) { Addmove (f, t, CAPTURE); }
   }
   for (f = PL_FIRST(QUEEN | (side << 3)); f < 128; f = PL_NEXT(f)) {
       for (t = f - 1; Square(t) == EMPTY; t -= 1) { Addmove(f, t, 0); }
       if (COLOR(Square(t)) == xside) { Addmove (f, t, CAPTURE); }
       for (t = f + 1; Square(t) == EMPTY; t += 1) { Addmove(f, t, 0); }
       if (COLOR(Square(t)) == xside) { Addmove (f, t, CAPTURE); }
       for (t = f + 16; Square(t) == EMPTY; t += 16) { Addmove(f, t, 0); }
       if (COLOR(Square(t)) == xside) { Addmove (f, t, CAPTURE); }
       for (t = f - 16; Square(t) == EMPTY; t -= 16) { Addmove(f, t, 0); }
       if (COLOR(Square(t)) == xside) { Addmove (f, t, CAPTURE); }
       for (t = f + 15; Square(t) == EMPTY; t += 15) { Addmove(f, t, 0); }
       if (COLOR(Square(t)) == xside) { Addmove (f, t, CAPTURE); }
       for (t = f + 17; Square(t) == EMPTY; t += 17) { Addmove(f, t, 0); }
       if (COLOR(Square(t)) == xside) { Addmove (f, t, CAPTURE); }
       for (t = f - 15; Square(t) == EMPTY; t -= 15) { Addmove(f, t, 0); }
       if (COLOR(Square(t)) == xside) { Addmove (f, t, CAPTURE); }
       for (t = f - 17; Square(t) == EMPTY; t -= 17) { Addmove(f, t, 0); }
       if (COLOR(Square(t)) == xside) { Addmove (f, t, CAPTURE); }
   }

    know_stack[ply + 1].ms = ms;
    return 0;
}


int generate_captures()
{
    int f, t, i;
    int *mvec;
    int xside = board.xside;
    int side = board.side;
    Move *ms = know_stack[ply].ms;

    // kings
    for (f = PL_FIRST(KING | (side << 3)); f < 128; f = PL_NEXT(f)) {
	for (mvec = king_move_vec; *mvec; mvec++) {
	    t = f + *mvec;
	    if (COLOR(Square(t)) == xside) {
		Addmove(f, t, CAPTURE); }
	}
    }
    // knights
    for (f = PL_FIRST(KNIGHT | (side << 3)); f < 128; f = PL_NEXT(f)) {
	for (mvec = knight_move_vec; *mvec; mvec++) {
	    t = f + *mvec;
	    if (COLOR(Square(t)) == xside) {
		Addmove(f, t, CAPTURE); }
	}
    }

   for (f = PL_FIRST(BISHOP | (side << 3)); f < 128; f = PL_NEXT(f)) {
       for (t = f + 15; Square(t) == EMPTY; t += 15) { }
       if (COLOR(Square(t)) == xside) { Addmove (f, t, CAPTURE); }
       for (t = f + 17; Square(t) == EMPTY; t += 17) { }
       if (COLOR(Square(t)) == xside) { Addmove (f, t, CAPTURE); }
       for (t = f - 15; Square(t) == EMPTY; t -= 15) { }
       if (COLOR(Square(t)) == xside) { Addmove (f, t, CAPTURE); }
       for (t = f - 17; Square(t) == EMPTY; t -= 17) { }
       if (COLOR(Square(t)) == xside) { Addmove (f, t, CAPTURE); }
   }
   for (f = PL_FIRST(ROOK | (side << 3)); f < 128; f = PL_NEXT(f)) {
       for (t = f - 1; Square(t) == EMPTY; t -= 1) { }
       if (COLOR(Square(t)) == xside) { Addmove (f, t, CAPTURE); } 
       for (t = f + 1; Square(t) == EMPTY; t += 1) { }
       if (COLOR(Square(t)) == xside) { Addmove (f, t, CAPTURE); }
       for (t = f + 16; Square(t) == EMPTY; t += 16) { }
       if (COLOR(Square(t)) == xside) { Addmove (f, t, CAPTURE); }
       for (t = f - 16; Square(t) == EMPTY; t -= 16) { }
       if (COLOR(Square(t)) == xside) { Addmove (f, t, CAPTURE); }
   }
   for (f = PL_FIRST(QUEEN | (side << 3)); f < 128; f = PL_NEXT(f)) {
       for (t = f - 1; Square(t) == EMPTY; t -= 1) { }
       if (COLOR(Square(t)) == xside) { Addmove (f, t, CAPTURE); } 
       for (t = f + 1; Square(t) == EMPTY; t += 1) { }
       if (COLOR(Square(t)) == xside) { Addmove (f, t, CAPTURE); }
       for (t = f + 16; Square(t) == EMPTY; t += 16) {  }
       if (COLOR(Square(t)) == xside) { Addmove (f, t, CAPTURE); }
       for (t = f - 16; Square(t) == EMPTY; t -= 16) { }
       if (COLOR(Square(t)) == xside) { Addmove (f, t, CAPTURE); }
       for (t = f + 15; Square(t) == EMPTY; t += 15) { }
       if (COLOR(Square(t)) == xside) { Addmove (f, t, CAPTURE); }
       for (t = f + 17; Square(t) == EMPTY; t += 17) { }
       if (COLOR(Square(t)) == xside) { Addmove (f, t, CAPTURE); }
       for (t = f - 15; Square(t) == EMPTY; t -= 15) { }
       if (COLOR(Square(t)) == xside) { Addmove (f, t, CAPTURE); }
       for (t = f - 17; Square(t) == EMPTY; t -= 17) { }
       if (COLOR(Square(t)) == xside) { Addmove (f, t, CAPTURE); }
   }


    // pawns
    for (f = PL_FIRST(PAWN | (side << 3)); f < 128; f = PL_NEXT(f)) {
	mvec = piece_vec_table[PAWN | (side << 3)];

	for (i=0; i<2; i++) {
	    t = *mvec + f;
	    if (COLOR(Square(t)) == xside) {
		if (pawn_rank[side][f] == 6) {  // fix this
		    Addmove (f, t, QUEEN_PROM | PAWNMOVE | CAPTURE);
		}
		else { 
		    Addmove(f, t, CAPTURE | PAWNMOVE); 
		}
	    }
	    else if ((t) == board.enpassant) {
		Addmove(f, t, ENPASSANT | CAPTURE | PAWNMOVE); }
	    mvec++;
	}

	t = *mvec + f;
	if ((Square(t) == EMPTY) && (pawn_rank[side][f] == 6)) { 
		Addmove (f, t, QUEEN_PROM | PAWNMOVE);
	}
    }

    know_stack[ply + 1].ms = ms;
    return 0;
}

int generate_evasions()
{
    int i, f, t, p;
    int *mvec;
    int xside = board.xside;
    int side = board.side;
    int checks;
    Move *ms = know_stack[ply].ms;

    checks = find_checks();

    f = PL_FIRST(KING | (side << 3));
    for (mvec = king_move_vec; *mvec; mvec++) {
	t = f + *mvec;
	if (!attack_tab[xside][t]) {
	    if (Square(t) == EMPTY) { Addmove(f, t, 0); }
	    else if (COLOR(Square(t)) == xside) { Addmove(f, t, CAPTURE); }
	}
    }

    if (checks > 1) {
        know_stack[ply + 1].ms = ms;
	return 0;
    }

    // knights
    for (f = PL_FIRST(KNIGHT | (side << 3)); f < 128; f = PL_NEXT(f)) {
	for (mvec = knight_move_vec; *mvec; mvec++) {
	    t = f + *mvec;
	    if (check_tab[t]) {
	        if (Square(t) == EMPTY) { Addmove(f, t, 0); }
	        else if (COLOR(Square(t)) == xside) { Addmove(f, t, CAPTURE); }
	    }
	}
    }
    // sliding pieces
    for (p = BISHOP; p <= QUEEN; p++) {
        for (f = PL_FIRST(p | (side << 3)); f < 128; f = PL_NEXT(f)) {
            for (mvec = piece_vec_table[p]; *mvec; mvec++) {
                for (t = f + *mvec; Square(t) == EMPTY; t += *mvec) {
		    if (check_tab[t]) {
                        Addmove(f, t, 0);
		    }
		}
		if (COLOR(Square(t)) == xside) { 
		    if (check_tab[t]) {
		        Addmove (f, t, CAPTURE); 
		    }
		}
	    }
	}
    }

    for (f = PL_FIRST(PAWN | (side << 3)); f < 128; f = PL_NEXT(f)) {
	mvec = piece_vec_table[PAWN | (side << 3)];
	for (i=0; i<2; i++) {
	    t = *mvec + f;
	    if (check_tab[t]) {
	        if (COLOR(Square(t)) == xside) {
		    if (pawn_rank[side][f] == 6) { 
		        Addpromote(f, t, CAPTURE); 
		    }
		    else { 
		        Addmove(f, t, CAPTURE | PAWNMOVE); 
		    }
	        }
	    }
	    else if ((board.enpassant < 128) && (t == board.enpassant)) {
		    Addmove(f, t, ENPASSANT | CAPTURE | PAWNMOVE); }
	    mvec++;
	}
	t = *mvec + f;
	if (Square(t) == EMPTY) {
	    if (check_tab[t]) {
	        if (pawn_rank[side][f] == 6) { 
		    Addpromote(f, t, 0); 
	        }
	        else { 
		    Addmove(f, t, PAWNMOVE); 
	        }
	    }
	    t += *mvec;
	    if (check_tab[t]) {
	        if ((pawn_rank[side][f] == 1) && (Square(t) == EMPTY)) {
		    Addmove(f, t, PAWNMOVE); }
	    }
	}
    }

    know_stack[ply + 1].ms = ms;
    return 0;
}

