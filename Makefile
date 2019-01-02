#
# Makefile
#

LIBS = 

default: diablo

OFILES = attack.o \
	data.o \
	eval.o \
	genmoves.o \
	hash.o \
	init.o \
	io.o \
	main.o \
	move.o \
	order.o \
	search.o \
	see.o \
	test.o \
	think.o \
	time.o \
	uci.o \
	util.o

$(OFILES): diablo.h

diablo:  $(OFILES) 
	gcc -o diablo $(OFILES)  $(LIBS)

clean:
	rm -f *.o  diablo.exe

.c.o:   main.c
	gcc -c -g $*.c  -D_WINDOWS_ -Wall -O2
#	gcc -c -g $*.c  -D_WINDOWS_ -Wall
#	gcc -c $*.c  -D_WINDOWS_ -Wall -O2

