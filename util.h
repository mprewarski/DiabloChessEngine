/*
    Diablo chess engine, Copyright 2006, Marcus Prewarski
          
    This program is free software. It may be copied or modified under the terms
    of the GNU General Public License version 2 as published by the Free Software 
    Foundation.  This program is distributed without any warranties whatsoever.
    See the file COPYING included with the distribution for details of the GNU general 
    public license.
 */

#ifndef _UTIL_H
#define _UTIL_H

int set_position(char *s);
int parse_move(char *s);
char *lan_move(int move);
int gen_hashkey();
int repetition();

#endif
