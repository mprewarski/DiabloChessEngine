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
#include <assert.h>

#include "diablo.h"

int verify_lists();
int debug_info();

int make_move(int move)
{
    Knowledge *know = know_stack + ply;
    unsigned int f = FROM(move);
    unsigned int t = TO(move);
    int oldcastle = board.castle;
    int enpassant_cap_sq = 0;
    int side = board.side;
    int prom_p = 0;
    int p;


    // assert(f <= H8);
    // assert(t <= H8);

    know->undo.enpassant = board.enpassant;
    know->undo.castle = board.castle;
    know->undo.fifty = board.fifty;
    know->undo.hash = board.hash;
    know->undo.capture = Square(t);
    know->move = move;
    (know+1)->material = know->material;

    p = Square(f);
    board.enpassant = 0xc0;
    ply++;

    if ((move & (CAPTURE | PAWNMOVE))) 
	board.fifty = 0;
    else
	board.fifty++;

    if (move & CAPTURE) {
    	if (move & ENPASSANT) {
	    if (side == WHITE) {
		enpassant_cap_sq = t-16;
	    	PL_REMOVE(enpassant_cap_sq);
    	    	know->undo.capture = BPAWN;
		Square(enpassant_cap_sq) = EMPTY;
	    }
	    else {
		enpassant_cap_sq = t+16;
	    	PL_REMOVE(enpassant_cap_sq);
    	    	know->undo.capture = WPAWN;
		Square(enpassant_cap_sq) = EMPTY;
	    }
    	}
	else {
	    PL_REMOVE(t);
	}
    }
    if (move & PROMOTION) {
	prom_p = promoted[(move >> 16) & 0xf] | (side << 3);
	Square(t) = prom_p;
	PL_REMOVE(f); PL_INSERT(prom_p, t);
    }
    else {
        Square(t) = p;
        PL_MOVE(f, t);
    }

    Square(f) = EMPTY;

    if (move & CASTLE) {
	if (t == G1) {
	    Square(H1) = EMPTY;
	    Square(F1) = WROOK;
	    PL_MOVE(H1, F1); }
	else if (t == C1) {
	    Square(A1) = EMPTY;
	    Square(D1) = WROOK;
	    PL_MOVE(A1, D1); }
	else if (t == G8) {
	    Square(H8) = EMPTY;
	    Square(F8) = BROOK;
	    PL_MOVE(H8, F8); }
	else {
	    Square(A8) = EMPTY;
	    Square(D8) = BROOK;
	    PL_MOVE(A8, D8); }
    }

    if ((Square(t) == WPAWN) && ((f+32) == t)) 
	board.enpassant = f+16;
    else if ((Square(t) == BPAWN) && ((f-32) == t))
	board.enpassant = f-16;


    if (board.castle) {
	if (f == E1) 
	    board.castle &= ~(WK_CASTLE | WQ_CASTLE);
	else if (f == E8) 
	    board.castle &= ~(BK_CASTLE | BQ_CASTLE);

	if ((f == A1) || (t == A1))
	    board.castle &= ~WQ_CASTLE;
	if ((f == H1) || (t == H1))
	    board.castle &= ~WK_CASTLE;
	if ((f == A8) || (t == A8))
	    board.castle &= ~BQ_CASTLE;
	if ((f == H8) || (t == H8))
	    board.castle &= ~BK_CASTLE;
    }

    board.hash ^= zobrist_side[0];
    board.hash ^= zobrist_board[p][f];
    if (move & PROMOTION) {
        board.hash ^= zobrist_board[prom_p][t];
    }
    else {
        board.hash ^= zobrist_board[p][t];
    }

    if (move & CAPTURE) {
	if (move & ENPASSANT) {
	    if (side == WHITE) 
            	board.hash ^= zobrist_board[BPAWN][enpassant_cap_sq];
	    else 
            	board.hash ^= zobrist_board[WPAWN][enpassant_cap_sq];
	}
	else {
            board.hash ^= zobrist_board[know->undo.capture][t];
	}
    }

    if (oldcastle != board.castle) {
	if (move & CASTLE) {
            board.hash ^= zobrist_castle[oldcastle];
            board.hash ^= zobrist_castle[board.castle];
	    if (t == G1) {
                board.hash ^= zobrist_board[WROOK][H1];
                board.hash ^= zobrist_board[WROOK][F1];
	    }
	    else if (t == C1) {
                board.hash ^= zobrist_board[WROOK][A1];
                board.hash ^= zobrist_board[WROOK][D1];
	    }
	    else if (t == G8) {
                board.hash ^= zobrist_board[BROOK][H8];
                board.hash ^= zobrist_board[BROOK][F8];
	    }
	    else {
                board.hash ^= zobrist_board[BROOK][A8];
                board.hash ^= zobrist_board[BROOK][D8];
	    }
	}
	else {
            board.hash ^= zobrist_castle[oldcastle];
            board.hash ^= zobrist_castle[board.castle];
	}
    }

    if ((move & CAPTURE)) {
	(know+1)->material -= piece_value[know->undo.capture];
    }
    if (move & PROMOTION) {
	(know+1)->material -= piece_value[PAWN];
	(know+1)->material += piece_value[prom_p];
    }


    board.side ^= 1;
    board.xside ^= 1;
    board.gameply++;

    move_hist[board.gameply].move = move;
    move_hist[board.gameply].hash_key = board.hash;


    // verify_lists(move, "make");
    return 0;
}


