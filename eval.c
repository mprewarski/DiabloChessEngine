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
#include "data.h"

#define PAWN_DOUBLED_OPEN	4    // counted for each pawn
#define PAWN_DOUBLED_END	7    // counted for each pawn
#define PAWN_BACKWARD_OPEN	7
#define PAWN_BACKWARD_END       14
#define TRAPPED_ROOK		60

void init_eval();
void basic_eval();
void pawns_eval();
void kings_eval();
void misc_eval();
int passed_pawn(int color, int s, int rank);

short pawn_file[2][10];
short backward_pawn[2][10];

int bishop_mob[16] = {-15, -5, 0,  5, 10, 15, 20, 24, 28, 32, 36, 38, 40, 42, 44, 46};
int rook_mob[16] =   {-15, -5, -2,  0, 3, 6, 9, 12,  15,  18, 20, 22, 24, 25, 26, 27 };

int pawn_advance[2][8] = {
	{0,  5, 10, 20, 35, 55, 90, 0},
	{0, 90, 55, 35, 20, 10,  5, 0}
};

static Knowledge *know;
static int valueOpen[2];
static int valueEnd[2];
static int king_threat[2];

int evaluate()
{
    int value = 0;
    int openscore;
    int endscore;

    init_eval();
    pawns_eval();
    // printf("after pawns: open=%d end=%d\n", valueOpen[0]-valueOpen[1], valueEnd[0]-valueEnd[1]);
    basic_eval();
    //printf("after basic: open=%d end=%d\n", valueOpen[0]-valueOpen[1], valueEnd[0]-valueEnd[1]);
    kings_eval();
    //printf("after kings:  open=%d end=%d\n", valueOpen[0]-valueOpen[1], valueEnd[0]-valueEnd[1]);
    misc_eval();
    //printf("after misc:  open=%d end=%d\n", valueOpen[0]-valueOpen[1], valueEnd[0]-valueEnd[1]);

    openscore = valueOpen[WHITE] - valueOpen[BLACK];
    endscore  = valueEnd[WHITE]  - valueEnd[BLACK];

    if (know->material <= ENDGAME_MAT )
    {
        value = endscore;
    }
    else
    {
        value = (((know->material - ENDGAME_MAT) * openscore) +
                 ((OPENING_MAT - know->material) * endscore) )
                   / (OPENING_MAT - ENDGAME_MAT);
    }

    // some very basic draw detection. If each side has only 1 piece or less and that piece
    // isn't a rook or queen, then return draw score
    if((know->pawns[0] + know->pawns[1] == 0)) {
        if ((know->pieces[0] <= 1) && (know->pieces[1] <= 1)) {
            if (((know->rooks[0] + know->queens[0]) == 0) && 
		 ((know->rooks[1] + know->queens[1]) == 0))
                value = 0;
        }
    }

    if (board.side == WHITE) return value;
    else return -value;
}


void init_eval()
{
    int i;

    memset(pawn_file, 0, sizeof(pawn_file));
    memset(backward_pawn, 0, sizeof(backward_pawn));

    SET_ATTACK_TAB();

    memset(&allattack_tab[ply][0][64], 0, 128);
    memset(&allattack_tab[ply][1][64], 0, 128);

    for (i = 0; i < 10; i++) {
	backward_pawn[0][i] = 6;
	backward_pawn[1][i] = 1;
    }

    know = know_stack + ply;
    know->pawns[WHITE] = know->pawns[BLACK] = 0; 
    know->knights[WHITE] = know->knights[BLACK] = 0;
    know->bishops[WHITE] = know->bishops[BLACK] = 0; 
    know->rooks[WHITE] = know->rooks[BLACK] = 0;
    know->queens[WHITE] = know->queens[BLACK] = 0;

    valueOpen[0] = valueOpen[1] = 0;
    valueEnd[0]  = valueEnd[1]  = 0;
}

char attack_pattern[32] = {
//      . P N N R R R R Q Q Q Q Q Q Q Q K K K K K K K K K K K K K K K K
//            P   P N N   P N N R R R R   P N N R R R R Q Q Q Q Q Q Q Q
//                    P       P   N N N       P   P N N   P N N R R R R
        0,0,0,0,0,0,1,1,0,1,2,2,2,3,3,3,0,0,0,0,1,1,2,2,2,3,3,3,3,3,3,3 };

