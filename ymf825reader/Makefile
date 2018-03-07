PROGRAM = ymf825
OBJS = main.o common.o spi.o ymf825.o
CC = gcc
CFLAGS = -Wall
FLAGS = -L. -lftd2xx -lm -Wl,-rpath /usr/local/lib

.PHONY: all
all: $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CC) $(FLAGS) -o $(PROGRAM) $^

.c.o:
	$(CC) $< $(CFLAGS) -c

.PHONY: clean
clean:
	$(RM) $(PROGRAM) $(OBJS)
