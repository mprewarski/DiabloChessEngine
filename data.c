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

#include "diablo.h"
#include "data.h"


Board	board;
char	inputline[4096];
Move	move_stack[2048];
MoveHist move_hist[2048];
Knowledge know_stack[MAX_DEPTH];
char	move_str[32];
int	history[2][128][128];
unsigned char distance[128];
unsigned char allattack_tab[MAX_DEPTH][2][256];
unsigned char *attack_tab[2];
char quicksee_tab[6][0x10000];
Attack attack_opportunity_[256];
Attack *attack_opportunity;

SearchStats search_stats;
int	ply;
int	nodes;
int	bestmove;
int	stop_search;
int	game_flags;
int	search_flags;
int 	idepth;
int 	max_search_depth;
int	start_time;
unsigned int	stop_time;
int	time_left;
int	otime_left;
int	moves_to_go;
int	root_moves;
unsigned int  maxhist;

int promoted[9] = {0, KNIGHT, BISHOP, 0, ROOK, 0, 0, 0, QUEEN};

int threattype[18] = { 
    0, WPAWN_THREAT, KNIGHT_THREAT, BISHOP_THREAT, ROOK_THREAT, QUEEN_THREAT, KING_THREAT,
    0, 0,
    BPAWN_THREAT, KNIGHT_THREAT, BISHOP_THREAT, ROOK_THREAT, QUEEN_THREAT, KING_THREAT,
    0, 0, 0};
      
int piece_value[18] = 
   {0, PAWN_VAL, KNIGHT_VAL, BISHOP_VAL, ROOK_VAL, QUEEN_VAL, KING_VAL, 0, 0, 
       PAWN_VAL, KNIGHT_VAL, BISHOP_VAL, ROOK_VAL, QUEEN_VAL, KING_VAL, 0, 0, 0};

int outposts[2][128] = {
    {
	0,   0,   0,   0,   0,   0,   0,   0,      0,0,0,0,0,0,0,0, 
	0,   0,   0,   0,   0,   0,   0,   0,      0,0,0,0,0,0,0,0, 
	0,   0,   0,   0,   0,   0,   0,   0,      0,0,0,0,0,0,0,0, 
	0,   0,   0,   8,   8,   0,   0,   0,      0,0,0,0,0,0,0,0, 
	0,   0,  10,  18,  18,  10,   0,   0,      0,0,0,0,0,0,0,0, 
	0,   0,  10,  18,  18,  10,   0,   0,      0,0,0,0,0,0,0,0, 
	0,   0,   6,  10,  10,   6,   0,   0,      0,0,0,0,0,0,0,0, 
	0,   0,   0,   0,   0,   0,   0,   0,      0,0,0,0,0,0,0,0 
    },
    {    
	0,   0,   0,   0,   0,   0,   0,   0,      0,0,0,0,0,0,0,0, 
	0,   0,   6,  10,  10,   6,   0,   0,      0,0,0,0,0,0,0,0, 
	0,   0,  10,  18,  18,  10,   0,   0,      0,0,0,0,0,0,0,0, 
	0,   0,  10,  18,  18,  10,   0,   0,      0,0,0,0,0,0,0,0, 
	0,   0,   0,   8,   8,   0,   0,   0,      0,0,0,0,0,0,0,0, 
	0,   0,   0,   0,   0,   0,   0,   0,      0,0,0,0,0,0,0,0, 
	0,   0,   0,   0,   0,   0,   0,   0,      0,0,0,0,0,0,0,0, 
	0,   0,   0,   0,   0,   0,   0,   0,      0,0,0,0,0,0,0,0 
    }
};

int pawn_rank[2][128] = {
    {
	0,   0,   0,   0,   0,   0,   0,   0,      0,0,0,0,0,0,0,0, 
	1,   1,   1,   1,   1,   1,   1,   1,      0,0,0,0,0,0,0,0, 
	2,   2,   2,   2,   2,   2,   2,   2,      0,0,0,0,0,0,0,0, 
	3,   3,   3,   3,   3,   3,   3,   3,      0,0,0,0,0,0,0,0, 
	4,   4,   4,   4,   4,   4,   4,   4,      0,0,0,0,0,0,0,0, 
	5,   5,   5,   5,   5,   5,   5,   5,      0,0,0,0,0,0,0,0, 
	6,   6,   6,   6,   6,   6,   6,   6,      0,0,0,0,0,0,0,0, 
	7,   7,   7,   7,   7,   7,   7,   7,      0,0,0,0,0,0,0,0 
    },
    {    
	7,   7,   7,   7,   7,   7,   7,   7,      0,0,0,0,0,0,0,0, 
	6,   6,   6,   6,   6,   6,   6,   6,      0,0,0,0,0,0,0,0, 
	5,   5,   5,   5,   5,   5,   5,   5,      0,0,0,0,0,0,0,0, 
	4,   4,   4,   4,   4,   4,   4,   4,      0,0,0,0,0,0,0,0, 
	3,   3,   3,   3,   3,   3,   3,   3,      0,0,0,0,0,0,0,0, 
	2,   2,   2,   2,   2,   2,   2,   2,      0,0,0,0,0,0,0,0, 
	1,   1,   1,   1,   1,   1,   1,   1,      0,0,0,0,0,0,0,0, 
	0,   0,   0,   0,   0,   0,   0,   0,      0,0,0,0,0,0,0,0 
    }
};


int king_exposed[2][128] = {
    {    1,   0,   0,   0,   0,   0,   0,   1,      0,0,0,0,0,0,0,0, 
         1,   1,   1,   1,   1,   1,   1,   1,      0,0,0,0,0,0,0,0,
         2,   2,   2,   2,   2,   2,   2,   2,      0,0,0,0,0,0,0,0,
         3,   3,   3,   3,   3,   3,   3,   3,      0,0,0,0,0,0,0,0,
         3,   3,   3,   3,   3,   3,   3,   3,      0,0,0,0,0,0,0,0,
         3,   3,   3,   3,   3,   3,   3,   3,      0,0,0,0,0,0,0,0,
         3,   3,   3,   3,   3,   3,   3,   3,      0,0,0,0,0,0,0,0,
         3,   3,   3,   3,   3,   3,   3,   3,      0,0,0,0,0,0,0,0 },

    {    3,   3,   3,   3,   3,   3,   3,   3,      0,0,0,0,0,0,0,0,
         3,   3,   3,   3,   3,   3,   3,   3,      0,0,0,0,0,0,0,0,
         3,   3,   3,   3,   3,   3,   3,   3,      0,0,0,0,0,0,0,0,
         3,   3,   3,   3,   3,   3,   3,   3,      0,0,0,0,0,0,0,0,
         3,   3,   3,   3,   3,   3,   3,   3,      0,0,0,0,0,0,0,0,
         2,   2,   2,   2,   2,   2,   2,   2,      0,0,0,0,0,0,0,0,
         1,   1,   1,   1,   1,   1,   1,   1,      0,0,0,0,0,0,0,0,
         1,   0,   0,   0,   0,   0,   0,   1,      0,0,0,0,0,0,0,0 }
};
int kingstorm_tab[2][128] = { 
    {    0,   0,   0,   0,   0,   0,   0,   0,      0,0,0,0,0,0,0,0,
        10,  10,  10,   0,   0, -12, -12, -12,      0,0,0,0,0,0,0,0,
         8,   8,   8,   6,   0,  -9,  -9,  -9,      0,0,0,0,0,0,0,0,
         4,   4,   4,  10,   6,  -6,  -6,  -6,      0,0,0,0,0,0,0,0,
         0,   0,   0,   4,  10,  -3,  -3,  -3,      0,0,0,0,0,0,0,0,
         0,   0,   0,   0,   0,   5,   0,   0,      0,0,0,0,0,0,0,0,
         0,   0,   0,   0,   0,   0,   0,   0,      0,0,0,0,0,0,0,0,
         0,   0,   0,   0,   0,   0,   0,   0,      0,0,0,0,0,0,0,0 },

    {    0,   0,   0,   0,   0,   0,   0,   0,      0,0,0,0,0,0,0,0,
         0,   0,   0,   0,   0,   0,   0,   0,      0,0,0,0,0,0,0,0,
         0,   0,   0,   0,   0,   5,   0,   0,      0,0,0,0,0,0,0,0,
         0,   0,   0,   4,  10,  -3,  -3,  -3,      0,0,0,0,0,0,0,0,
         4,   4,   4,  10,   6,  -6,  -6,  -6,      0,0,0,0,0,0,0,0,
         8,   8,   8,   6,   0,  -9,  -9,  -9,      0,0,0,0,0,0,0,0,
        10,  10,  10,   0,   0, -12, -12, -12,      0,0,0,0,0,0,0,0,
         0,   0,   0,   0,   0,   0,   0,   0,      0,0,0,0,0,0,0,0 }
};
int queenstorm_tab[2][128] = {
    {    0,   0,   0,   0,   0,   0,   0,   0,      0,0,0,0,0,0,0,0,
        -12, -12, -12,  0,   0,   9,  10,  10,      0,0,0,0,0,0,0,0,
        -9,  -9,  -9,   0,   6,   8,   8,   8,      0,0,0,0,0,0,0,0,
        -6,  -6,  -6,   6,  10,   4,   4,   4,      0,0,0,0,0,0,0,0,
        -3,  -3,  -3,  10,   4,   0,   0,   0,      0,0,0,0,0,0,0,0,
         0,   0,   5,   0,   0,   0,   0,   0,      0,0,0,0,0,0,0,0,
         0,   0,   0,   0,   0,   0,   0,   0,      0,0,0,0,0,0,0,0,
         0,   0,   0,   0,   0,   0,   0,   0,      0,0,0,0,0,0,0,0 },

    {    0,   0,   0,   0,   0,   0,   0,   0,      0,0,0,0,0,0,0,0,
         0,   0,   0,   0,   0,   0,   0,   0,      0,0,0,0,0,0,0,0,
         0,   0,   5,   0,   0,   0,   0,   0,      0,0,0,0,0,0,0,0,
        -3,  -3,  -3,  10,   4,   0,   0,   0,      0,0,0,0,0,0,0,0,
        -6,  -6,  -6,   6,  10,   4,   4,   4,      0,0,0,0,0,0,0,0,
        -9,  -9,  -9,   0,   6,   8,   8,   8,      0,0,0,0,0,0,0,0,
        -12, -12, -12,  0,   0,   9,  10,  10,      0,0,0,0,0,0,0,0,
         0,   0,   0,   0,   0,   0,   0,   0,      0,0,0,0,0,0,0,0 }
};

int pawn_tab[2][128] = {
    {    0,   0,   0,   0,   0,   0,   0,   0,      0,0,0,0,0,0,0,0,
        -4,  -2 , -2, -10, -10,  -2,  -2,  -4,      0,0,0,0,0,0,0,0,
        -4,  -1,   1,   0,   0,   1,  -1,  -4,      0,0,0,0,0,0,0,0,
        -3,   1,   9,  15,  15,   9,   1,  -3,      0,0,0,0,0,0,0,0,
        -1,   2,   4,   8,   8,   4,   2,  -1,      0,0,0,0,0,0,0,0,
         0,   3,   5,   8,   8,   5,   3,   0,      0,0,0,0,0,0,0,0,
         2,   5,   7,  10,  10,   7,   5,   2,      0,0,0,0,0,0,0,0,
         0,   0,   0,   0,   0,   0,   0,   0,      0,0,0,0,0,0,0,0, },
    {
         0,   0,   0,   0,   0,   0,   0,   0,      0,0,0,0,0,0,0,0,
         2,   5,   7,  10,  10,   7,   5,   2,      0,0,0,0,0,0,0,0,
         0,   3,   5,   8,   8,   5,   3,   0,      0,0,0,0,0,0,0,0,
        -1,   2,   4,   8,   8,   4,   2,  -1,      0,0,0,0,0,0,0,0,
        -3,   1,   9,  15,  15,   9,   1,  -3,      0,0,0,0,0,0,0,0,
        -4,  -1,   1,   0,   0,   1,  -1,  -4,      0,0,0,0,0,0,0,0,
        -4,  -2 , -2, -10, -10,  -2,  -2,  -4,      0,0,0,0,0,0,0,0,
         0,   0,   0,   0,   0,   0,   0,   0,      0,0,0,0,0,0,0,0 }
};
int pawn_end_tab[2][128] = {
    {    
         0,   0,   0,   0,   0,   0,   0,   0,      0,0,0,0,0,0,0,0,
        -8,  -8,  -8,  -8,  -8,  -8,  -8,  -8,      0,0,0,0,0,0,0,0,
        -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,      0,0,0,0,0,0,0,0,
         0,   0,   0,   0,   0,   0,   0,   0,      0,0,0,0,0,0,0,0,
         4,   4,   4,   4,   4,   4,   4,   4,      0,0,0,0,0,0,0,0,
         8,   8,   8,   8,   8,   8,   8,   8,      0,0,0,0,0,0,0,0,
        12,  12,  12,  12,  12,  12,  12,  12,      0,0,0,0,0,0,0,0,
         0,   0,   0,   0,   0,   0,   0,   0,      0,0,0,0,0,0,0,0
    },
    {
         0,   0,   0,   0,   0,   0,   0,   0,      0,0,0,0,0,0,0,0,
        12,  12,  12,  12,  12,  12,  12,  12,      0,0,0,0,0,0,0,0,
         8,   8,   8,   8,   8,   8,   8,   8,      0,0,0,0,0,0,0,0,
         4,   4,   4,   4,   4,   4,   4,   4,      0,0,0,0,0,0,0,0,
         0,   0,   0,   0,   0,   0,   0,   0,      0,0,0,0,0,0,0,0,
        -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,      0,0,0,0,0,0,0,0,
        -8,  -8,  -8,  -8,  -8,  -8,  -8,  -8,      0,0,0,0,0,0,0,0,
         0,   0,   0,   0,   0,   0,   0,   0,      0,0,0,0,0,0,0,0 
    }
};

