.PHONY: all clean

#compiler c
CC = gcc
CFLAGS = -D_POSIX_SOURCE -Wall -pedantic -std=c99 -g

###################################

#c files
SOURCE = $(wildcard *.c) #all .c files
HEADER = $(SOURCE:.c=.h)
OBJS = $(SOURCE:.c=.o)
TARGET = test_list1 test_list2 test_list3 test_list4



###################################

#standard rules
all: $(TARGET)

%: %.o linkedlist.o
	$(CC) -o $@ $< linkedlist.o

clean:
	rm -f $(OBJS) $(TARGET)

###################################

#overwrite suffix rules to enforce our rules
.SUFFIXES:

%.o: %.c
	$(CC) -c $(CFLAGS) $<

#EOF