int unmake_move(int move)
{
    Knowledge *know;
    int f, t, p;

    ply--;
    board.side ^= 1;
    board.xside ^= 1;
    board.gameply--;

    know = know_stack + ply;

    board.enpassant = know->undo.enpassant;
    board.castle = know->undo.castle;
    board.fifty = know->undo.fifty;
    board.hash = know->undo.hash;

    f = FROM(move); t = TO(move);
    p = Square(t);

    Square(t) = EMPTY;

    if (move & PROMOTION) {
	PL_REMOVE(t);
	PL_INSERT(PAWN | (board.side << 3), f)
	Square(f) = PAWN | (board.side << 3);
    }
    else {
        PL_MOVE(t, f);
        Square(f) = p;
    }

    if (move & CAPTURE) {
	if (move & ENPASSANT) {
	    if (RANK(t) == 5) {
	    	PL_INSERT(BPAWN, t-16);
		Square(t-16) = BPAWN; }
	    else {
	    	PL_INSERT(WPAWN, t+16);
		Square(t+16) = WPAWN; }
	}
	else {
	    Square(t) = know->undo.capture;
	    PL_INSERT(know->undo.capture, t);
	}
    }

    if (move & CASTLE) {
	if (t == G1) {
	    Square(F1) = EMPTY;
	    Square(H1) = WROOK;
	    PL_MOVE(F1, H1); }
	else if (t == C1) {
	    Square(D1) = EMPTY;
	    Square(A1) = WROOK;
	    PL_MOVE(D1, A1); }
	else if (t == G8) {
	    Square(F8) = EMPTY;
	    Square(H8) = BROOK;
	    PL_MOVE(F8, H8); }
	else {
	    Square(D8) = EMPTY;
	    Square(A8) = BROOK;
	    PL_MOVE(D8, A8); }
    }

    // verify_lists(move, "unmake");
    return 0;
}

int make_null_move()
{
    Knowledge *know = know_stack + ply;

    know->undo.enpassant = board.enpassant;
    know->undo.castle = board.castle;
    know->undo.fifty = board.fifty;
    know->undo.hash = board.hash;
    know->undo.capture = 0;
    know++;
    know->ms = (know-1)->ms;
    know->material = (know-1)->material;

    board.hash ^= zobrist_side[0];
    board.fifty++;
    board.enpassant = 0xc0;

    ply++;
    board.side ^= 1;
    board.xside ^= 1;
    board.gameply++;

    move_hist[board.gameply].move = 0;
    move_hist[board.gameply].hash_key = board.hash;

    // verify_lists(0, "make null");

    return 0;
}

int unmake_null_move()
{
    Knowledge *know;

    ply--;
    board.gameply--;
    board.side ^= 1;
    board.xside ^= 1;

    know = know_stack + ply;
    board.enpassant = know->undo.enpassant;
    board.castle = know->undo.castle;
    board.fifty = know->undo.fifty;
    board.hash = know->undo.hash;

    // verify_lists(0, "unmake null");
    return 0;
}


