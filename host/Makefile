.PHONY: all clean

CC := gcc
CFLAGS := -std=gnu11

all: host

clean:
	-@$(RM) host

host: main.o
	$(CC) -o $@ $<
