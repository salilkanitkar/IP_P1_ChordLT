SRCS =  chord_server.c chord_util.c 
INC = chord_server.h 
OBJS = $(SRCS:.c=.o)

CFLAGS = -Werror -I.
LDFLAGS = -L.
EXTRA_CFLAGS = -g
DEBUG=0 

CC = gcc

ifeq ($(DEBUG),1)
EXTRA_CFLAGS += -DDEBUG_FLAG
endif

all: myserver myclient

myserver: mycompile mylink

myclient: client

mycompile: $(OBJS) $(INC)

%.o: %.c $(INC)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -lpthread -o $@ -c $<

mylink: $(OBJS) $(INC)
	$(CC) -lpthread -o chord_peer $(CFLAGS) $(EXTRA_CFLAGS) $(OBJS)

client: 
	$(CC) $(CFLAGS) $(LDFLAGS) $(EXTRA_CFLAG) -o chord_client chord_client.c

clean:
	rm -f $(OBJS) chord_peer chord_client *~
tags:
	find . -name "*.[cChH]" | xargs ctags
	find . -name "*.[cChH]" | etags -