int king_shield_value[2][32] = {
    {0,18,2,6,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,18,2,6,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

// threat value based on number of pawns on a file
int king_pawnshield_threat[8] = {
        1, 0, 0, 0, 0, 0, 0, 0
};

void kings_eval()
{
    int s;
    int kfile;
    int to;
    int color;
    int xcolor;
    int *mvec;
    int atk_pat;

    if (know->material <= ENDGAME_MAT)
	return;

    // calculate king shield bonus
    s = KSQ(WHITE);
    valueOpen[WHITE] += king_shield_value[WHITE][Square(s+15)];
    valueOpen[WHITE] += king_shield_value[WHITE][Square(s+16)];
    valueOpen[WHITE] += king_shield_value[WHITE][Square(s+17)];
    valueOpen[WHITE] += king_shield_value[WHITE][Square(s+31)] / 2;
    valueOpen[WHITE] += king_shield_value[WHITE][Square(s+32)] / 2;
    valueOpen[WHITE] += king_shield_value[WHITE][Square(s+33)] / 2;
    s = KSQ(BLACK);
    valueOpen[BLACK] += king_shield_value[BLACK][Square(s-15)];
    valueOpen[BLACK] += king_shield_value[BLACK][Square(s-16)];
    valueOpen[BLACK] += king_shield_value[BLACK][Square(s-17)];
    valueOpen[BLACK] += king_shield_value[BLACK][Square(s-31)] / 2;
    valueOpen[BLACK] += king_shield_value[BLACK][Square(s-32)] / 2;
    valueOpen[BLACK] += king_shield_value[BLACK][Square(s-33)] / 2;

    king_threat[WHITE] = king_threat[BLACK] = 0;
    for (color = WHITE; color <= BLACK; color++) {
	s = KSQ(color);
	xcolor = color^1;
	atk_pat = 0;
	king_threat[color] = king_exposed[color][s];

	for (mvec = king_move_vec; *mvec; mvec++) {
	    to = s + *mvec;
	    if (!(to & 0x88)) {
		if (attack_tab[xcolor][to]) {
	            king_threat[color] += (attack_tab[xcolor][to] & 7);
	            atk_pat |= (attack_tab[xcolor][to]);
		    if (attack_tab[color][to] > 0x51) {
			king_threat[color]++;
		    }
		}
	    }
	}

	// increase threat value if no pawns in front
	kfile = COLUMN(s) + 1;
	king_threat[color] += king_pawnshield_threat[pawn_file[color][kfile-1]];
	king_threat[color] += king_pawnshield_threat[pawn_file[color][kfile]];
	king_threat[color] += king_pawnshield_threat[pawn_file[color][kfile+1]];

	if (color == WHITE) {
	    if (!((s+31) & 0x88)) 
		atk_pat |= (attack_tab[xcolor][s+31]);
	    if (!((s+32) & 0x88)) 
		atk_pat |= (attack_tab[xcolor][s+32]);
	    if (!((s+33) & 0x88)) 
		atk_pat |= (attack_tab[xcolor][s+33]);
	}
	else {
	    if (!((s-31) & 0x88)) 
		atk_pat |= (attack_tab[xcolor][s-31]);
	    if (!((s-32) & 0x88)) 
		atk_pat |= (attack_tab[xcolor][s-32]);
	    if (!((s-33) & 0x88)) 
		atk_pat |= (attack_tab[xcolor][s-33]);
	}
	king_threat[color] += attack_pattern[(atk_pat >> 3)];
	know->king_threat[color] = king_threat[color];
    }

    valueOpen[0] -= king_threat[0] * king_threat[0];
    valueOpen[1] -= king_threat[1] * king_threat[1];

    // printf("w kt %d   b kt %d  \n", king_threat[0], king_threat[1]);
    // printf("king safety w %d  b %d  ret %d\n", value[0], value[1], ret);
}

void pawns_eval()
{
    int color;
    int s;
    int file;
    int rank;
    int piece;
    int passedValue;
    char *atk_tab;
    int *mvec;
    int to;
    int *pawn_table[2];  // pointer to pawn piece square tables

    // decide which pawn table to use
    {
        int kf0 = COLUMN(KSQ(0));
        int kf1 = COLUMN(KSQ(1));
        if ((kf0 > E_FILE) && (kf1 < E_FILE)) {
            pawn_table[WHITE] = &queenstorm_tab[WHITE][0];
            pawn_table[BLACK] = &kingstorm_tab[BLACK][0];
        } 
        else if ((kf0 < E_FILE) && (kf1 > E_FILE))
        {
            pawn_table[WHITE] = &kingstorm_tab[WHITE][0];
            pawn_table[BLACK] = &queenstorm_tab[BLACK][0];
        }
        else {
            pawn_table[0] = &pawn_tab[0][0];
            pawn_table[1] = &pawn_tab[1][0];
        }
    }

    for (color = WHITE; color <= BLACK; color++) {
        atk_tab = attack_tab[color];
        piece = PAWN | (color << 3);
        s = PL_FIRST(piece);
        while (s < 128) {
            file = COLUMN(s)+1;
            valueOpen[color] += pawn_table[color][s] + PAWN_VAL - 10;
            valueEnd[color] +=  pawn_end_tab[color][s] + PAWN_VAL;
            know->pawns[color]++;
            pawn_file[color][file]++;
            if (color == WHITE) {
                if (RANK(s) < backward_pawn[0][file])
                    backward_pawn[0][file] = RANK(s);
            }
            else {
                if (RANK(s) > backward_pawn[1][file])
                    backward_pawn[1][file] = RANK(s);
            }
            mvec = piece_vec_table[piece];
            to = s + *mvec;
            atk_tab[to] |= PAWN_ATK; atk_tab[to]++;
            to += 2;
            atk_tab[to] |= PAWN_ATK; atk_tab[to]++;
            s = PL_NEXT(s);
        }
    }

    for (color = WHITE; color <= BLACK; color++) {
        s = PL_FIRST(PAWN | (color << 3));
        while (s < 128) {
	    file = COLUMN(s)+1;
	    rank = RANK(s);
	    if (pawn_file[color][file] > 1) {
	        valueOpen[color] -= PAWN_DOUBLED_OPEN;
	        valueEnd[color]  -= PAWN_DOUBLED_END;
	    }
	    if (color == WHITE) {
	        if ((backward_pawn[color][file-1] > rank) && (backward_pawn[color][file+1] > rank)){
	            valueOpen[color] -= PAWN_BACKWARD_OPEN;
	            valueEnd[color]  -= PAWN_BACKWARD_END;
	        }
		if ((rank >= backward_pawn[1][file-1]) &&
		        (rank >= backward_pawn[1][file]) &&
		        (rank >= backward_pawn[1][file+1]) ) {
		        passedValue = passed_pawn(0, s, rank);
			valueOpen[0] += passedValue / 2;
			valueEnd[0]  += passedValue;
		}
	    }
	    else { // color == BLACK
	        if ((backward_pawn[color][file-1] < rank) && (backward_pawn[color][file+1] < rank)){
		    // printf("color %d  square %x is backward \n", color, s);
	            valueOpen[color] -= PAWN_BACKWARD_OPEN;
	            valueEnd[color]  -= PAWN_BACKWARD_END;
	        }
		if ((rank <= backward_pawn[0][file-1]) &&
		        (rank <= backward_pawn[0][file]) &&
		        (rank <= backward_pawn[0][file+1]) ) {

		    passedValue = passed_pawn(1, s, rank);
		    valueOpen[1] += passedValue / 2;
		    valueEnd[1] += passedValue;
		}
	    }

        s = PL_NEXT(s);
	}
    }

}

void basic_eval()
{
    int value[2];
    int color;
    int s;
    int to;
    int mob;
    char *atk_tab;

    value[0] = value[1] = 0;

    for (color = WHITE; color <= BLACK; color++) {
	atk_tab = attack_tab[color];

        s = PL_FIRST(KNIGHT | (color << 3));
        while(s < 128) {
            value[color] += knight_tab[color][s] + KNIGHT_VAL;
            know->knights[color]++;
            // knight outpost
            if (atk_tab[s] & PAWN_ATK) {
                value[color] += outposts[color][s];
            }
            atk_tab[s+14] |= KNIGHT_ATK; atk_tab[s+14]++;
            atk_tab[s+31] |= KNIGHT_ATK; atk_tab[s+31]++;
            atk_tab[s+33] |= KNIGHT_ATK; atk_tab[s+33]++;
            atk_tab[s+18] |= KNIGHT_ATK; atk_tab[s+18]++;
            atk_tab[s-14] |= KNIGHT_ATK; atk_tab[s-14]++;
            atk_tab[s-31] |= KNIGHT_ATK; atk_tab[s-31]++;
            atk_tab[s-33] |= KNIGHT_ATK; atk_tab[s-33]++;
            atk_tab[s-18] |= KNIGHT_ATK; atk_tab[s-18]++;
            s = PL_NEXT(s);
        }

        s = PL_FIRST(BISHOP | (color << 3));
        while (s < 128) {
	    value[color] += bishop_tab[color][s] + BISHOP_VAL;
	    know->bishops[color]++;
	    mob = 0;

	    for (to = s + 15; Square(to) == EMPTY; to += 15) {
		atk_tab[to] |= BISHOP_ATK; atk_tab[to]++; mob++;
	    }
	    atk_tab[to] |= BISHOP_ATK; atk_tab[to]++;

	    for (to = s + 17; Square(to) == EMPTY; to += 17) {
		atk_tab[to] |= BISHOP_ATK; atk_tab[to]++; mob++;
	    }
	    atk_tab[to] |= BISHOP_ATK; atk_tab[to]++;

	    for (to = s - 15; Square(to) == EMPTY; to -= 15) {
		atk_tab[to] |= BISHOP_ATK; atk_tab[to]++; mob++;
	    }
	    atk_tab[to] |= BISHOP_ATK; atk_tab[to]++;

	    for (to = s - 17; Square(to) == EMPTY; to -= 17) {
		atk_tab[to] |= BISHOP_ATK; atk_tab[to]++; mob++;
	    }
	    atk_tab[to] |= BISHOP_ATK; atk_tab[to]++;

	    value[color] += bishop_mob[mob]; 
            s = PL_NEXT(s);
        }

        s = PL_FIRST( ROOK | (color << 3));
        while(s < 128) {
	    value[color] += rook_tab[color][s] + ROOK_VAL;
	    know->rooks[color]++;

	    // rooks on the 7th
	    if (pawn_rank[color][s] == 6) {
		valueEnd[color] += 10;
		if (pawn_rank[color][KSQ(color ^ 1)] == 7)
		    valueEnd[color] += 20;
	    }
	    // rooks on open file
	    if (!pawn_file[color][COLUMN(s)+1]) {
		value[color] += 5;
	        if (!pawn_file[color^1][COLUMN(s)+1]) {
		    value[color] += 10;
		}
	    }
	    mob = 0;
	    for (to = s - 1; Square(to) == EMPTY; to -= 1) {
		atk_tab[to] |= ROOK_ATK; atk_tab[to]++; mob++;
	    }
	    atk_tab[to] |= ROOK_ATK; atk_tab[to]++;
	    for (to = s + 1; Square(to) == EMPTY; to += 1) {
		atk_tab[to] |= ROOK_ATK; atk_tab[to]++; mob++;
	    }
	    atk_tab[to] |= ROOK_ATK; atk_tab[to]++;
	    for (to = s + 16; Square(to) == EMPTY; to += 16) {
		atk_tab[to] |= ROOK_ATK; atk_tab[to]++; mob++;
	    }
	    atk_tab[to] |= ROOK_ATK; atk_tab[to]++;
	    for (to = s - 16; Square(to) == EMPTY; to -= 16) {
		atk_tab[to] |= ROOK_ATK; atk_tab[to]++; mob++;
	    }
	    atk_tab[to] |= ROOK_ATK; atk_tab[to]++;
	    value[color] += rook_mob[mob]; 
            s = PL_NEXT(s);
        }

        s = PL_FIRST(QUEEN | (color << 3));
        while (s < 128) {
	    value[color] += queen_tab[color][s] + QUEEN_VAL;
	    know->queens[color]++;

	    for (to = s-1; Square(to) == EMPTY; to -= 1) {
	        atk_tab[to] |= QUEEN_ATK; atk_tab[to]++;
	    }
	    atk_tab[to] |= QUEEN_ATK; atk_tab[to]++;

	    for (to = s+1; Square(to) == EMPTY; to += 1) {
	        atk_tab[to] |= QUEEN_ATK; atk_tab[to]++;
	    }
	    atk_tab[to] |= QUEEN_ATK; atk_tab[to]++;

	    for (to = s+16; Square(to) == EMPTY; to += 16) {
	        atk_tab[to] |= QUEEN_ATK; atk_tab[to]++;
	    }
	    atk_tab[to] |= QUEEN_ATK; atk_tab[to]++;

	    for (to = s-16; Square(to) == EMPTY; to -= 16) {
	        atk_tab[to] |= QUEEN_ATK; atk_tab[to]++;
	    }
	    atk_tab[to] |= QUEEN_ATK; atk_tab[to]++;

	    for (to = s+15; Square(to) == EMPTY; to += 15) {
	        atk_tab[to] |= QUEEN_ATK; atk_tab[to]++;
	    }
	    atk_tab[to] |= QUEEN_ATK; atk_tab[to]++;

	    for (to = s+17; Square(to) == EMPTY; to += 17) {
	        atk_tab[to] |= QUEEN_ATK; atk_tab[to]++;
	    }
	    atk_tab[to] |= QUEEN_ATK; atk_tab[to]++;

	    for (to = s-15; Square(to) == EMPTY; to -= 15) {
	        atk_tab[to] |= QUEEN_ATK; atk_tab[to]++;
	    }
	    atk_tab[to] |= QUEEN_ATK; atk_tab[to]++;

	    for (to = s-17; Square(to) == EMPTY; to -= 17) {
	        atk_tab[to] |= QUEEN_ATK; atk_tab[to]++;
	    }
	    atk_tab[to] |= QUEEN_ATK; atk_tab[to]++;
            s = PL_NEXT(s);
        }

	s = KSQ(color);
	valueOpen[color] += king_tab[color][s] + KING_VAL;
	valueEnd[color] += king_end_tab[color][s] + KING_VAL;

	atk_tab[s-1] |= KING_ATK; atk_tab[s-1]++;
	atk_tab[s+1] |= KING_ATK; atk_tab[s+1]++;
	atk_tab[s+16] |= KING_ATK; atk_tab[s+16]++;
	atk_tab[s-16] |= KING_ATK; atk_tab[s-16]++;
	atk_tab[s+15] |= KING_ATK; atk_tab[s+15]++;
	atk_tab[s+17] |= KING_ATK; atk_tab[s+17]++;
	atk_tab[s-15] |= KING_ATK; atk_tab[s-15]++;
	atk_tab[s-17] |= KING_ATK; atk_tab[s-17]++;

        // bishop pair
        if (know->bishops[color] > 1) {
	    value[color] += (16 - (know->pawns[WHITE]+know->pawns[BLACK])) * 4;
        }

	valueOpen[color] += value[color];
	valueEnd[color] += value[color];
    }

    know->pieces[0] = know->knights[0]+know->bishops[0]+know->rooks[0]+know->queens[0];
    know->pieces[1] = know->knights[1]+know->bishops[1]+know->rooks[1]+know->queens[1];

    
}

extern int maxdist[];

int passed_pawn(int color, int s, int rank)
{
    int adv;
    int value = 0;
    int kingdist;
    int t;

    adv = pawn_advance[color][rank];

    if (color == WHITE) {
	if (Square(s+16) == EMPTY) value += adv;
	else value += adv / 2;

	if (Square(s-15) == WPAWN) value += adv*2;
	if (Square(s-17) == WPAWN) value += adv*2;
	if (Square(s+1) == WPAWN) value += adv*2;
	if (Square(s-1) == WPAWN) value += adv*2;

	for (t = s-16; Square(t) == EMPTY; t-=16);
	if (( (Square(t)&7) == ROOK)) {
	    if (COLOR(Square(t)) == color) { value +=20; }
	    else { value -=20; }
	}

	if (know->material < ENDGAME_MAT) {
	    kingdist = distance[abs(s - KSQ(0))];
	    value -= (kingdist * adv) / 10;
	    kingdist = distance[abs(s - KSQ(1))];
	    value += (kingdist * adv) / 10;
	}
    }
    else {
	if (Square(s-16) == EMPTY) value += adv;
	else value += adv / 2;

	if (Square(s+15) == BPAWN) value += adv*2;
	if (Square(s+17) == BPAWN) value += adv*2;
	if (Square(s+1) == BPAWN) value += adv*2;
	if (Square(s-1) == BPAWN) value += adv*2;

	for (t = s+16; Square(t) == EMPTY; t+=16);
	if (( (Square(t)&7) == ROOK)) {
	    if (COLOR(Square(t)) == color)
		value +=20;
	    else
		value -=20;
	}
	if (know->material < ENDGAME_MAT) {
	    kingdist = distance[abs(s - KSQ(1))];
	    value -= (kingdist * adv) / 10;
	    kingdist = distance[abs(s - KSQ(0))];
	    value += (kingdist * adv) / 10;
	}
    }

    // printf("color %d  square %x   adv %d  value %d\n", color, s, adv, value);

    return value;
}

int development_penalty[] = {0, 6, 10, 14, 21, 29, 41, 54, 65, 80, 100, 120};

void misc_eval()
{
    int d0 = 0;
    int d1 = 0;
    int c0 = 0;
    int c1 = 0;

    if (know_stack[ply].material > ENDGAME_MAT) {

	// development
    	if (Square(B1) == WKNIGHT) d0 += 2;
    	if (Square(G1) == WKNIGHT) d0 += 2;
    	if (Square(C1) == WBISHOP) d0 += 2;
    	if (Square(F1) == WBISHOP) d0 += 2;
	if (board.castle & (WK_CASTLE | WQ_CASTLE)) d0 += 2;
	valueOpen[0] -= development_penalty[d0];

    	if (Square(B8) == BKNIGHT) d1 += 2;
    	if (Square(G8) == BKNIGHT) d1 += 2;
    	if (Square(C8) == BBISHOP) d1 += 2;
    	if (Square(F8) == BBISHOP) d1 += 2;
	if (board.castle & (BK_CASTLE | BQ_CASTLE)) d1 += 2;
	valueOpen[1] -= development_penalty[d1];

	// center control
	c0 += ((attack_tab[0][C5]  & PAWN_ATK) >> 3) * 5;
	c0 += ((attack_tab[0][D5]  & PAWN_ATK) >> 3) * 5;
	c0 += ((attack_tab[0][E5]  & PAWN_ATK) >> 3) * 5;
	c0 += ((attack_tab[0][F5]  & PAWN_ATK) >> 3) * 5;
	c1 += ((attack_tab[1][C4]  & PAWN_ATK) >> 3) * 5;
	c1 += ((attack_tab[1][D4]  & PAWN_ATK) >> 3) * 5;
	c1 += ((attack_tab[1][E4]  & PAWN_ATK) >> 3) * 5;
	c1 += ((attack_tab[1][F4]  & PAWN_ATK) >> 3) * 5;
	valueOpen[0] += c0;
	valueOpen[1] += c1;

        // Trapped Rooks
        if (   ((Square(F1) == WKING) || (Square(G1) == WKING))
            && ((Square(H1) == WROOK) || (Square(G1) == WROOK))) {
            valueOpen[0] -= TRAPPED_ROOK;
        }
        if (   ((Square(C1) == WKING) || (Square(D1) == WKING))
            && ((Square(A1) == WROOK) || (Square(B1) == WROOK))) {
            valueOpen[0] -= TRAPPED_ROOK;
        }
        if (   ((Square(F8) == BKING) || (Square(G8) == BKING))
            && ((Square(H8) == BROOK) || (Square(G8) == BROOK))) {
            valueOpen[1] -= TRAPPED_ROOK;
        }
        if (   ((Square(C8) == BKING) || (Square(D8) == BKING))
            && ((Square(A8) == BROOK) || (Square(B8) == BROOK))) {
            valueOpen[1] -= TRAPPED_ROOK;
        }
    }
}
