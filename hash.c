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
#include <ctype.h>
#include <string.h>

#include "diablo.h"
#include "hash.h"

int hash_entries;
HashEntry *tt = NULL;

static int hash_gen = 0;

#define BUCKET_SIZE 2

int clear_hash()
{
    memset(tt, 0, sizeof(HashEntry) * hash_entries);
    return 0;
}

int set_hash_size(int mb)
{
    int size;

    size = mb * 1048576;
    free(tt);
    if ((tt = malloc(size)) == 0) {
        printf("ERROR: Couldn't allocate memory for hash table\n");
        exit(1);
    }
    hash_entries = (size / sizeof(HashEntry)) - BUCKET_SIZE;
    clear_hash();
    return 0;
}

int age_hash()
{
    hash_gen++;
    hash_gen &= 0x3f;

    return 0;
}


int probe_hash(int depth, int *alpha, int *beta, int *move, int *do_null)
{
    HashEntry *hashe;
    int value;
    int flags;
    // int i;

    hashe = tt + (board.hash % hash_entries);

    if (hashe->hkey !=board.hash) {
        *move = 0;
        return 0;
    }

/*
    for (i = 0; i < BUCKET_SIZE; i++, hashe++) {
	if (hashe->hkey == board.hash)
	    break;
    }
    if (i == BUCKET_SIZE) {
	*move = 0;
	return 0;
    }
*/

    flags = (hashe->move >> 24) & 3;

    value = hashe->score;
    if ((flags == UPPER_BOUND) && (value < *beta))
	*do_null = 0;

    *move = hashe->move & 0x00ffffff;

    if (abs(value) > (INFINITY-300)) {
        if (value > 0) 
            value -= ply;
        else
            value += ply;
    }


    if (hashe->depth >= depth) {
	switch(flags) {
	    case EXACT:
		*alpha = value;
		return EXACT;
	    case LOWER_BOUND:
		if (value >= *beta) {
		    *beta = value;
		    return (LOWER_BOUND);
		}
		break;
	    case UPPER_BOUND:
		if (value <= *alpha) {
		    *alpha = value;
		    return (UPPER_BOUND);
		}
		break;
	}
    }
    return HASH_UNKNOWN;
}

int store_hash(int depth, int score, int flags, int move)
{
    HashEntry *hashentry;
    int age;
    // int i;

    hashentry = tt + (board.hash % hash_entries);

    age = (hashentry->move >> 26) & 0x3f;

    if ((hash_gen == age) && (hashentry->depth > depth))
        return 0;

/*
    for ( i = 0; i < BUCKET_SIZE; i++, hashentry++)
    {
	age = (hashentry->move >> 26) & 0x3f;
	if ((hash_gen != age) || (depth >= hashentry->depth))
	    break;
    }

    if (i == BUCKET_SIZE) {
	return 0;
    }
*/

    hashentry->hkey = board.hash;
    hashentry->score = score;
    hashentry->depth = depth;
    hashentry->move = move & 0x00ffffff;
    hashentry->move |= (flags & 3) << 24;
    hashentry->move |= (hash_gen & 0x3f) << 26;

    return 0;
}
