.PHONY: all clean

#compiler c
CC = gcc
CFLAGS = -D_POSIX_SOURCE -Wall -Werror -pedantic -std=c99 -D_GNU_SOURCE -pthread

###################################

#c files
SOURCE = $(wildcard *.c) #all .c files
HEADER = $(SOURCE:.c=.h)
OBJS = $(SOURCE:.c=.o)
TARGET = $(SOURCE:.c=)

###################################

#standard rules
all: client serverSingle serverThreaded

serverSingle: serverThreaded
	cp serverThreaded serverSingle

clean:
	@rm -f $(OBJS) $(TARGET) serverSingle

###################################

#overwrite suffix rules to enforce our rules
.SUFFIXES:

%: %.o
	@$(CC) $(CFLAGS) -o $@ $<

%.o: %.c
	@$(CC) -c $(CFLAGS) $<

#EOF
