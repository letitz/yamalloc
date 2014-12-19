CC=`which gcc`
CPPFLAGS=-DYA_DEBUG
CFLAGS=--std=c11 -ggdb -Werror

all: yatest

test: yatest
	./yatest

%.o: %.c
	$(CC) -c $^ $(CPPFLAGS) $(CFLAGS)

yatest: yatest.o yamalloc.o ya_freelist.o ya_block.o
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o
	rm -f yatest
