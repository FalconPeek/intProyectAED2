CC=cc
CFLAGS=-std=c99 -Wall -Wextra -O2 -Iinclude
SRC=$(wildcard src/*.c)
OBJ=$(SRC:.c=.o)

all: sim_hospital
sim_hospital: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f src/*.o sim_hospital
