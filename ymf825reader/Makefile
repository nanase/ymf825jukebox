PROGRAM = ymf825
OBJS = main.o spi.o delay.o ymf825.o
CC = gcc
CFLAGS = -Wall
FLAGS = -lbcm2835 -lm -O3

.PHONY: all
all: $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CC) -o $(PROGRAM) $^ $(FLAGS) 

.c.o:
	$(CC) $< $(CFLAGS) -c

.PHONY: clean
clean:
	$(RM) $(PROGRAM) $(OBJS)
