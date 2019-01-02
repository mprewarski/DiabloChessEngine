/*
    Diablo chess engine, Copyright 2005, Marcus Prewarski
          
    This program is free software. It may be copied or modified under the terms
    of the GNU General Public License version 2 as published by the Free Software 
    Foundation.  This program is distributed without any warranties whatsoever.
    See the file COPYING included with the distribution for details of the GNU general 
    public license.
 */

#ifndef _ORDER_H
#define _ORDER_H

int order_moves(int);
int order_q();
int next_move();
int update_ordering(int, int, int);

#endif