int knight_tab[2][128] = {
    {
       -20, -15, -10, -10, -10, -10, -15, -20,      0,0,0,0,0,0,0,0,
       -15,  -5,   0,   2,   2,   0,  -2, -15,      0,0,0,0,0,0,0,0,
       -10,   0,   5,   5,   5,   5,   0, -10,      0,0,0,0,0,0,0,0,
       -10,   0,   6,  10,  10,   6,   0, -10,      0,0,0,0,0,0,0,0,
       -10,   0,   6,  10,  10,   6,   0, -10,      0,0,0,0,0,0,0,0,
       -10,   0,   8,   8,   8,   8,   0, -10,      0,0,0,0,0,0,0,0,
       -15,  -5,   2,   2,   2,   0,  -5, -15,      0,0,0,0,0,0,0,0,
       -20, -15, -10, -10, -10, -10, -15, -20,      0,0,0,0,0,0,0,0
    },
    {
       -20, -15, -10, -10, -10, -10, -15, -20,      0,0,0,0,0,0,0,0,
       -15,  -5,   2,   2,   2,   0,  -5, -15,      0,0,0,0,0,0,0,0,
       -10,   0,   8,   8,   8,   8,   0, -10,      0,0,0,0,0,0,0,0,
       -10,   0,   6,  10,  10,   6,   0, -10,      0,0,0,0,0,0,0,0,
       -10,   0,   6,  10,  10,   6,   0, -10,      0,0,0,0,0,0,0,0,
       -10,   0,   5,   5,   5,   5,   0, -10,      0,0,0,0,0,0,0,0,
       -15,  -5,   0,   2,   2,   0,  -2, -15,      0,0,0,0,0,0,0,0,
       -20, -15, -10, -10, -10, -10, -15, -20,      0,0,0,0,0,0,0,0 }
};
int bishop_tab[2][128] = {
    {
         2,  -1,  -2,  -5,  -5,  -2,  -1,   2,      0,0,0,0,0,0,0,0,
         0,   7,   2,   2,   2,   2,   7,   0,      0,0,0,0,0,0,0,0,
         2,   4,   8,   6,   6,   8,   4,   2,      0,0,0,0,0,0,0,0,
         2,   6,   8,  10,  10,   8,   6,   2,      0,0,0,0,0,0,0,0,
         2,   8,   8,  12,  12,   8,   8,   2,      0,0,0,0,0,0,0,0,
         2,   5,   8,   8,   8,   8,   5,   2,      0,0,0,0,0,0,0,0,
         0,   4,   3,   2,   2,   3,   4,   0,      0,0,0,0,0,0,0,0,
         0,   2,   0,   0,   0,   0,   2,   0,      0,0,0,0,0,0,0,0
    },
    { 
         0,   2,   0,   0,   0,   0,   2,   0,      0,0,0,0,0,0,0,0,
         0,   4,   3,   2,   2,   3,   4,   0,      0,0,0,0,0,0,0,0,
         2,   5,   8,   8,   8,   8,   5,   2,      0,0,0,0,0,0,0,0,
         2,   8,   8,  12,  12,   8,   8,   2,      0,0,0,0,0,0,0,0,
         2,   6,   8,  10,  10,   8,   6,   2,      0,0,0,0,0,0,0,0,
         2,   4,   8,   6,   6,   8,   4,   2,      0,0,0,0,0,0,0,0,
         0,   7,   2,   2,   2,   2,   7,   0,      0,0,0,0,0,0,0,0,
         2,  -1,  -2,  -5,  -5,  -2,  -1,   2,      0,0,0,0,0,0,0,0 
    }
};

int rook_tab[2][128] = {
    {   -1,   1,   3,   5,   5,   3,   1,  -1,      0,0,0,0,0,0,0,0,
        -1,   1,   3,   5,   5,   3,   1,  -1,      0,0,0,0,0,0,0,0,
        -1,   1,   3,   5,   5,   3,   1,  -1,      0,0,0,0,0,0,0,0,
        -1,   1,   3,   5,   5,   3,   1,  -1,      0,0,0,0,0,0,0,0,
        -1,   1,   3,   5,   5,   3,   1,  -1,      0,0,0,0,0,0,0,0,
         4,   6,   8,  10,  10,   8,   6,   6,      0,0,0,0,0,0,0,0,
         8,  10,  12,  14,  14,  12,  10,   8,      0,0,0,0,0,0,0,0,
         4,   6,   8,  10,  10,   8,   6,   4,      0,0,0,0,0,0,0,0 },
    {
         4,   6,   8,  10,  10,   8,   6,   4,      0,0,0,0,0,0,0,0,
         8,  10,  12,  14,  14,  12,  10,   8,      0,0,0,0,0,0,0,0,
         4,   6,   8,  10,  10,   8,   6,   6,      0,0,0,0,0,0,0,0,
        -1,   1,   3,   5,   5,   3,   1,  -1,      0,0,0,0,0,0,0,0,
        -1,   1,   3,   5,   5,   3,   1,  -1,      0,0,0,0,0,0,0,0,
        -1,   1,   3,   5,   5,   3,   1,  -1,      0,0,0,0,0,0,0,0,
        -1,   1,   3,   5,   5,   3,   1,  -1,      0,0,0,0,0,0,0,0,
        -1,   1,   3,   5,   5,   3,   1,  -1,      0,0,0,0,0,0,0,0 }
};
int queen_tab[2][128] = {
    {
        -5,  -4,  -3,   0,   0,  -3,  -4,  -5,      0,0,0,0,0,0,0,0,
        -2,   0,   2,   3,   3,   2,   0,  -2,      0,0,0,0,0,0,0,0,
         0,   1,   4,   4,   4,   4,   1,   0,      0,0,0,0,0,0,0,0,
         0,   2,   4,   8,   8,   4,   2,   0,      0,0,0,0,0,0,0,0,
         1,   3,   6,  10,  10,   6,   3,   1,      0,0,0,0,0,0,0,0,
         1,   4,   6,  12,  12,   6,   4,   1,      0,0,0,0,0,0,0,0,
         2,   4,   8,  12,  12,   8,   4,   2,      0,0,0,0,0,0,0,0,
        -2,   1,   2,   3,   3,   2,   1,  -2,      0,0,0,0,0,0,0,0
    },
    {
        -2,   1,   2,   3,   3,   2,   1,  -2,      0,0,0,0,0,0,0,0,
         2,   4,   8,  12,  12,   8,   4,   2,      0,0,0,0,0,0,0,0,
         1,   4,   6,  12,  12,   6,   4,   1,      0,0,0,0,0,0,0,0,
         1,   3,   6,  10,  10,   6,   3,   1,      0,0,0,0,0,0,0,0,
         0,   2,   4,   8,   8,   4,   2,   0,      0,0,0,0,0,0,0,0,
         0,   1,   4,   4,   4,   4,   1,   0,      0,0,0,0,0,0,0,0,
        -2,   0,   2,   3,   3,   2,   0,  -2,      0,0,0,0,0,0,0,0,
        -5,  -4,  -3,   0,   0,  -3,  -4,  -5,      0,0,0,0,0,0,0,0 }
};
int king_tab[2][128] = {
    {    3,   5,   4,  -5,  -5,  -2,   4,   2,      0,0,0,0,0,0,0,0,
        -1,   0, -10, -10, -10 ,-10,   0,  -1,      0,0,0,0,0,0,0,0,
       -10, -10, -10, -10, -10, -10, -10, -10,      0,0,0,0,0,0,0,0,
       -20, -20, -20, -20, -20, -20, -20, -20,      0,0,0,0,0,0,0,0,
       -20, -20, -20, -20, -20, -20, -20, -20,      0,0,0,0,0,0,0,0,
       -20, -20, -20, -20, -20, -20, -20, -20,      0,0,0,0,0,0,0,0,
       -20, -20, -20, -20, -20, -20, -20, -20,      0,0,0,0,0,0,0,0,
       -20, -20, -20, -20, -20, -20, -20, -20,      0,0,0,0,0,0,0,0, },
    { 
       -20, -20, -20, -20, -20, -20, -20, -20,      0,0,0,0,0,0,0,0,
       -20, -20, -20, -20, -20, -20, -20, -20,      0,0,0,0,0,0,0,0,
       -20, -20, -20, -20, -20, -20, -20, -20,      0,0,0,0,0,0,0,0,
       -20, -20, -20, -20, -20, -20, -20, -20,      0,0,0,0,0,0,0,0,
       -20, -20, -20, -20, -20, -20, -20, -20,      0,0,0,0,0,0,0,0,
       -10, -10, -10, -10, -10, -10, -10, -10,      0,0,0,0,0,0,0,0,
        -1,   0, -10, -10, -10 ,-10,   0,  -1,      0,0,0,0,0,0,0,0,
         3,   4,   4,  -5,  -5,  -2,   4,   2,      0,0,0,0,0,0,0,0 }
};
int king_end_tab[2][128] = {
    { 
       -20, -15, -10, -10, -10, -10, -15, -20,      0,0,0,0,0,0,0,0,
       -15,  -5,   0,   0,   0,   0,  -5, -11,      0,0,0,0,0,0,0,0,
       -15,   0,   5,   5,   5,   5,   0, -15,      0,0,0,0,0,0,0,0,
	-8,   0,   5,  12,  12,   5,   0,  -8,      0,0,0,0,0,0,0,0,
	-8,   0,   5,  12,  12,   5,   0,  -8,      0,0,0,0,0,0,0,0,
       -15,   0,   5,   5,   5,   5,   0, -15,      0,0,0,0,0,0,0,0,
       -15,  -5,   0,   0,   0,   0,  -5, -11,      0,0,0,0,0,0,0,0,
       -20, -15, -10, -10, -10, -10, -15, -20,      0,0,0,0,0,0,0,0 },
    { 
       -20, -15, -10, -10, -10, -10, -15, -20,      0,0,0,0,0,0,0,0,
       -15,  -5,   0,   0,   0,   0,  -5, -11,      0,0,0,0,0,0,0,0,
       -15,   0,   5,   5,   5,   5,   0, -15,      0,0,0,0,0,0,0,0,
	-8,   0,   5,  12,  12,   5,   0,  -8,      0,0,0,0,0,0,0,0,
	-8,   0,   5,  12,  12,   5,   0,  -8,      0,0,0,0,0,0,0,0,
       -15,   0,   5,   5,   5,   5,   0, -15,      0,0,0,0,0,0,0,0,
       -15,  -5,   0,   0,   0,   0,  -5, -11,      0,0,0,0,0,0,0,0,
       -20, -15, -10, -10, -10, -10, -15, -20,      0,0,0,0,0,0,0,0 }
};

//-------------------------------------------------------------------
// Hash Keys
//-------------------------------------------------------------------
HashKey zobrist_side[2] = {0x2c09389704482d4fULL, 0x6ca9f7823a686c4cULL};

HashKey zobrist_castle[16] =
{
        0x7902cc016099d570ULL, 0x47f06b275c171da0ULL, 0x2f0f5e85685de4e1ULL, 0x70ff7fe54f703da5ULL, 
        0x781e012d7607dfc1ULL, 0x3eebb295353cd184ULL, 0x7c15f9452dbc0819ULL, 0x44968ee1529665cfULL, 
        0x6a39d8f46bfb10d4ULL, 0x710941cd578e502eULL, 0x0482480601f21aacULL, 0x5899cc384a085e1fULL, 
        0x4f5b805c2ca4e8a4ULL, 0x0b004fc87b64b8f3ULL, 0x30ed15f377aa474aULL, 0x35cd254029efe1f5ULL 
};

