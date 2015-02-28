SOURCES := $(wildcard src/*.c)
OBJECTS = $(patsubst %.c,%.o,$(SOURCES))

CC := clang
CFLAGS := -std=c99 -g -Wall -Wextra -pedantic

all: ced

release: CFLAGS += -O2
release: ced

ced: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS)

clean: $(OBJECTS)
	rm src/*.o

%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<
