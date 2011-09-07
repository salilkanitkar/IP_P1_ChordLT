SRCS =  chord_server.c 
INC = chord_server.h 
OBJS = $(SRCS:.c=.o)

CFLAGS = -Wall -Werror -I.
LDFLAGS = -L.
EXTRA_CFLAGS = -g
DEBUG=0 

CC = gcc

ifeq ($(DEBUG),1)
EXTRA_CFLAGS += -DDEBUG_FLAG
endif

all: myserver 

myserver: mycompile mylink

mycompile: $(OBJS) $(INC)

%.o: %.c $(INC)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<

mylink: $(OBJS) $(INC)
	$(CC) -o chord_peer $(CFLAGS) $(EXTRA_CFLAGS) $(OBJS)

clean:
	rm -f $(OBJS) chord_peer *~
tags:
	find . -name "*.[cChH]" | xargs ctags
	find . -name "*.[cChH]" | etags -

