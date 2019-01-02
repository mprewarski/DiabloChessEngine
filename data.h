/*
    Diablo chess engine, Copyright 2006, Marcus Prewarski
          
    This program is free software. It may be copied or modified under the terms
    of the GNU General Public License version 2 as published by the Free Software 
    Foundation.  This program is distributed without any warranties whatsoever.
    See the file COPYING included with the distribution for details of the GNU general 
    public license.
 */

#ifndef _DATA_H
#define _DATA_H

extern int outposts[2][128];
extern int pawn_rank[2][128];
extern int king_exposed[2][128];
extern int pawn_tab[2][128];
extern int pawn_end_tab[2][128];
extern int kingstorm_tab[2][128];
extern int queenstorm_tab[2][128];
extern int knight_tab[2][128];
extern int bishop_tab[2][128];
extern int rook_tab[2][128];
extern int queen_tab[2][128];
extern int king_tab[2][128];
extern int king_end_tab[2][128];

#endif
