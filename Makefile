## AdivinaNro+ Makefile (strict C99, no extern libs)

CC       ?= gcc
CFLAGS   ?= -std=c99 -Wall -Wextra -Werror -Wpedantic -O2
CPPFLAGS ?= -Iinclude
LDFLAGS  ?=

TARGET   := main
##SRC      := $(wildcard src/*.c)
SRC := $(filter-out src/persist.c,$(wildcard src/*.c))
OBJ      := $(SRC:src/%.c=build/%.o)

.PHONY: all run debug clean print

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@ $(LDFLAGS)

# Pattern rule for object files
build/%.o: src/%.c | build
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

build:
	mkdir -p build

run: $(TARGET)
	./$(TARGET)

# Debug build: rebuild with debug flags
debug: CFLAGS := -std=c99 -Wall -Wextra -Werror -Wpedantic -g -O0
debug: clean $(TARGET)

clean:
	rm -rf build $(TARGET)

print:
	@echo "SRC:  $(SRC)"
	@echo "OBJ:  $(OBJ)"
