all: a.out
.PHONY: all clean

a.out: 1.o 2.o
	gcc $^ -o $@

CFLAGS=-Wall -Werror -MD
-include *.d

clean:
	rm -rf *.o a.out
