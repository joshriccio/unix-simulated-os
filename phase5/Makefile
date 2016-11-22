
TARGET = libphase5.a
ASSIGNMENT = 452phase5
CC = gcc
AR = ar
COBJS = phase5.o p1.o libuser.o
CSRCS = ${COBJS:.o=.c}

PHASE1LIB = patrickphase1
PHASE2LIB = patrickphase2
PHASE3LIB = patrickphase3
PHASE4LIB = patrickphase4
#PHASE1LIB = patrickphase1debug
#PHASE2LIB = patrickphase2debug
#PHASE3LIB = patrickphase3debug
#PHASE4LIB = patrickphase4debug

HDRS = vm.h

INCLUDE = ./usloss/include

CFLAGS = -Wall -g -std=gnu99 -I${INCLUDE} -I.

UNAME := $(shell uname -s)

ifeq ($(UNAME), Darwin)
        CFLAGS += -D_XOPEN_SOURCE
endif

LDFLAGS += -L. -L./usloss/lib

TESTDIR = testcases
TESTS = simple1 simple2 simple3 simple4 simple5

LIBS = $(TESTDIR)/Tconsole.o -l$(PHASE4LIB) -l$(PHASE3LIB) -l$(PHASE2LIB) \
       -l$(PHASE1LIB) -lusloss -l$(PHASE1LIB) -l$(PHASE2LIB) \
       -l$(PHASE3LIB) -l$(PHASE4LIB) -lphase5 

$(TARGET):	$(COBJS)
		$(AR) -r $@ $(COBJS) 

$(TESTS):	$(TARGET)
	$(CC) $(CFLAGS) -c $(TESTDIR)/$@.c
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS)

clean:
	rm -f $(COBJS) $(TARGET) simple?.o simple?  term[0-3].out disk[01]

submit: $(CSRCS) $(HDRS)
	tar cvzf phase5.tgz $(CSRCS) $(HDRS) Makefile
