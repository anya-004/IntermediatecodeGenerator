CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2

.PHONY: all clean run

all: optimizer

optimizer: optimizer.c optimizer.h lexer.l
	flex -o lex.yy.c lexer.l
	$(CC) $(CFLAGS) optimizer.c lex.yy.c -o optimizer

run: optimizer
	./optimizer input.tac

clean:
	rm -f optimizer lex.yy.c
