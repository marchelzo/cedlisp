SOURCES := $(wildcard src/*.c)
OBJECTS = $(patsubst %.c,%.o,$(SOURCES))

CC := clang
CCFLAGS := -std=c99 -g -Wall -Wextra -pedantic -O0

ced: $(OBJECTS)
	$(CC) $(CCFLAGS) -o $@ $(OBJECTS)

%.o : %.c
	$(CC) $(CCFLAGS) -c -o $@ $<
