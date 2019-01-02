/*
    Diablo chess engine, Copyright 2005, Marcus Prewarski
          
    This program is free software. It may be copied or modified under the terms
    of the GNU General Public License version 2 as published by the Free Software 
    Foundation.  This program is distributed without any warranties whatsoever.
    See the file COPYING included with the distribution for details of the GNU general 
    public license.
 */

#ifndef _SEARCH_H
#define _SEARCH_H

int     pv[MAX_DEPTH][MAX_DEPTH];
int     pv_length[MAX_DEPTH];

extern int root_search(int depth, int alpha, int beta);

#endif
