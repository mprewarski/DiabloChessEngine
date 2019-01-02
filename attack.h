/*
    Diablo chess engine, Copyright 2006, Marcus Prewarski
          
    This program is free software. It may be copied or modified under the terms
    of the GNU General Public License version 2 as published by the Free Software 
    Foundation.  This program is distributed without any warranties whatsoever.
    See the file COPYING included with the distribution for details of the GNU general 
    public license.
 */

#ifndef _ATTACK_H
#define _ATTACK_H

extern unsigned char check_tab[128];


int in_check(int side);
int color_attacks_square(int color, int sq);
int find_checks();
int hung_value(int side);

#endif
