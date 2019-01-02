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
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

#ifdef _WINDOWS_
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif

#include "diablo.h"

//-------------------------------------------------------------------------
// gtime
//   returns the current time in milli-seconds since 1970.  
//-------------------------------------------------------------------------
int gtime()
{
#ifdef _WINDOWS_
  struct timeb time_b;
  int  ms;

  ftime(&time_b);
  ms = (time_b.time * 1000) +  time_b.millitm;

  return(ms);
#else
  struct timeval tv;
  struct timezone tz;
  int  ms;

  gettimeofday(&tv, &tz);
  ms = (tv.tv_sec * 1000) + tv.tv_usec/1000;
  return (ms);
#endif
}

int alloc_time()
{
    unsigned int time_to_use = 0;
    unsigned int cur_time;

    if (moves_to_go) {
	time_to_use = time_left / moves_to_go;

	time_to_use *= 8;
	time_to_use /= 10;
    }
    else {
	if (board.gameply < 40) 
	    time_to_use = time_left / 40;
	else if (board.gameply < 60)
	    time_to_use = time_left / 28;
	else if (board.gameply < 80)
	    time_to_use = time_left / 23;
	else
	    time_to_use = time_left / 18;
    }

    time_left -= time_to_use;

    if (search_flags & FLAG_PONDERING) {
	time_to_use += (time_left / 100);
	time_left -= (time_left / 100);
    }

    cur_time = gtime();
    stop_time = cur_time + time_to_use;

    if (!(game_flags & UCI_FLAG)) {
        printf("cur_time: %d    stop_time: %d \n", cur_time, stop_time);
        printf("Using %d.%d  secs out of %d.%d time left\n",
	 	time_to_use / 1000, time_to_use % 1000,
	 	time_left / 1000, time_left % 1000);
    }


    return 0;
}
