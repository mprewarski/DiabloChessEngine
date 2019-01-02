/*
    Diablo chess engine, Copyright 2005, Marcus Prewarski
          
    This program is free software. It may be copied or modified under the terms
    of the GNU General Public License version 2 as published by the Free Software 
    Foundation.  This program is distributed without any warranties whatsoever.
    See the file COPYING included with the distribution for details of the GNU general 
    public license.
 */

#ifndef _HASH_H
#define _HASH_H

//
// Definitions
// 
#define UPPER_BOUND     1
#define LOWER_BOUND     2
#define EXACT           3
#define HASH_UNKNOWN    4

//
// Function Prototypes
// 
extern int clear_hash();
extern int set_hash_size(int);
extern int age_hash();
extern int probe_hash(int depth, int *alpha, int *beta, int *move, int *do_null);
extern int store_hash(int depth, int score, int flags, int move);

//
// Global Variables
// 
extern int hash_entries;
extern HashEntry *tt;

#endif
