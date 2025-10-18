CC       ?= gcc
CFLAGS   ?= -std=c99 -Wall -Wextra -Werror -Wpedantic -O2
CPPFLAGS ?= -Iinclude
LDFLAGS  ?= -lmpg123 -lportaudio

SRC      := $(wildcard src/*.c)
OBJ      := $(SRC:src/%.c=build/%.o)
TARGET   := build/app

.PHONY: all run debug clean print

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@ $(LDFLAGS)

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

build:
	mkdir -p build

run: $(TARGET)
	./$(TARGET)

debug: CFLAGS := -std=c99 -Wall -Wextra -Werror -Wpedantic -g -O0

debug: clean $(TARGET)

clean:
	rm -rf build $(TARGET)

print:
	@echo "SRC:  $(SRC)"
	@echo "OBJ:  $(OBJ)"
