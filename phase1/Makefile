
TARGET = libphase1.a
ASSIGNMENT = 452phase1
CC = gcc
AR = ar

COBJS = phase1.o
CSRCS = ${COBJS:.o=.c}

HDRS = kernel.h phase1.h

INCLUDE = ./usloss/include

CFLAGS = -Wall -g -I${INCLUDE}

UNAME := $(shell uname -s)

ifeq ($(UNAME), Darwin)
	CFLAGS += -D_XOPEN_SOURCE      # use for Mac, NOT for Linux!!
endif

LDFLAGS = -L. -L./usloss/lib

TESTDIR = testcases
TESTS = test00 test01 test02 test03 test04 test05 test06 test07 test08 \
        test09 test10 test11 test12 
LIBS = -lphase1 -lusloss

$(TARGET):	$(COBJS)
		$(AR) -r $@ $(COBJS) 

$(TESTS):	$(TARGET) p1.o
	$(CC) $(CFLAGS) -I. -c $(TESTDIR)/$@.c
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS) p1.o

clean:
	rm -f $(COBJS) $(TARGET) p1.o test??.o test?? test??.txt core term*.out

phase1.o:	kernel.h

submit:	$(CSRCS) $(HDRS) Makefile
	tar cvzf phase1.tgz $(CSRCS) $(HDRS) Makefile
