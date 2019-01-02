/*
    Diablo chess engine, Copyright 2005, Marcus Prewarski
          
    This program is free software. It may be copied or modified under the terms
    of the GNU General Public License version 2 as published by the Free Software 
    Foundation.  This program is distributed without any warranties whatsoever.
    See the file COPYING included with the distribution for details of the GNU general 
    public license.
*/
#ifndef WIN32
#include <unistd.h>
#include <sys/time.h>
#else
#include <windows.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>

#include "diablo.h"

int main(int argc, char *argv[])
{

    printf("\n\t %s %s \n\n", NAME, VERSION);

    setbuf(stdout, NULL);
    init_diablo();

    while(1) {
	readline(inputline, sizeof(inputline), stdin);
	uci_command(inputline);
    }

    return 0;
}
