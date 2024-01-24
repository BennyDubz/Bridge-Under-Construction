# 
# Ben Williams '25, Makefile for bridge project

EXECS = bridge
OBJS = bridge.c

CFLAGS = -g -pthread -Wall

all: $(EXECS)

$(EXECS) : $(OBJS)
	gcc $(CFLAGS) -o $@ $^

bridge.o : bridge.c

test:
	./testing.sh

clean:
	rm -f *.o *~ $(EXECS)

