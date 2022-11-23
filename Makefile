#
# Students' Makefile for the Malloc Lab
#
TEAM = bovik
VERSION = 1
HANDINDIR = /afs/cs.cmu.edu/academic/class/15213-f01/malloclab/handin

CC = gcc
CFLAGS = -m32 -g 

MDRIVER_OBJS = mdriver.o mm.o memlib.o fsecs.o fcyc.o clock.o ftimer.o
MM_OBJS = mm_main.o memlib.o

mdriver: $(MDRIVER_OBJS)
	$(CC) $(CFLAGS) -o mdriver $(MDRIVER_OBJS)

mm: $(MM_OBJS)
	$(CC) $(CFLAGS) -o mm $(MM_OBJS)

mdriver.o: mdriver.c fsecs.h fcyc.h clock.h memlib.h config.h mm.h
memlib.o: memlib.c memlib.h
# For mm main debuging
mm_main.o: mm.c mm.h memlib.h
	$(CC) $(CFLAGS) -c -o mm_main.o -DMAIN mm.c

# mm without main
mm.o: mm-cmu.c mm.h memlib.h
	$(CC) $(CFLAGS) -c -o mm.o mm-cmu.c
mm2.o: mm2.c mm.h memlib.h
fsecs.o: fsecs.c fsecs.h config.h
fcyc.o: fcyc.c fcyc.h
ftimer.o: ftimer.c ftimer.h config.h
clock.o: clock.c clock.h

# mm.c: mm.c
# 	$(CC) -DMAIN -E mm.c
handin:
	cp mm.c $(HANDINDIR)/$(TEAM)-$(VERSION)-mm.c

clean:
	rm -f *~ *.o mdriver


