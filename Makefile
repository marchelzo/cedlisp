SOURCES := $(wildcard src/*.c)
OBJECTS = $(patsubst %.c,%.o,$(SOURCES))

CC := clang
CFLAGS := -std=c99 -g -Wall -Wextra -pedantic

all: ced repl

release: CFLAGS += -O2
release: ced

repl: repl.c
	$(CC) $(CFLAGS) -o $@ $< src/eval.o src/parser.o src/environment.o src/builtins.o -lreadline

ced: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS)

clean: $(OBJECTS)
	rm src/*.o

%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: repl