HashKey zobrist_board[15][128] =
{
	{
        0x58441cbb7dbd9067ULL, 0x0606ff9507537b40ULL, 0x661b754877067f7aULL, 0x56c3b8e55e397676ULL, 
        0x6d0e5f3b15af6b7aULL, 0x137647fa69245881ULL, 0x436b7393580cd6dcULL, 0x3bbabe502da54c88ULL, 
        0x4407e7b02cc4001dULL, 0x05339cb6488a2fb7ULL, 0x2eb61ac95dcd68efULL, 0x12928dd67e119b26ULL, 
        0x0a7251931d92dd9eULL, 0x797654193b5f6787ULL, 0x153d24e92f437959ULL, 0x654f497c6d8141a4ULL, 
        0x2d0109c06b564911ULL, 0x74d4bce4131c7f09ULL, 0x625cc88c4b9875c9ULL, 0x7155f57f4f6b27c7ULL, 
        0x6147e14404cc3d79ULL, 0x388f804824b354d7ULL, 0x5cd91455744a3e98ULL, 0x5258a15f20e0fc06ULL, 
        0x210e3eb6578c3e16ULL, 0x696b2bbd4fc4597fULL, 0x3559a7057bfdb993ULL, 0x4dd5f4a53fcbf898ULL, 
        0x19909732474c48bfULL, 0x7b2b601f2ecdbc1bULL, 0x768fc218607aa99bULL, 0x1c4efdbf2390cbd9ULL, 
        0x4bd0f2ad1123baa3ULL, 0x36ad4ae22e2dbb39ULL, 0x5cbc306c28034061ULL, 0x7d98e3003e0411b0ULL, 
        0x2ccf7dda36286349ULL, 0x62b7668809a89230ULL, 0x2a72a1e1351007e7ULL, 0x2a898e364b80e097ULL, 
        0x0c9c45fd13f4b9f3ULL, 0x1b453a1741f5ed02ULL, 0x0ff27386691b2ebcULL, 0x01c1e59b29830ab8ULL, 
        0x3067777b7ced45baULL, 0x5850c6d326f73994ULL, 0x5d67ef56749fc492ULL, 0x4a88056d2938e203ULL, 
        0x05c37f350135504fULL, 0x57669d3c627fafa2ULL, 0x293890b054ff803cULL, 0x2083c15256080e8aULL, 
        0x0b27e385033b27daULL, 0x5fb0a0ba359a8567ULL, 0x384b2fc20a3a2ef0ULL, 0x011b65fe44e775bfULL, 
        0x1e2ee8e31c60a015ULL, 0x06dd62c22e215c6aULL, 0x057bced2089f485dULL, 0x57a4672235e3464dULL, 
        0x058c8e172ff52df6ULL, 0x5cda7fe162f47d6dULL, 0x2494f2882762854eULL, 0x0c2d5f702a5871beULL, 
        0x2897d59d6393fcacULL, 0x0cd8216051d0664dULL, 0x38937ce92d5be2b2ULL, 0x27d874d843bb606eULL, 
        0x30970a8d07891592ULL, 0x7955e5d568e23a4fULL, 0x11c344837a714bd4ULL, 0x2dc9b00e2ff22d66ULL, 
        0x16d1ebe934a712d0ULL, 0x5e1389d01c4dbabbULL, 0x3d465b2d35b7f0f3ULL, 0x5231010942d2e945ULL, 
        0x65ad1ee92f0b80eaULL, 0x25c766b20a421171ULL, 0x566e063931f4c623ULL, 0x349a832f7f05dbd6ULL, 
        0x1588c2cf4172a48fULL, 0x50d642244e1c3fb8ULL, 0x6ece874278aeb6fcULL, 0x11d7a0271f6591cfULL, 
        0x0037cc8e0b2d85fcULL, 0x0847cc1e11fb1111ULL, 0x059ed1d036117c2cULL, 0x41ed3e781c70bdbaULL, 
        0x6ab88efd2000c848ULL, 0x38be787527feea2aULL, 0x55b8b93b0aef797eULL, 0x6ad1d36f3b65d824ULL, 
        0x39fafa6910993a22ULL, 0x45a7e996106900a2ULL, 0x428e00457a426cc5ULL, 0x0f6edc785816c314ULL, 
        0x3bb5115560451e9cULL, 0x263302cd2a839897ULL, 0x58f3d598380aa2f4ULL, 0x49e92a66592ba227ULL, 
        0x433828f05230f684ULL, 0x6b26b33848d6fac1ULL, 0x084272b02d13f1b0ULL, 0x6547b87b72fb01adULL, 
        0x4d14b9f91e0630f0ULL, 0x1af9ebd822cd7334ULL, 0x28f5aa6f05cbbf47ULL, 0x5e334b5962f0a4d8ULL, 
        0x1664f96923db34efULL, 0x7359a57a58f2f9aeULL, 0x1e1da1b402c881f2ULL, 0x3109bcc359d2b309ULL, 
        0x630da08f573cbf90ULL, 0x04564ba03c017627ULL, 0x0f4762844e3f7606ULL, 0x152d184e527f8b74ULL, 
        0x20706c8a0053cb87ULL, 0x1b56863528b2df3bULL, 0x2d67bd37009e3eb0ULL, 0x1bade0e87a7c7730ULL, 
        0x1ea46fa136a7ccc0ULL, 0x1d49ea65479a1a10ULL, 0x3c738c087b7d35beULL, 0x2a8abee852d88571ULL, 
        0x1f586aad1de46462ULL, 0x2bcb7f203d760c61ULL, 0x20ace6545cd53be3ULL, 0x1748bf6b03ba86e3ULL
	},

	{
        0x3411fb731b9f0b0bULL, 0x3fbbfd0b43595df7ULL, 0x69de811254e91559ULL, 0x15d8e96b0a4eed9cULL, 
        0x553ce0e0312f6fa1ULL, 0x3301ccd702a49e18ULL, 0x31cdae514eafadc0ULL, 0x7d21154850721df2ULL, 
        0x05577a801a6affadULL, 0x180c380241cb0688ULL, 0x15e8356b4296f6eaULL, 0x14a38bfa3540a018ULL, 
        0x607b5b4c406f0b1aULL, 0x72b6ac7a012841a1ULL, 0x1d4446fd09ff6be5ULL, 0x04e2c88451564270ULL, 
        0x259e76f0449ec58fULL, 0x14afa0670f7cf802ULL, 0x1987dae92a8889d2ULL, 0x19cbe59f6ec4bbc9ULL, 
        0x5bb7f9734ccdb276ULL, 0x716959e10d85a7c5ULL, 0x1b7d60366e8a6f2aULL, 0x5df7c5b720d4dab7ULL, 
        0x08f56ed77603fdbaULL, 0x629fe13f1edda443ULL, 0x389af4a477436d39ULL, 0x541e445b19164ff1ULL, 
        0x37b2785346d4f0d5ULL, 0x1a3e919254f6bf50ULL, 0x50d45cba1f215a16ULL, 0x264d01c07672d3abULL, 
        0x63c01fa63afca227ULL, 0x05efcbad7d47fa8fULL, 0x65852bfa1fbbb14cULL, 0x6c0cb658413d256dULL, 
        0x6c8963c35d76103aULL, 0x4ec2cd320806c3f9ULL, 0x4c007f642cba92eaULL, 0x28db9eb054f5ee3bULL, 
        0x22be90a40b7b7ff0ULL, 0x73d3927e5b598548ULL, 0x02beed2947f1d6daULL, 0x746fd5393a71657dULL, 
        0x0ec6c7af0eae66cbULL, 0x0f6824cd5f9b246aULL, 0x2dcfc0e235b5268eULL, 0x560df815118fe088ULL, 
        0x70b1c8b55bfdc3c2ULL, 0x0ed7db175636f4afULL, 0x7bb9750f7ae4916fULL, 0x17741a1d6842d8d2ULL, 
        0x585aa1a96636e74fULL, 0x70499ccb245b210dULL, 0x12f17a3919253b7cULL, 0x79510f4935b00addULL, 
        0x24a0bb6c6d24a1c7ULL, 0x11099026275fa895ULL, 0x351678a10579655fULL, 0x61d10e1243dd4051ULL, 
        0x1427cc2b713932e0ULL, 0x237864bb41f78d0dULL, 0x26ee596e79865cd0ULL, 0x53876d9517a02223ULL, 
        0x55842092625f48acULL, 0x6dd716d3513d95a1ULL, 0x5d43da1b054b30f0ULL, 0x39806e73359e7bc5ULL, 
        0x6b82183f29ca0b3fULL, 0x59f99cd27e739279ULL, 0x42ef46bb534aac1bULL, 0x34239d5667900227ULL, 
        0x406f4de3452d2d7cULL, 0x0eefaabc7585c684ULL, 0x4aa692dc70c0b8cfULL, 0x396306d55ece5f07ULL, 
        0x61f9ebaf5cdb6b90ULL, 0x20c5ec1408e8451dULL, 0x5661c860744d59a9ULL, 0x208867402be5e8f3ULL, 
        0x56aca2550e5f7e13ULL, 0x7d237e9433f07c70ULL, 0x13aaaf0336a3ed08ULL, 0x698ef8357f2cc743ULL, 
        0x606df84743889508ULL, 0x7da059bc235d3f02ULL, 0x16d3412331c3f712ULL, 0x0aed412957428f06ULL, 
        0x76f1248f19dcebe5ULL, 0x4cc8558b4197b76bULL, 0x0a9da4b4062b5c60ULL, 0x206616726c979063ULL, 
        0x6306c7f1412c0286ULL, 0x757fd58039689051ULL, 0x35795c2f16083cc1ULL, 0x654e79440c25fe84ULL, 
        0x2467bad46271f7d9ULL, 0x40167af4381269d8ULL, 0x1915e4e129a5732aULL, 0x373f311b7983dd28ULL, 
        0x6d2e083234df8ad7ULL, 0x1ce11c2a04014955ULL, 0x66a381e927ce5d53ULL, 0x5b43d85c5d94a678ULL, 
        0x41ab4938280c2de7ULL, 0x1f2c5de34c48ededULL, 0x2e378a473f927455ULL, 0x38e07e50113e5238ULL, 
        0x00be76db2e6053d1ULL, 0x4aa6e28a3637d30aULL, 0x446890922ff55bceULL, 0x425dd18e68d04b66ULL, 
        0x126753a702744c83ULL, 0x20e2b53e2b7d3888ULL, 0x2c19bfad5821e659ULL, 0x250115b01947c7dfULL, 
        0x0d01713041e231daULL, 0x1d49113473a4f31aULL, 0x69b08f2d788ce990ULL, 0x513999922b5bd866ULL, 
        0x209917777065f776ULL, 0x77a4c6534ed0a1bfULL, 0x2ff86bcb308544a3ULL, 0x600ef3f730b6e2a7ULL, 
        0x5ee598742ab5d681ULL, 0x66eeb5b1234e2906ULL, 0x5aab3250294c8740ULL, 0x0c1e746d6d1285f7ULL
	},

	{
        0x2bc0d3c32d0129abULL, 0x188fbe8057da9370ULL, 0x052310053d90d430ULL, 0x71225b4f12248135ULL, 
        0x7f73060b0e6b6c83ULL, 0x05c9744f69239538ULL, 0x06f8561457030de2ULL, 0x147f6d9e27916d8bULL, 
        0x476905580c2433f1ULL, 0x76620f4a77617123ULL, 0x3ca9789556710342ULL, 0x281853ca1b8f1109ULL, 
        0x0126d9c30f07097cULL, 0x3edd3a105bd20c13ULL, 0x385390bc4afbae7dULL, 0x48e4920b6414647fULL, 
        0x77fcd8286174508bULL, 0x3beef7ef7d1fe82dULL, 0x1f0524bb2d11533eULL, 0x0f4469631e782ac6ULL, 
        0x3b7cbfc1150dddb2ULL, 0x079bbfff427515d5ULL, 0x6c10eb941c1b2d9dULL, 0x6a0683613379f0ecULL, 
        0x283f618f606892abULL, 0x2adb621064e8da24ULL, 0x36d995ed52f3b5daULL, 0x0077eb2d38006fb1ULL, 
        0x61fabf563f55253dULL, 0x13d27bc41a4e5012ULL, 0x0a50d3ba5cb70dcfULL, 0x7e62b491024dabe3ULL, 
        0x3e2b5e5a3a51ac80ULL, 0x7f6d94105d308316ULL, 0x6762ffbe0eb1fd73ULL, 0x7ba8addc22dfbf80ULL, 
        0x23bfdb2603446ddbULL, 0x6554d5550fd0c6baULL, 0x1f5f9b794f5b58b6ULL, 0x434ab7a7479efd08ULL, 
        0x2fc3eb626e2619b7ULL, 0x2c87d72c669d814fULL, 0x4119cf912cffc259ULL, 0x1e9df10023148ee8ULL, 
        0x6c54e79732706cc5ULL, 0x3d62defa76a5bb51ULL, 0x0f277a943bc5938cULL, 0x78f367344d52d8efULL, 
        0x7617400c7860fb45ULL, 0x2a835c055d7a3fcbULL, 0x0712f8b8262c09e1ULL, 0x0059ff4b2ad2d3deULL, 
        0x297077bd65aed4a0ULL, 0x3aa39a9948d01336ULL, 0x350a2d577dee5240ULL, 0x106f103e64ce18b9ULL, 
        0x6c146bf73cf6e76aULL, 0x4b6b9a082d2e3b88ULL, 0x69f6a9c36a098b09ULL, 0x5042ca70564b915aULL, 
        0x1c79f7ce0da5a96bULL, 0x4cf14cac2ba17262ULL, 0x496b3cf745e4b3e0ULL, 0x78f44b513f827d03ULL, 
        0x3e45af252377a756ULL, 0x1cfcbcce4558a7deULL, 0x49a3b1381d56bc19ULL, 0x702b7bbc731428f5ULL, 
        0x030590ba2acf1655ULL, 0x3be43c2b380fbe11ULL, 0x28bd68954c534c69ULL, 0x1cddd6ca14d1d48cULL, 
        0x094a33d3684970d2ULL, 0x420010157340dd96ULL, 0x5252fbdb1242da85ULL, 0x498c6ef16eccf3a9ULL, 
        0x1fe883f0167dbb9dULL, 0x1a6e660c6953c0e7ULL, 0x5c626f7d1362b15dULL, 0x28d63deb1aa81ea3ULL, 
        0x36da58b445d2fab9ULL, 0x6000c681007e09ecULL, 0x6329b6d3502c423dULL, 0x739232e1662f478dULL, 
        0x7afb58932f766f0cULL, 0x1e3f059e23b8c128ULL, 0x7bc9bb753b1cdc68ULL, 0x388a95b50513ef48ULL, 
        0x23664d3a7a8aa5caULL, 0x7854ccde75b94916ULL, 0x0ccd804f41e13bcfULL, 0x64863cbf2cb60440ULL, 
        0x585ef76c7ef4a2cbULL, 0x1609c52734c166eaULL, 0x125754293ee00312ULL, 0x4f69858d4931acddULL, 
        0x04b2fdcc2f6a4c0eULL, 0x49afb6c967dcb49fULL, 0x7f968e4b3d41e9aaULL, 0x4e0bfc2c7a91e6deULL, 
        0x6cb858b66c4b01caULL, 0x1e4aa8076882142bULL, 0x2767de3256d53dbcULL, 0x6d9603734ace2b6cULL, 
        0x515fe38665ead051ULL, 0x408774825e2d63d5ULL, 0x27cc0c21250db142ULL, 0x0ae36815002b038dULL, 
        0x2402540d20ed2d3dULL, 0x34ec6a773659a836ULL, 0x5fcd304f0455f004ULL, 0x7f8b551364802e1bULL, 
        0x33c03c12493b0bdcULL, 0x4c5ce2ba3356ca5eULL, 0x067cf5861a68dee6ULL, 0x2de8b13c73354e3cULL, 
        0x06b3e0b04c335943ULL, 0x5bb762672e1bbee2ULL, 0x230896ff494d65daULL, 0x78e9ea4f74687a85ULL, 
        0x2f38362c39715ed1ULL, 0x5295de5b5704424dULL, 0x5e7f10135d794670ULL, 0x572f45da02816421ULL, 
        0x7e6673ad0c1bb052ULL, 0x38db0c575e33a3fdULL, 0x1071a0563866616bULL, 0x42b3d2184431dc69ULL
	},

	{
        0x01a16d470f10b4d3ULL, 0x7788a6c7081e62ceULL, 0x297993b925715803ULL, 0x7b53b10a302d746aULL, 
        0x71a4b147570b1372ULL, 0x5e49334c14ad4846ULL, 0x2058794c57331d9bULL, 0x0915c2cc4f90af78ULL, 
        0x10a47c6d5baba127ULL, 0x2694f1c56f238c80ULL, 0x3924e7977dc437a0ULL, 0x71a4f0a1378b5b45ULL, 
        0x09dfe7f22a7ffcf9ULL, 0x15beff421a518848ULL, 0x62e65e645872d15aULL, 0x5e8364b16487cbabULL, 
        0x6783862d560c0b78ULL, 0x6ca62e7910fd19e7ULL, 0x7b7d637c67f9df84ULL, 0x412a8e516d2214c3ULL, 
        0x3f04f2f61f73c19dULL, 0x01cf5d095f5d6c42ULL, 0x76a6df390ae51fd5ULL, 0x2eee1bbb074b5ba6ULL, 
        0x6690c0fc55830d80ULL, 0x766ee8261fb5a894ULL, 0x534745206813d8c8ULL, 0x574103d95d272d12ULL, 
        0x1293d5c16d00031bULL, 0x7778b55b757a3425ULL, 0x4572d47555fc1a0cULL, 0x5a01ffd02cf65aa3ULL, 
        0x2c08258546a82e4aULL, 0x3df3748a27858901ULL, 0x2ea20dce7f1e02dbULL, 0x14a79dc46da700c4ULL, 
        0x1e91c4781676facdULL, 0x4d046d061538a3b1ULL, 0x215c1aa37bf288c1ULL, 0x1c83ff5707ecdb9fULL, 
        0x5175964212f2e77eULL, 0x27a2843324bcdb62ULL, 0x7b06c0467ee3880cULL, 0x01e408750d9a9607ULL, 
        0x6be38b27795cbdd0ULL, 0x0314ca2c31565f9dULL, 0x4f58d7dc5d16c9fcULL, 0x5e4cba407b60fd61ULL, 
        0x23bef8461c402ecaULL, 0x22e6866252610614ULL, 0x1b5e31a5378e2426ULL, 0x400806d839eff61dULL, 
        0x4e051ef40d0c73dfULL, 0x4f2899cf6f613997ULL, 0x08fefca06bac9926ULL, 0x774e15365a7492e2ULL, 
        0x7e9f80a41ef0996aULL, 0x7f316e4579a640eaULL, 0x1dd42176011576baULL, 0x0740d6f109b7ac9eULL, 
        0x7a72348a0a55a11dULL, 0x3b0e0c3b49cb0c66ULL, 0x676c6b1a195ac67bULL, 0x452c09c80b2b6360ULL, 
        0x359af5456812902aULL, 0x5d8c697550f926eaULL, 0x1fa0b4511d94704dULL, 0x0ae91d076da5d345ULL, 
        0x2aa0e42c5a11b6d6ULL, 0x5d070cdc339fe0cdULL, 0x45be4ffd54552212ULL, 0x0e1473af445dd0a1ULL, 
        0x7345bb7c0d45e1f4ULL, 0x3e04118c1119dcf3ULL, 0x0e5b58ae4544e87dULL, 0x1ad1899108cd8d38ULL, 
        0x4f9a899b55df95ccULL, 0x5298999f3706f4b5ULL, 0x6f3a5c4717c4a367ULL, 0x4232581524d5518cULL, 
        0x7fd733911fbec18aULL, 0x75ce78761f77e7e2ULL, 0x3d5331d800b7957dULL, 0x0d1dbb2767f41604ULL, 
        0x5ac94c546a24c803ULL, 0x1b93f6d120879c51ULL, 0x3e79ea1629a86a81ULL, 0x64e56cf231bfa592ULL, 
        0x36ee4c7522e97e7eULL, 0x42d982854549a524ULL, 0x682e66fc5dab0c16ULL, 0x4e17325c37c8f097ULL, 
        0x338aa1e220afcbfbULL, 0x6ecfe54c22c4fe29ULL, 0x38746f6231023d61ULL, 0x479a4fb5384ba2f4ULL, 
        0x50c0feec3d68c82bULL, 0x57c38ad60e1430c4ULL, 0x3e205da964e145feULL, 0x760846c818e9a9fdULL, 
        0x4f060e01119c3d9aULL, 0x3971464e0d7ff817ULL, 0x3b44a81b1e56b340ULL, 0x3f3f9daa7232f490ULL, 
        0x414031bf0219202fULL, 0x377c99b4296e98bbULL, 0x5fc42c460593cc11ULL, 0x61378952134ece28ULL, 
        0x2643980c50076e9eULL, 0x3613cc525eb8076fULL, 0x0109abff7dae1c07ULL, 0x1703aa6351caaaebULL, 
        0x3b16e4336ec73539ULL, 0x5fdedbaf793741dcULL, 0x53a87b3755e72278ULL, 0x1220ebd922ae8939ULL, 
        0x678360124b923227ULL, 0x302e815022c8082dULL, 0x69e8e5676f6e1efaULL, 0x14fafcbd2b291726ULL, 
        0x71873f2a4c779672ULL, 0x5497afe1514b6b70ULL, 0x520b628335cf3933ULL, 0x649a3998784efa8fULL, 
        0x05d6a7d11aae05eaULL, 0x570701fe06e053d1ULL, 0x185c21f26e0aac61ULL, 0x58aafebc53730625ULL
	},

	{
        0x5cd1e19b3889da6cULL, 0x4caa4801307a5cd2ULL, 0x0e70fce45ecb33daULL, 0x5328e60b75f45cf6ULL, 
        0x2a5d66010357675cULL, 0x18bc652314464b68ULL, 0x72c586562db761e0ULL, 0x3f6f628f644cc580ULL, 
        0x7a2ef85214071270ULL, 0x359830f04c3a5ad5ULL, 0x49d64ba41a326a89ULL, 0x448955654facf375ULL, 
        0x34e070731b905763ULL, 0x568d47464d3c9265ULL, 0x099b03c52f384603ULL, 0x20af988a666ce560ULL, 
        0x67c2206f6d59e08bULL, 0x16e7423276331d53ULL, 0x4c2514656a10283eULL, 0x6c277a4976827a66ULL, 
        0x6d678f9a04e3df6cULL, 0x0ac8c5cf602d15f0ULL, 0x329b414c4a38285eULL, 0x4479db712cca399fULL, 
        0x5e3f3ace7a120c61ULL, 0x7904947428158672ULL, 0x144476ea3d8de9d9ULL, 0x77c279e84924e75eULL, 
        0x591e413d4e4fc12eULL, 0x166179c362b94502ULL, 0x7d8807313711124eULL, 0x49262a62654a27a0ULL, 
        0x246af2d9600d6c94ULL, 0x5b7d44f37090073fULL, 0x4a1d94d247a4bf3cULL, 0x671281a53785246cULL, 
        0x4c889ea871db4774ULL, 0x17b23a5d7f23dff5ULL, 0x3c136fd25c2c15ceULL, 0x2bee19941a52aaa1ULL, 
        0x563e222f24f2ae08ULL, 0x426831136a82991aULL, 0x628097e23a2aaafbULL, 0x33a780783b9ed91fULL, 
        0x087a6c2a4a08fa3bULL, 0x1e581e210602735bULL, 0x011a0c89677e4883ULL, 0x6b4c9afc2584ff63ULL, 
        0x478bb51746c9dfefULL, 0x161506a211a949eaULL, 0x0e6e9f2c7d278847ULL, 0x492e6e565af73dd4ULL, 
        0x6f02cfbc60e0a8b3ULL, 0x5a1b1dc92b163f8eULL, 0x3d0cbe810609375dULL, 0x4568ea2f134ae0b1ULL, 
        0x2afbe56607d11b43ULL, 0x7dcd79cb0d7c7d48ULL, 0x41fbc63e3174fa43ULL, 0x491b56674a763268ULL, 
        0x7b7df47e67737488ULL, 0x5078a5c47c980108ULL, 0x4ef1bd0b3bc540c0ULL, 0x221d006b167d7222ULL, 
        0x028f20af3832070dULL, 0x2826bc0c10fdbfdbULL, 0x35598f5471552a63ULL, 0x6bf4fdb0245c5f10ULL, 
        0x5235d31646101b79ULL, 0x4f729e9f0f429198ULL, 0x4c1952d714db88ceULL, 0x228d72497715383dULL, 
        0x1caca411205aec14ULL, 0x0491b5855ea86a50ULL, 0x51cfe6574dad0becULL, 0x291e9cb84d4ddad5ULL, 
        0x352080747997427cULL, 0x49e5dbdd04123d7fULL, 0x355c833c6c02dc48ULL, 0x1a8fafa137eba3ecULL, 
        0x2434e35542b66baeULL, 0x48e963c7598e72aaULL, 0x340b961134de6177ULL, 0x7dead1ba06416927ULL, 
        0x7aee7cf14d5d7059ULL, 0x1583fabf4707cfc8ULL, 0x6238f92838116d08ULL, 0x3e1d08057ee59d39ULL, 
        0x586c591c42aebd8aULL, 0x5d8e07892a3c3f73ULL, 0x105bc97606aca442ULL, 0x778a1a49457c49eaULL, 
        0x0043e6be416ff626ULL, 0x498e876935a069fbULL, 0x2d72d26f641e370aULL, 0x6d8c0de751a7b5c4ULL, 
        0x26d4a2b8367571aeULL, 0x2b36286e5ae038c9ULL, 0x6b53d3262920fa29ULL, 0x6121a1f166425017ULL, 
        0x767e6a8276a59cb0ULL, 0x2d4a1fdf58b763aaULL, 0x2eb709b96b6727e4ULL, 0x579d00e4072362d5ULL, 
        0x2e15e56e352b086dULL, 0x315fa2493e71aee4ULL, 0x3bd7acaf28e9bc92ULL, 0x03edf8ce3c1b936eULL, 
        0x6a59b2b84d7c8037ULL, 0x71bbfd6917cc8527ULL, 0x319ab7415f480b50ULL, 0x69743aec586f59faULL, 
        0x15bd7cfe14aa635aULL, 0x334f92c301115024ULL, 0x3dcb5d83147134b4ULL, 0x6753a03b3449c806ULL, 
        0x0b16d165149dc01aULL, 0x0d012bb039cddb1eULL, 0x0004e7fe649e2c94ULL, 0x40f13df32e1acd6cULL, 
        0x19c935027250e03cULL, 0x6c8c7c5055a0e1b1ULL, 0x1b3a9cce707a751eULL, 0x11bc751f05944f87ULL, 
        0x3df6f55503787288ULL, 0x1d60d4ae6f91ac97ULL, 0x62c07dd806d50f9aULL, 0x48010691787dfad7ULL
	},

	{
        0x1b7f72f57b509954ULL, 0x798f4afb594ad078ULL, 0x0fc1ce0960e2eb37ULL, 0x0d94987e1ad89f6eULL, 
        0x7580ab511a95c42fULL, 0x54a67a8c75859350ULL, 0x7f33f0c31597b87fULL, 0x23a060bc18fd25c5ULL, 
        0x07e898bc102cdd0dULL, 0x6e9e07772323358aULL, 0x00a7522b005a7c96ULL, 0x28b785113e9e4781ULL, 
        0x03d2ef1f461859c0ULL, 0x2e2ff41866936cf7ULL, 0x4ced695a7630faa9ULL, 0x5f1167ce686cdc4fULL, 
        0x718193fd58a0b2caULL, 0x41b7acc801436206ULL, 0x39839e014f4c4546ULL, 0x1c1c01742f044952ULL, 
        0x69e2097570c27c00ULL, 0x2489dca26915fa39ULL, 0x065a3480482a3d5fULL, 0x02131ffe0e42cd3cULL, 
        0x58571a6c70b12775ULL, 0x316602c658fe6c97ULL, 0x710ba40c5a1d87d8ULL, 0x179cb41874de932bULL, 
        0x2035e19845cca830ULL, 0x5b7200226d234af2ULL, 0x3bfda2d93a8367f1ULL, 0x559027422d7f36d7ULL, 
        0x13241abb1747d40aULL, 0x2ec298dd4ca7b8bcULL, 0x669419504ade9a52ULL, 0x7bac020e507622c6ULL, 
        0x3ba116522035deb1ULL, 0x398c1cff41fb4ad2ULL, 0x68601c103b9f3cfdULL, 0x503e180e40b7367cULL, 
        0x2c50647301a41ad5ULL, 0x19b5a3131d5c087fULL, 0x5bc1a2ad3152572cULL, 0x123a9baa7bf78445ULL, 
        0x771eff5c6dac9bccULL, 0x691acf37331ca236ULL, 0x283003bd3eaaf679ULL, 0x609bd90d3b541e78ULL, 
        0x55f2ca830f5e71eaULL, 0x07fbd7343c86e3d4ULL, 0x5a3d0c3c03a7d943ULL, 0x0cfd069a15de228fULL, 
        0x23ddb7f446892399ULL, 0x57d96d610c3dd404ULL, 0x0228609628178570ULL, 0x4cf50a802e78c509ULL, 
        0x29bba04566aaad93ULL, 0x4bd4cd88057d42f2ULL, 0x17fd04bf5e0f6932ULL, 0x0174c7370f1c041cULL, 
        0x4bbc04ff6a8f966eULL, 0x4238a65273ec08bcULL, 0x293a8ce822d47f5fULL, 0x2f4027357f2d576bULL, 
        0x3232f149373bfe69ULL, 0x3bb43b3f0c6ffd86ULL, 0x3ae3d7ac48b141d9ULL, 0x224e20155ec18fa0ULL, 
        0x0f3a65727a278d76ULL, 0x6aff63a41162c609ULL, 0x223f12e637f46e24ULL, 0x3fdb8b124bfab32bULL, 
        0x1e9f1bb80bb0589bULL, 0x5177f61d369c2077ULL, 0x69bfc1cd52ecbd54ULL, 0x45b82493357bc6ccULL, 
        0x3d7c53c307f0cae5ULL, 0x2967cf8966b6e0abULL, 0x2ac54a4458a7f6beULL, 0x65e438165cf83b8eULL, 
        0x0fe3f52721987356ULL, 0x696839144ac7ccd4ULL, 0x6a49b52f0bb65929ULL, 0x29895c7479841aa2ULL, 
        0x05dde69f1488c019ULL, 0x0ae6e0ab281cf986ULL, 0x4c7d2e3d4ac26bbdULL, 0x7417acb16b1c49f5ULL, 
        0x5672c458458fa2cfULL, 0x21b86a6d40328626ULL, 0x187c602367708f00ULL, 0x75ae4cf255f8b3e6ULL, 
        0x6f6159e61f161c7bULL, 0x3caf94911a26a42aULL, 0x77be13392293cca8ULL, 0x771edfb807a20861ULL, 
        0x442c3ffe608718ccULL, 0x5269d5352e75f52dULL, 0x6c3d71f57bf331a9ULL, 0x27fa0fcf721b5895ULL, 
        0x107bf1c232e0f07aULL, 0x1a38521b5cf92000ULL, 0x7da35c380e4ffeccULL, 0x481569f554162090ULL, 
        0x53dfa19b69cdd462ULL, 0x1448a6b66c5c01bfULL, 0x513e636309f6f3a9ULL, 0x4254b5a5409fbd49ULL, 
        0x290d10247f044a37ULL, 0x5ac6617320cb235eULL, 0x219816df51e5412cULL, 0x286d2bbf65c456ddULL, 
        0x326c59f87ad700f4ULL, 0x143a4c0a1ea9cbeeULL, 0x76ca329d3c345bdaULL, 0x10c5248307462460ULL, 
        0x6f154c542afd769eULL, 0x643f44606cb8a88cULL, 0x394d756a2c54ae55ULL, 0x40cec91d0d2d1706ULL, 
        0x162282b855176fd3ULL, 0x798918c56760e61bULL, 0x5f0e637c3bddce6aULL, 0x2800a364081b73a1ULL, 
        0x3ae218a102c704d7ULL, 0x28e696ff5c7a2f80ULL, 0x54ac46035153c2beULL, 0x423e865d07189ffcULL
	},

	{
        0x4c2ac3b25678d268ULL, 0x25c26bea42f4f64fULL, 0x12ad2e423687906dULL, 0x4a3b1aaf01c27a96ULL, 
        0x6185070b2e7a5f0fULL, 0x6e7b23231ad27c75ULL, 0x5acf0d652f49ec40ULL, 0x27ff937b70f1901dULL, 
        0x04615c132188ac40ULL, 0x58527638636fbf90ULL, 0x5d667aab0053199cULL, 0x6b8b33311848934cULL, 
        0x031a1e731471ca30ULL, 0x74c2c2cd57c66477ULL, 0x65c58cee3701492aULL, 0x5edf047331f050a0ULL, 
        0x0d7a1b9204a1705dULL, 0x74e546ef202749d4ULL, 0x3b2900ca3f20619fULL, 0x21e9c46b1cae07d5ULL, 
        0x6d9ac0ae1064e78eULL, 0x3780844a4869ce13ULL, 0x3faed3ce5f8017c6ULL, 0x395b5e3044102fe1ULL, 
        0x0108c40611add468ULL, 0x277fef715e6f3eb1ULL, 0x1200ee04130b22a2ULL, 0x76b7d1fe151b0c78ULL, 
        0x277cecd26b7a94cbULL, 0x6ce170ef0d4279c0ULL, 0x227bddf54bc07562ULL, 0x3f32ca602ff5f988ULL, 
        0x5061e5bf34181150ULL, 0x501d435c0b8ae689ULL, 0x733872ef720707c7ULL, 0x2838ee5e60d3339dULL, 
        0x026bef555fb972a8ULL, 0x293d01b1421ac323ULL, 0x3f398a6e62985fe1ULL, 0x062af30540424e75ULL, 
        0x7446344a2daae276ULL, 0x1eb18d260647224eULL, 0x40b6051915695f24ULL, 0x1b622ec66832f1ebULL, 
        0x00e3f3ef08439fb5ULL, 0x75756bac235fd1e5ULL, 0x5404151734a8360cULL, 0x5355cb6d2465fad6ULL, 
        0x68c0475c23730ec9ULL, 0x2ff0e15f5bf8ba4bULL, 0x157a16915829cfbdULL, 0x3ccbede917e605e6ULL, 
        0x37e342666608ef9aULL, 0x5a00c90a771cccd4ULL, 0x48a14f7b602bbc0fULL, 0x375f1b493ce783c5ULL, 
        0x0dd69e855610a870ULL, 0x432ea6144e8ca39eULL, 0x6b7a07945e90d4daULL, 0x36bf958a6c5dfb84ULL, 
        0x66d474902c350136ULL, 0x0fbdcd693ad889a7ULL, 0x60dd3742631398d6ULL, 0x5f3e847e499d7e9fULL, 
        0x0686a79f0f2f65ddULL, 0x259638ea1c00be30ULL, 0x6759359b626226d3ULL, 0x33e6c4171f3c7801ULL, 
        0x486b166d0de78d21ULL, 0x165944d5110c65e9ULL, 0x6e1349304db8601fULL, 0x4df3e9ae7be9e7b5ULL, 
        0x23c9088f11228fc2ULL, 0x4a768b540f431023ULL, 0x6fb3649d013620deULL, 0x7ba10ba75687d92dULL, 
        0x2d6b22140b5ed910ULL, 0x116062d40e485956ULL, 0x6e7271e6709ee752ULL, 0x57e5d7f574f91986ULL, 
        0x7fce4d307d7c10e0ULL, 0x10f9d7b6672782cbULL, 0x5fde37b344e09bcdULL, 0x0663facc28494e21ULL, 
        0x52c828ee1cbd3fa1ULL, 0x3955b40a40db721eULL, 0x6a759fc007499db8ULL, 0x3cc559d40e3ea84fULL, 
        0x186c2d7b073be528ULL, 0x1d81b873081f9218ULL, 0x087206061922c41aULL, 0x5ea76b4535dd281aULL, 
        0x24819d2b7007ce19ULL, 0x4425817012f40f11ULL, 0x60a6b56c1c0b5966ULL, 0x07ed28976075029cULL, 
        0x19876a4618e7004eULL, 0x479c85677965a1f9ULL, 0x5dc79c1b4e008033ULL, 0x21aef01a308fc50aULL, 
        0x6abdbfd45b04a424ULL, 0x716b372855335f95ULL, 0x624e41dd2e3090fcULL, 0x637207e47aba6f58ULL, 
        0x356c762400f3c057ULL, 0x02da01703dde7c2aULL, 0x1a16847261816cb5ULL, 0x73bba4443e98219dULL, 
        0x51893ace37e125b5ULL, 0x518c30ae322ff03aULL, 0x53ec7f1b59795946ULL, 0x12a4f2d66d73e961ULL, 
        0x726059945a41783dULL, 0x66d98b5a5027f5afULL, 0x2841f87008887b75ULL, 0x00b7bab912ffb845ULL, 
        0x638d1f997222f1e2ULL, 0x683317da45db6176ULL, 0x205382de4ba51fbeULL, 0x4095d0ce55bff903ULL, 
        0x4c98e016436fd23eULL, 0x139e752d66af6488ULL, 0x24f13ef3075a1972ULL, 0x25478625767a79c2ULL, 
        0x3f3b3f2776d3b6d3ULL, 0x28aa69fc1327be42ULL, 0x504d10193b4f5cd3ULL, 0x009ba7a342ad69adULL
	},

	{
        0x1590d510677532fdULL, 0x12d55f5d3dd2cd81ULL, 0x6ffdae72138d1a16ULL, 0x50d285c6538ace0cULL, 
        0x05b00bf839059da0ULL, 0x19662f8226038ed7ULL, 0x04aabd5e59fc0051ULL, 0x7bc387da51439d74ULL, 
        0x1d6bd28f0f61fd07ULL, 0x37f301fc425d1183ULL, 0x16bc16795d3a8821ULL, 0x38d78b4555f755a0ULL, 
        0x540e3ef56181f541ULL, 0x691f13e2245b4f0eULL, 0x1cd1521469babb85ULL, 0x6708b8bc32622725ULL, 
        0x512fee8379de1819ULL, 0x7034f4a6412d9cf5ULL, 0x0d6b322f41077a6cULL, 0x14b86b01131b3e28ULL, 
        0x7a0d180c2e1e9a84ULL, 0x391eccff7eb7d56aULL, 0x081a9ad534e254d9ULL, 0x4ffb72df25866d64ULL, 
        0x444451e007ee74dbULL, 0x67e37ee75b00685aULL, 0x6528fcfd20bb0a2cULL, 0x30f7bdfa39373bf2ULL, 
        0x023cff6e1a16d1ddULL, 0x5d928b001f0e5182ULL, 0x03d18d62449b43bcULL, 0x517078a755017be5ULL, 
        0x3e795bd541a56d4dULL, 0x162f18db4be48e05ULL, 0x02ace7b92ae783dcULL, 0x5effcc2d7cb9ffc5ULL, 
        0x59061e60181e992cULL, 0x7b71d5306120b935ULL, 0x4d00ee054b6d480fULL, 0x06a7269a11453fe5ULL, 
        0x535bbcea6e8aa581ULL, 0x6c45a83f3884b9e7ULL, 0x0f45afae1d3d663aULL, 0x71bbf5d91182af1cULL, 
        0x375438174f4e80daULL, 0x3091009e3b25c579ULL, 0x13e9c49602017946ULL, 0x1027415f5263206cULL, 
        0x43a6e69326565a3aULL, 0x1e47ae714653ce4dULL, 0x513dde167d477a9eULL, 0x430dce122a43fc77ULL, 
        0x156613ca3e7fa342ULL, 0x0b64b5ac626701cfULL, 0x09eceb51120bdc46ULL, 0x73ac41b45d48a83cULL, 
        0x009681c85ff1e9f4ULL, 0x15cd62230fdc3176ULL, 0x7d2f502e078957fdULL, 0x215ee09234838845ULL, 
        0x56d7d8d751efe130ULL, 0x6fa94dbe6ac19d6dULL, 0x53f15a767fd08f1dULL, 0x3d24bdd91798410aULL, 
        0x2626e9575b6c6c4aULL, 0x5dec0f577764c76eULL, 0x58b3e6e820f9dd69ULL, 0x21a8c3e56e19fab2ULL, 
        0x5f7980ac2d0d7991ULL, 0x5080fc8169666bfdULL, 0x3f1955d8442d3e36ULL, 0x46af14393fafd7a0ULL, 
        0x241f282a5c7c765dULL, 0x4f8c0916214e7858ULL, 0x6405ce5a70eae9a8ULL, 0x55d2009d3adda731ULL, 
        0x42dacad8457b4e5bULL, 0x259f449e16cc254fULL, 0x454bdd7962c40278ULL, 0x2e6466596b72c6d0ULL, 
        0x3e306ec20c5075b0ULL, 0x62d78e3e16e455abULL, 0x2d4a531904805223ULL, 0x04fe505d0cc3d3c5ULL, 
        0x318dcbb5557f4cdfULL, 0x762a3fc370a7218dULL, 0x19ac8b153cd953fcULL, 0x3056f92d3dcbb33fULL, 
        0x1955ca597fe30243ULL, 0x5f1a2b977d5b98b3ULL, 0x70cdebeb34ec2c34ULL, 0x38393fe433a8b6c3ULL, 
        0x7a677a8f5dd88483ULL, 0x4a74dc123fb35808ULL, 0x409c86fb78d9426bULL, 0x2b261ed97eccf5bdULL, 
        0x0529b81b0dfdad17ULL, 0x15b14b6832740b35ULL, 0x127dff3b1aaf9bc6ULL, 0x3f37defa440bcaf0ULL, 
        0x702ee8a535621ebdULL, 0x34b2ec7d09db73baULL, 0x723b72ba6509e5aaULL, 0x47a726f90b913d13ULL, 
        0x64ece7ed26c15290ULL, 0x08ecd5c755bad3d8ULL, 0x5bad7ec4412615abULL, 0x09638a9b5614f953ULL, 
        0x1efe9a2e53d866aeULL, 0x15c8515c5f9b2129ULL, 0x4cb1a91940ee7035ULL, 0x5e6816e751db6135ULL, 
        0x4eec1d4c7419624fULL, 0x044f6c6a616a1c87ULL, 0x0ec8fe1543874b64ULL, 0x2575e7777ef7e6baULL, 
        0x78e96a225a28d3f4ULL, 0x08d35a746b24dcdcULL, 0x3f32b99e507a816dULL, 0x76b619ef241fa18bULL, 
        0x773bd3fd7fa2efb6ULL, 0x79da756352e952c1ULL, 0x40c90562033dffffULL, 0x28fe4c155fc79f90ULL, 
        0x571666ad3ec69d71ULL, 0x3f62c0ba23c80fc6ULL, 0x7fb50da61dcad7a1ULL, 0x75a370fb4ea12af2ULL
	},
	{
        0x11e439f079f2dd65ULL, 0x300b477a20ad3806ULL, 0x3d7a28ca55812ef1ULL, 0x1fa51ec0366392ecULL, 
        0x2faa02e628787935ULL, 0x21886fc86edcbc84ULL, 0x78f2faa2183e89b7ULL, 0x12fc5e10702ecea0ULL, 
        0x17e1796e0cd6d373ULL, 0x4318216158aa7ed0ULL, 0x1014d3726c166d76ULL, 0x38721e60672b3a1fULL, 
        0x2add0ae777d4df1aULL, 0x0af349e62a92188dULL, 0x159fb6bb0096bae1ULL, 0x793343802783f0acULL, 
        0x7a899847293e8afaULL, 0x483128b23803c111ULL, 0x7ebfb9eb67d64772ULL, 0x6e6753fd2e69bcd1ULL, 
        0x104ec0a70fefc3c5ULL, 0x1d4679560941bb4aULL, 0x282e4d7c3042d766ULL, 0x797089ea400fc6eaULL, 
        0x3d19aad93c88ab4bULL, 0x18ba45ba4d2e7e4cULL, 0x289f18c2512c641bULL, 0x3459b86b537c23a9ULL, 
        0x490143353f4d0251ULL, 0x7e0e3c375ea0f9f1ULL, 0x3fe3bd3377417fb7ULL, 0x0624ea9d3a6d557aULL, 
        0x20800ab14e56134fULL, 0x7271168b1f3fc49cULL, 0x362c5ac160d86a88ULL, 0x4da9816e467b1b69ULL, 
        0x70c82e4d6aeffac4ULL, 0x4fbcd6b318f67bc9ULL, 0x1b32d22a492d609dULL, 0x590642b4584c7d03ULL, 
        0x05b60be871c0886eULL, 0x257afb4f2e5524aaULL, 0x42ecec8959d4b3bbULL, 0x01d148540bee2fbfULL, 
        0x1921b60c7fdf848bULL, 0x6a8f29b05905733fULL, 0x7721044270b4144dULL, 0x1372c8b917a10ef3ULL, 
        0x3f0a279c05e3df44ULL, 0x36e0d38f7536825dULL, 0x66bc49cc048a54fdULL, 0x3bb19dc657847819ULL, 
        0x6f7a4fc10b6e7479ULL, 0x707af3e30aad21ebULL, 0x549bd51649813697ULL, 0x62f99eef5a51e0ffULL, 
        0x3b41bf0508749a3eULL, 0x08a705a97e2eab8fULL, 0x62494df90a784dfdULL, 0x0a1cdb4e7b6b0406ULL, 
        0x0a57d28874ac04feULL, 0x547077450178d6caULL, 0x6560194b67e33fffULL, 0x1919e5bd246a40e7ULL, 
        0x6dc71f434ffab94dULL, 0x19a0c34454836910ULL, 0x54850e4a5552610bULL, 0x2c07e12943ff5e0cULL, 
        0x60c0d5841c82d50cULL, 0x4eac7ff7355caa9bULL, 0x66040ba331a61ee6ULL, 0x0fae8b9a2145caa9ULL, 
        0x3a1ab92518559143ULL, 0x1f7476381c64071eULL, 0x22cddf4129915186ULL, 0x17cf0b242d25b1c9ULL, 
        0x1e3d56846c3f826aULL, 0x2e9e8894039d6fcfULL, 0x5422c26947b86e51ULL, 0x2807b0b641e9e1acULL, 
        0x17b3279e41a873faULL, 0x166d4abc6c3835e9ULL, 0x16fad50542752be6ULL, 0x303793f577bbaa8aULL, 
        0x5ef800f27ee413ecULL, 0x2d18552544fc0c96ULL, 0x308a32d33cc6e0bfULL, 0x6641d73f6aa4ebf8ULL, 
        0x551c720205b64d77ULL, 0x0708f31677ea5143ULL, 0x2f479efd1ed7fe3bULL, 0x2510030d4d84f581ULL, 
        0x0b1780a553ae8ba1ULL, 0x512265505f3a430eULL, 0x1b66f9f2792a1606ULL, 0x212424ba331a2191ULL, 
        0x3ad28a0037916f77ULL, 0x1f52577a51cd5f06ULL, 0x7a069b5d4f89eb6fULL, 0x4989099058fe9c4fULL, 
        0x4e6dff5b76a15eb5ULL, 0x1dfaa8e57ef8322eULL, 0x33683f74043c8024ULL, 0x699d1e260884b176ULL, 
        0x09f2cd9b70a6113dULL, 0x006f02ba393a6c98ULL, 0x0f7e0f78257f05c7ULL, 0x06bf62191a95901dULL, 
        0x792d916857e1c769ULL, 0x79cfd32b14948b5aULL, 0x510bdd6f1af3f7e5ULL, 0x47aeaceb0bde6770ULL, 
        0x5285675c67010465ULL, 0x5dabc6764c8c02b9ULL, 0x368aefd42734d006ULL, 0x258a9f0904f8ef30ULL, 
        0x1dd62ebb438547eeULL, 0x03f1215e513e6e2fULL, 0x47c1c8136d8e3f85ULL, 0x59c31fa551b495aeULL, 
        0x5e3450c25a32225fULL, 0x0aef02476db2603aULL, 0x7fb1282611ae6460ULL, 0x0847f05778deb98eULL, 
        0x69902bca0217c382ULL, 0x0d7344e93a9c0939ULL, 0x1d0bbb675521f1d4ULL, 0x467a70a96f9122c4ULL
	},
	{
        0x3c22f63a2426371fULL, 0x3c1d257d72ade60eULL, 0x4b5b072561a7c486ULL, 0x77a6d53e693135e0ULL, 
        0x252d0c757b97f69dULL, 0x3a6fa40f6ceed488ULL, 0x692636221432c3b5ULL, 0x3ea36a36475a86e4ULL, 
        0x6e64e61449926c7dULL, 0x350ce71e6e160e3bULL, 0x5b40d0de3d54d775ULL, 0x66f4c7c944d0fca8ULL, 
        0x3f6c9af774680cb2ULL, 0x7f6d05e15c78565eULL, 0x4989fe8745e7768bULL, 0x4c09792205acf4c1ULL, 
        0x6a0dadaa08269ea0ULL, 0x785adacf3568b4d0ULL, 0x69ce63267001b00eULL, 0x1e99eab00efb6f9bULL, 
        0x6b99a6ab59098ec0ULL, 0x7bea442354bfdccdULL, 0x6d3c52753a8dae5aULL, 0x1c1a63b15ba13889ULL, 
        0x04201ad751274acfULL, 0x49b746c45f60ebb5ULL, 0x0e7c224430ac0e8eULL, 0x2431e85d4de8bd3bULL, 
        0x25141b40239eee3fULL, 0x2a6113996e9e19c7ULL, 0x698664ca766a8cbcULL, 0x744b0e8853941274ULL, 
        0x7e912b5c6ca5e958ULL, 0x08fcc744685f8e82ULL, 0x5ca799662796b1f5ULL, 0x775afe1e48414011ULL, 
        0x00a040b573454241ULL, 0x1d011cde6ddc932aULL, 0x2dd2f09b391b808fULL, 0x497dcbb331f30b73ULL, 
        0x0a42cb5e13351278ULL, 0x1153f72818beeda2ULL, 0x43e121063585df86ULL, 0x66a7aadd68f53c46ULL, 
        0x5924cdc51108be76ULL, 0x5793560e42ab328fULL, 0x07734b324bde6496ULL, 0x163f45030604768eULL, 
        0x38844dee1f3c0c48ULL, 0x6e640511152be754ULL, 0x46d2be3d65bf032fULL, 0x5d6d27654772fef2ULL, 
        0x590445707a6e4443ULL, 0x354f921c06d7360cULL, 0x3389c4d27ecd5dcfULL, 0x38ca417f3dcc9030ULL, 
        0x120270474a1e38a7ULL, 0x568b7dd255e3914dULL, 0x7fa4182d3d3328afULL, 0x3ed8cd9458c8e5f2ULL, 
        0x4e3be726166c23a2ULL, 0x1b74188155af3258ULL, 0x624a883831b35d85ULL, 0x5bb3a8e71aced627ULL, 
        0x50ef69cd4a17adf8ULL, 0x2ffabd7b17c2280aULL, 0x2fd6b1270d67e4e1ULL, 0x5f3526fc08daf697ULL, 
        0x07d629241484b918ULL, 0x0fb22ca33b5fedf7ULL, 0x135216e7487c6e22ULL, 0x792c7e272554872fULL, 
        0x129aa6ca4fb7fbfaULL, 0x7b38187c123ebef7ULL, 0x0ceb24a93a10e610ULL, 0x6b07a4ea5b270bcfULL, 
        0x507d09b2067bbd6bULL, 0x30d63e2832c791ebULL, 0x382f1af00c89e70fULL, 0x4d966812091e84bdULL, 
        0x56a195077d91258dULL, 0x20e0acc70678462eULL, 0x0af90a6e0015d3c3ULL, 0x0f533cc512cf3393ULL, 
        0x149a8cdb1f056969ULL, 0x4e2f218a27eca3c3ULL, 0x6781d78b475b9fb1ULL, 0x4d412af27a1c7e55ULL, 
        0x17139bab4879436eULL, 0x0c5b3d4d23fec055ULL, 0x028a297f7762e237ULL, 0x7f25cc2453073331ULL, 
        0x7dde9fa22ffc0a4cULL, 0x05cec51c360dba93ULL, 0x3c85f15b53652d2eULL, 0x3f2c3f5013278662ULL, 
        0x50f652bc600cec18ULL, 0x199fcc905bef5d2aULL, 0x6022bfdb28f30956ULL, 0x6ebe90bd74bd4cb7ULL, 
        0x47f872bf3cedb247ULL, 0x1ca9f07a2f7a4a4aULL, 0x044951f969eb1b6cULL, 0x2996c8a01b5ceda4ULL, 
        0x32645eda35f205edULL, 0x3f5badf934ee8859ULL, 0x2d54e8243e817a1eULL, 0x07f5bb8b2b3387c6ULL, 
        0x6e7d846a0dc480a7ULL, 0x614142592b0375c6ULL, 0x6129add6206d81aaULL, 0x3e2afc2832200092ULL, 
        0x007a6dc257cac8b9ULL, 0x0e0f5dbc609d2d9dULL, 0x00bdd20f7ccdee7aULL, 0x555a7a5448b644ceULL, 
        0x39bba0c172046aceULL, 0x78308f183e04f2baULL, 0x5bef863a21c757b8ULL, 0x5961e05f0e53e515ULL, 
        0x57b95da518bd8e58ULL, 0x43426d6e050e45c9ULL, 0x573f08764b3828f9ULL, 0x3041cd9045bc8ce1ULL, 
        0x58fca9a111830fe9ULL, 0x70c002a73a265777ULL, 0x31f091932eeafecfULL, 0x6c465809326aff55ULL
	},
	{
        0x06b5c7887a55b5c5ULL, 0x13082cf307739997ULL, 0x7723a43f6862a747ULL, 0x5029de6530df4501ULL, 
        0x5a671216485a6d7eULL, 0x6ee437bb36569850ULL, 0x6a21c5364846181aULL, 0x44aa7d6541db22dcULL, 
        0x6103a67307ecead4ULL, 0x46e968a53842aee9ULL, 0x532513cd772b3635ULL, 0x7dff3bca2c21bd6eULL, 
        0x08ae461f6ebf3e71ULL, 0x664814e53a9ed7b2ULL, 0x1daa3d41528e6ceeULL, 0x6d09d708246004c9ULL, 
        0x4ce422b4001203fbULL, 0x2bd39e614407c6f3ULL, 0x6874ab427bfd7cc6ULL, 0x74e70bf442dbbd58ULL, 
        0x4457ea4463cb43b0ULL, 0x793255a92e79af7bULL, 0x2c115bca3ddcd30eULL, 0x7054d2570d15023dULL, 
        0x45c9bde2373e3afcULL, 0x4557b12718eed1b0ULL, 0x2e6971324356ecf1ULL, 0x45108f1e3717b751ULL, 
        0x32162b632b58a404ULL, 0x71b68f034fc068a4ULL, 0x7de710f25ec0660bULL, 0x74206d6d4acb33a6ULL, 
        0x5ed26a061ff40bceULL, 0x0ed2fa9a47471549ULL, 0x1bf1889503ba068eULL, 0x0a22d2a1604972d9ULL, 
        0x67854a3e0355284aULL, 0x0ec322541396a609ULL, 0x4131fb597f17f4abULL, 0x20aba84606fbb93bULL, 
        0x36562fa86603596dULL, 0x1fea8aeb64bfa0daULL, 0x295a465f64fb1a0aULL, 0x1bd7582b5b7071c2ULL, 
        0x1053be0e0d8de72eULL, 0x2b30da660e3acf00ULL, 0x6c4e4d3a1f5147d3ULL, 0x590602a74b20b740ULL, 
        0x3f4553a267d8fd41ULL, 0x1267cc895b36dc37ULL, 0x6b9303cf1c8a9f2bULL, 0x3b804f1053184e0eULL, 
        0x1fdfc7754a437165ULL, 0x66aef4176111c2ceULL, 0x495b6610075a9c5dULL, 0x680d7c0a7fb195b8ULL, 
        0x6d5df5cb07f806f5ULL, 0x6471369216b83c2aULL, 0x6cf320ff00488ebdULL, 0x7228adec7d46df0dULL, 
        0x0dd675ec1d598852ULL, 0x0b81ae0e7a24c326ULL, 0x3caad0256487b0b5ULL, 0x45457a667bf023c7ULL, 
        0x4c60adf657ad46f0ULL, 0x5726fffe37f3b1c5ULL, 0x7437e61b12a74f0fULL, 0x0b0bffd31417ad90ULL, 
        0x5ceac07471baf3eaULL, 0x7529705f26462684ULL, 0x791590485d36ec69ULL, 0x25f7bc3d66738613ULL, 
        0x652ef35e0a68f2cfULL, 0x7d2bc23d5222145eULL, 0x0ab1818d6f547029ULL, 0x4f68f36b1887f779ULL, 
        0x0cadf87b5aeaa179ULL, 0x12acba9f4958c8a0ULL, 0x3f72522e57f23505ULL, 0x4548ec680bd30024ULL, 
        0x2f9f7bf51c6fec66ULL, 0x43c6b1ea23d76210ULL, 0x2f173b754ed2b1bdULL, 0x37ef0fa10c01fbe9ULL, 
        0x408da5a82d188000ULL, 0x3248226e39a335f0ULL, 0x0a4f6c69583fdeabULL, 0x2016bc036f7e5fc7ULL, 
        0x62a8d17a1d427e40ULL, 0x41a074256d5a5307ULL, 0x0c96ee6911096791ULL, 0x05e24a801944e6e4ULL, 
        0x6bf4090a188f051fULL, 0x629daf842b665b39ULL, 0x70813a2527e69becULL, 0x37395b5d2020b61aULL, 
        0x445688537b000d47ULL, 0x43f8182b736dc3c8ULL, 0x49d2bf057be727ccULL, 0x7f6fbfb20a6064adULL, 
        0x28ffa7cc31b7e220ULL, 0x44039a9d334f1435ULL, 0x09f7c0cb641a56a0ULL, 0x22cd73fc6ca09245ULL, 
        0x015cd4e0646de822ULL, 0x59fae54d0df3c349ULL, 0x75774fb35fdd2fcdULL, 0x2738aa2d616b58bdULL, 
        0x786c34ed09d659b1ULL, 0x0cd1b3f668ed6f12ULL, 0x31bcf59e440b0f54ULL, 0x090e252c76137df1ULL, 
        0x3f0b1c9b4d063d57ULL, 0x698141b908dddba0ULL, 0x48ed652368f1016bULL, 0x133e404d71ed0cefULL, 
        0x1aa8e38b5741daeaULL, 0x253c212424a0a456ULL, 0x3b5c318a48099521ULL, 0x1141369c3cb9066aULL, 
        0x2c777d436b3c1be9ULL, 0x4aacc9b321eeccf6ULL, 0x4b194bb671e573e0ULL, 0x035a25b3438580a3ULL, 
        0x7bbbcd92102bd9aaULL, 0x2c72efb52d78c330ULL, 0x5436e8fe358114e2ULL, 0x238c412113420599ULL
	},
	{
        0x028752390d0d82daULL, 0x1c1fe13a4b74b75dULL, 0x75fe84462f5e2187ULL, 0x3d61c44c10a767d1ULL, 
        0x069ffc72629de571ULL, 0x35480c2841fc2dfcULL, 0x2aa77a92468942c4ULL, 0x7eb53467571ef7d5ULL, 
        0x31c55ead4961fe1aULL, 0x790dc4cb7cdeaa63ULL, 0x3b4771fb7c67ea7eULL, 0x40642b0737033f8dULL, 
        0x0c93c4286cd71abcULL, 0x647c02bd60caad26ULL, 0x22582f9e080843deULL, 0x740cb2c024df81d8ULL, 
        0x1515c6b8102c93faULL, 0x705439350b144afeULL, 0x3f8ab5812db5fd81ULL, 0x1bbbb2d0462ab1f3ULL, 
        0x1053e2f25103bef8ULL, 0x0826dff03afb5d84ULL, 0x178d01bc06dc1457ULL, 0x121a555949526069ULL, 
        0x503e12710b281a24ULL, 0x46310acc0b85846cULL, 0x079004a3069535d3ULL, 0x4288c3f91423c8cbULL, 
        0x736c50902704c6b6ULL, 0x74ee75f215c4802eULL, 0x2f0d0a9468fb28b2ULL, 0x3aa402064422d14dULL, 
        0x7927bcac2af83b3bULL, 0x4f371c4b38b2722dULL, 0x58ae38bd6af2cf1bULL, 0x7edd242169021bafULL, 
        0x3bf68e1307040411ULL, 0x23fd793453838fcfULL, 0x0de018683617ce8dULL, 0x1cd5f0385e1e2ad9ULL, 
        0x413fe8b26306fb05ULL, 0x69a3af4648cfed55ULL, 0x699c30d82c2c733fULL, 0x5cf3b6205d088168ULL, 
        0x533139f651e22c12ULL, 0x72cd0197023e448aULL, 0x3add54c42d71039dULL, 0x466115d734051170ULL, 
        0x58693ed915983223ULL, 0x6cb7839e31177796ULL, 0x008b013e6b94a7bfULL, 0x1a1993453c818f52ULL, 
        0x7298abd03e170c79ULL, 0x10051f210078c438ULL, 0x742edb072cdb0f5aULL, 0x5e96ef11356ec3b9ULL, 
        0x0fe20a5f483a9e57ULL, 0x7e3eb10e797e3b37ULL, 0x746711975b32672eULL, 0x5686bca047984b8dULL, 
        0x2d1493414953be37ULL, 0x49d6901767f1e805ULL, 0x76c4c1d41037a5efULL, 0x1bf6f9764f2e00adULL, 
        0x25cfd81208ae7d14ULL, 0x00457843265ad950ULL, 0x744324d31a5f0b89ULL, 0x62dc68a266dbd0a3ULL, 
        0x5876180272e187c4ULL, 0x675494db4ca4f309ULL, 0x1fbc971e45eb83ecULL, 0x0213b6c22f9ea17dULL, 
        0x0e262244005267d0ULL, 0x291cdcb4028d33dbULL, 0x5b84ceff7fa39954ULL, 0x4a257f6808996240ULL, 
        0x48f7578b13fc0f7fULL, 0x708b4a453fbc1960ULL, 0x2433b56e0c8243bbULL, 0x0eea1a0d4a038d80ULL, 
        0x1530c0cf0f2f9251ULL, 0x705e66d10973e5a2ULL, 0x298e9dda533acf73ULL, 0x704fb6450204b5dcULL, 
        0x461c573757a44b20ULL, 0x4ea9a8e665d8ee55ULL, 0x1d8fcf0d50bd5fa8ULL, 0x15778fd22bb5f151ULL, 
        0x510fc7793e946c87ULL, 0x2e43252c2c949678ULL, 0x3e3805db7868a494ULL, 0x352df8b8072f5d67ULL, 
        0x0c64b41325b942fdULL, 0x46eb76c730986982ULL, 0x323b86b955d590d4ULL, 0x7a9bf702476c4788ULL, 
        0x650523256afa5dd3ULL, 0x50e02d2b0e93c0ffULL, 0x3e352d47412fe370ULL, 0x109876dc0451847eULL, 
        0x18d42e915f421fc2ULL, 0x6a2a72d43663fd9eULL, 0x2fff7f6a7fa202a6ULL, 0x6219eeef010f46e3ULL, 
        0x3e366f2d105d141bULL, 0x2da3dd5b7c6e7509ULL, 0x08c5b8af62d1d613ULL, 0x039dd270152a6cc2ULL, 
        0x088b19114a894937ULL, 0x45c2d6443ac69fcaULL, 0x205eda0b405ecd47ULL, 0x0232e7520563fd31ULL, 
        0x2b592b1a5313147dULL, 0x13f7be30698e5861ULL, 0x1442f7ee2490350cULL, 0x6ddfdce02d17267fULL, 
        0x03d254ce580a4fb4ULL, 0x637b241d33d1d439ULL, 0x57ac525a4595130cULL, 0x34e11b1c15e2c188ULL, 
        0x55f227276284f878ULL, 0x125136915eb7dfd6ULL, 0x4556ce8b15ef0901ULL, 0x73e24c984de1e79cULL, 
        0x6078523839a522ddULL, 0x08a8876600d72c43ULL, 0x7a03f0240adb6eb9ULL, 0x063b2974255d1b3eULL
	},

	{
	0x21e550ddb48df5d5ULL, 0xe3082b02fdbbd47aULL, 0xe31656fbe8651387ULL, 0xc997bf7f0c48fc98ULL, 
	0x410477d9c4d3bfa0ULL, 0x7c4b3468afabf062ULL, 0xb04fd01f2e2806f8ULL, 0x9365c805e9d6dd50ULL, 
	0xe4d8beb64bf635a9ULL, 0xe366f8e001fa311bULL, 0xd4c1dfc77f0cd80cULL, 0x800f80c17b8797bcULL, 
	0xb36a0eba707cbdbaULL, 0xbf605eb21c2ffe6cULL, 0xf874693a0398f18bULL, 0x369ccefbeae292a5ULL, 
	0x56c14d2f5cefc199ULL, 0xb9424d2528d2c11eULL, 0xc37157c1e154ba3dULL, 0x5d1499fa5d7037d2ULL, 
	0x74e5655b36d6a80fULL, 0x7813af824c6ce1f9ULL, 0xdebf92a442c899eaULL, 0x9e80cb07fcb7ec0dULL, 
	0xb5df3c8727badae4ULL, 0xa5db6b11b184c6c5ULL, 0xf066012b4d7df759ULL, 0xa1e74b6af0411a1cULL, 
	0xc3b6bcfa5722bfe0ULL, 0xe7a3691a7fa2d84aULL, 0xa26f8b9e2bfa3d53ULL, 0x0d51016b61962ac8ULL, 
	0x4372ccfeed96bfcbULL, 0xe77291e5dd4e7f50ULL, 0x9ce11a4403c7d1a0ULL, 0x8ac6d552763c82d9ULL, 
	0xe01c53d9129e426cULL, 0x4c50cbbaf311239eULL, 0x85c49566fe6d1d08ULL, 0xc24faf6658bd8c17ULL, 
	0x40bb3bd4edc3b08cULL, 0xbf45ffe0ea722cfdULL, 0x0521e34c44738753ULL, 0x5af377f02fa1af4aULL, 
	0x0b576b37a78c71f3ULL, 0xe85a15a0eafa0235ULL, 0xc6ffee3efc617948ULL, 0xfdbb1538226e5239ULL, 
	0xeaf9cb4a6782ed69ULL, 0x6e96f5421a2f0d0eULL, 0x6d669d844ec05ab0ULL, 0x51ad718659aedfadULL, 
	0x84a74355562b8cb6ULL, 0xfa02870ea39bb550ULL, 0xa55ed86563169253ULL, 0xfed47422fde8bd6eULL, 
	0x836bbb9f9c12b7a1ULL, 0x33a4b44badc663c2ULL, 0x14f0872a63ee89f9ULL, 0xad35045336a55444ULL, 
	0x8c4d1b7160bcd4f4ULL, 0xc28662435f377d2dULL, 0x6223931c76cda66aULL, 0x06da0a632b6c0cf7ULL, 
	0x4a544b13cab34d75ULL, 0x4eb07a3ce1766d59ULL, 0x3800e280c23d536dULL, 0xb0ca6f9805c54d4eULL, 
	0x628833cd037e88edULL, 0x8028e57f5c0b990eULL, 0x3e8d5da172c5f8ccULL, 0x540d193beb387f12ULL, 
	0x7ff1bce733a5ef23ULL, 0x00f88954f87f6b14ULL, 0x1bd4ecc6aceefb4dULL, 0x99acf293064e0a0bULL, 
	0x4798cca881b0e8e1ULL, 0x75285002dc584932ULL, 0x77dd7836983ec5b9ULL, 0x28aee1ea7e8e5601ULL, 
	0x62844c591628ddedULL, 0x87be21d331219d32ULL, 0xfcaee73a603fbed7ULL, 0xa81bcd867a81cbbcULL, 
	0x79bd25421a953510ULL, 0xdfc4e30d1e5fcedaULL, 0x5051231a29794e71ULL, 0xc2fba0b022add203ULL, 
	0x344c3dabb47d5812ULL, 0x254180f9cc9c44f3ULL, 0x1bce121e1e73de4dULL, 0x1e5741b09f9cd1a0ULL, 
	0x3a387edc0c6aaebbULL, 0x013edfdf625f6744ULL, 0x062b9e8e64b5d435ULL, 0x633698ce19d63259ULL, 
	0x3489cf1c4be39fd3ULL, 0x19c2e80709319f97ULL, 0xb872aeb126c79aeeULL, 0x39a18d52b7e15bf7ULL, 
	0xc94817b599719321ULL, 0x18d583b8e99954b2ULL, 0xdaab29d08a329643ULL, 0x4a9f0883a247b642ULL, 
	0xa27c40ed1d9af26eULL, 0xa47f983c291fee5eULL, 0x14dcf933b87d32fbULL, 0x3b38f1ab0290a901ULL, 
	0x662d310d00e92482ULL, 0x65c90fd9f24bd563ULL, 0x0d0f0522d930d5ddULL, 0xb7743010fe429efeULL, 
	0xbe63d25c69e39125ULL, 0x05bad0d86ba67189ULL, 0x6e4b35e515d3e6b3ULL, 0x645cadfabee7fcffULL, 
	0x52270b248112a11fULL, 0x295bc481bdb72b98ULL, 0xde9971c393efcf43ULL, 0xeaf751b36b062bcdULL, 
	0xc980c4ab70fdbd37ULL, 0x7bb2d11b0f066957ULL, 0x0600a1067c0bf756ULL, 0xf24e02822d289330ULL, 
	0xcc77e53b5d2c4b37ULL, 0xa3cae0f08a1c948fULL, 0x8e88adf4f7b0c6b3ULL, 0x2467a9ae2bd49cefULL 
	},
	{
	0xa28114f913f32a2cULL, 0x1a8c19f5da3616cdULL, 0xb9340ae4416ea8ecULL, 0xb658cd90b5d5aea3ULL, 
	0x3bf9dd08548c4a0eULL, 0xe6bdc91f741670cdULL, 0x486cb5eb1cdb6a86ULL, 0x6975d84688a75461ULL, 
	0x426afd50aa28a08bULL, 0x418003eba37393b4ULL, 0xc8105d3d4e80c791ULL, 0xaf189232b6a9a75aULL, 
	0xbebbbdf89b2ff64cULL, 0x33bd0e81f2b64828ULL, 0x4208480561c486b2ULL, 0x9029417dc7456f33ULL, 
	0xb7d46529b10912f8ULL, 0xc55c330ae64555d1ULL, 0xbd3cbe69dd0e7193ULL, 0x14912e4e41e27397ULL, 
	0x359d3d0a741fbc38ULL, 0xfe45baae09898259ULL, 0x4194099148c84edcULL, 0x4436a1ceaee77c2cULL, 
	0x41fe8cc46bd7beb4ULL, 0xe760eb94e2ea9965ULL, 0xd7f870a62c59e635ULL, 0x2701e12595be529aULL, 
	0xe2df9c7e1e9adf13ULL, 0x86960de5f9d160a0ULL, 0x85503ad010290144ULL, 0xc5da387a7fcebd8aULL, 
	0x2128b36115d1e6feULL, 0xe5cd6ac8d7a4a1afULL, 0x5584b0367da067b4ULL, 0x27aaecf5f37f85a4ULL, 
	0x04c11a95d9e29c1cULL, 0x0cee486603cc223dULL, 0x4f7c1a01fa26df2bULL, 0x545746c07939718fULL, 
	0x96921941f137c916ULL, 0x01e2f0e705b2acf0ULL, 0x7a20c0580f233251ULL, 0x54cb8e0099644af4ULL, 
	0xdc83f78de5363593ULL, 0xce8faa7266bd0670ULL, 0xde59e96444ff28d0ULL, 0x30ec0ce0dc68d87aULL, 
	0xe07dfea23e49a73cULL, 0x7bdfbe2fad54fa66ULL, 0x830dde4d2222894dULL, 0xefa40886c9aee3caULL, 
	0xa96674a883d7e8b9ULL, 0x0eb9734762e14e7aULL, 0x7225e83a30f41d73ULL, 0x99dac91ae99c338bULL, 
	0x3f28a2c63c48c1b1ULL, 0x910412e20ecbcb53ULL, 0xb28a4d53f6deace7ULL, 0x377698c5c29b8f66ULL, 
	0xaba9d025f103f7ccULL, 0x0baae327387a399aULL, 0x4b2255c2fd46fd54ULL, 0xd05fbdaede14c103ULL, 
	0xf4d346ed2b7255b3ULL, 0x85922e3f685660f6ULL, 0x46d74aaccc95d960ULL, 0x6c7f80ffc56d8f09ULL, 
	0x228c4b4571fca20eULL, 0x06a43b5226c70710ULL, 0xaa90723cec3408b3ULL, 0x13bd28ddfd10c221ULL, 
	0x3ebe28554c09a584ULL, 0x96c85187fb35f88fULL, 0x7f351797e38952f6ULL, 0xce01ff72106322f2ULL, 
	0x4e502646430127beULL, 0x3de6ba066e08f91cULL, 0xcdad7fe83bfe7fd1ULL, 0xa4334be686d07625ULL, 
	0x5d2a8b40de4df064ULL, 0x04e6bcf808a8d35fULL, 0x9de2f2557bfa56ebULL, 0x9e3c5561e5bd8861ULL, 
	0x70349f6aa553c81dULL, 0xf2b0a0854f7d4effULL, 0xf6bbba062ce5a0edULL, 0xc302650ab7941e4fULL, 
	0x9156aced217c7791ULL, 0x102daed4cdf031a5ULL, 0xe1201e24d526257eULL, 0x1b6ec30983bc0196ULL, 
	0xc778f8f1d930c469ULL, 0x65432d0e0a6746f8ULL, 0x65f965d7fe27ad47ULL, 0xae68b687d29df9dfULL, 
	0x1b82cc9d55d8794dULL, 0xf9db675a8d4b53a1ULL, 0x8a2ed846304f00f0ULL, 0x85d987ac2b9fcdd2ULL, 
	0x935c70191eda5ce4ULL, 0xd4dea2e1de052147ULL, 0x58a8bf9af106e520ULL, 0xa7a77f9f162a4716ULL, 
	0x3aee2c8ebba036d7ULL, 0xff3227ca85fb7893ULL, 0xd84d62faccb4267fULL, 0x1cbce4891ba62d54ULL, 
	0x15214824b490cecdULL, 0x81c13e3e0a971f2dULL, 0x1106088f46c189b6ULL, 0xedfefe91c37b4833ULL, 
	0x2edb0b029214ee6fULL, 0x62712e65f640e0bcULL, 0x0bbcfa81e996d76dULL, 0x215717e09511605cULL, 
	0x8c05bf51dc935c65ULL, 0xaa2c4166d15d81e8ULL, 0xcf5581f73c99d74bULL, 0xc0ae759d1ad13d76ULL, 
	0x3788aa381a75e156ULL, 0x62d9bd6a2158cb5bULL, 0x64bbe319c73353f9ULL, 0xd3ea61f1d821a62aULL, 
	0x384a14e0d52145ebULL, 0x9260eb98709886baULL, 0xd2d4681013cc111fULL, 0x60f52304596a6420ULL 
	},
	{
	0xecefc263a48c5df9ULL, 0x79a604eb251bf4eaULL, 0xcefa922621efa0f0ULL, 0x53301c7e477b8911ULL, 
	0x4612c901d9740cb9ULL, 0x386bb4e02f573839ULL, 0x8d8c6af134e15343ULL, 0x00ca6d2ee6945370ULL, 
	0x1dfdb434da3965baULL, 0xc50329c5b6596903ULL, 0xad3b2947b6c2748cULL, 0xe00b084328084d7eULL, 
	0x385b8be6f002f065ULL, 0xe9176b8304490eafULL, 0xf7b1d80ff0bd8b32ULL, 0xba9cf4a353ffffa3ULL, 
	0x61d254fe61f73620ULL, 0x6c4f83005f50b0a7ULL, 0x30937f312af7219dULL, 0x57243938b0a0f248ULL, 
	0x5e0a19647542be54ULL, 0x165278261096d652ULL, 0x238c2494ac9bbd34ULL, 0x804ce0e88714add3ULL, 
	0xf8ade1007608106aULL, 0xaec952dc5f420818ULL, 0x9742d122bdcfe861ULL, 0xfbbcf09d2083b8aeULL, 
	0xf760b5baaa74b5c8ULL, 0xfe5b1a09937ccf61ULL, 0x535d8dc2a6bb298cULL, 0x901b713ec2149c3fULL, 
	0x23cd9c7a5bab34d8ULL, 0xccb1d797f66eb195ULL, 0x2086615cb089081bULL, 0x09126cb3b7efe0f0ULL, 
	0x449c9e28cfd81501ULL, 0xe172916dce3d381cULL, 0xc66553d9205e0d77ULL, 0x2d48e7e4453d0c28ULL, 
	0x2175c3ac4e21e1aaULL, 0x054750736414ec5fULL, 0x0da16e1f4165c108ULL, 0xc366edb9b526a94eULL, 
	0x83fe14ee22ae1f3dULL, 0xffd81d91001954c4ULL, 0xbde2b7175ac3ab36ULL, 0x9413831a4fd13eccULL, 
	0x32e298d692a85721ULL, 0x99ccffafea74f8b4ULL, 0x9dd238a9b2a3546aULL, 0x68f8b3ef5a675309ULL, 
	0xf6c7574de53712beULL, 0x99ccffb5694e6098ULL, 0x7617f8be922b420aULL, 0x06bc8420200f706dULL, 
	0x97565a396482d67cULL, 0xc87f248bc6cf15d6ULL, 0x1059ff3c4283ff80ULL, 0x3808ff95e7f21e60ULL, 
	0xdc36a88457b22ec3ULL, 0xee8e761a4a1e9ed8ULL, 0x3341560d0ad41332ULL, 0xc4842a36f837e44aULL, 
	0x8e0f491406ee9ffbULL, 0xd3a0fd483a648304ULL, 0xa77604173246048aULL, 0x73d70fec9b074b93ULL, 
	0x758b46d3b85fb38cULL, 0x3e5ec2fee1c84dc4ULL, 0x33a1114401005ceeULL, 0x0daab69d178adaa3ULL, 
	0x5950a6a7b62df1deULL, 0xf96fcc258673837fULL, 0xa16a867ac02ba2c8ULL, 0x5aa42532b6e719e3ULL, 
	0x0206717a497fe159ULL, 0xca7d23a3708cad9cULL, 0xb7786aa3b8ee5f7fULL, 0x226e6694be4790b9ULL, 
	0x3856b033b77d0c66ULL, 0x7a2dd062e83c5385ULL, 0x3e74c5a62e721a7bULL, 0x2caf81a978d1c88eULL, 
	0xc2e86aba4851f96bULL, 0xd129da4836abfea1ULL, 0xfd059f6b6dde5b24ULL, 0x41107c5b2cae48cbULL, 
	0x6963a7f7462030d2ULL, 0x9619493fa2003558ULL, 0xbed401dabc5babe2ULL, 0x29396191220598d6ULL, 
	0xf56f6fd2f7153a01ULL, 0x93a4262e74648112ULL, 0x4789f2db6211901dULL, 0xacd13833a1ff4119ULL, 
	0x2eb5cb33a5559e62ULL, 0x8e7378fcf3fe6937ULL, 0x61cb7b56a827943dULL, 0x91810728f2c4cafaULL, 
	0xdbddc202960be45bULL, 0x502e479368f7752fULL, 0xd442a333d6c63faaULL, 0xa2f0d85a5e7bbbe3ULL, 
	0xc68f5d27125d9556ULL, 0xa17c9bda1b762d63ULL, 0x6897725a341518ccULL, 0xa5c6b2b02b4d9d3bULL, 
	0xb572a38a637338baULL, 0x48057db954a41a39ULL, 0xe572f0b3093da70cULL, 0x63ad9e12a161f66aULL, 
	0x712e9c13d07655efULL, 0x0f72f4185ba8c31aULL, 0x127a26279f6574d0ULL, 0xa44ba2680ae051d8ULL, 
	0xc26c50aba08e755dULL, 0xbc6b09df77abb06fULL, 0xb8581b9c3cb60881ULL, 0x3048c99aacf133edULL, 
	0x6fd4c8381ce21f6cULL, 0x1897c3f6f2d56a9eULL, 0x9fb4d7fb2958ea88ULL, 0xce4d1891d1bd2611ULL, 
	0x420d0ba38c9bdb84ULL, 0xeb9f2a46124d7811ULL, 0x8f34632dae72a24bULL, 0x48029933bf6bb1acULL 
	}

};





