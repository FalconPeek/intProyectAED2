# ==== Configuración =====
PROJECT := sim_hospital

# Si no definís CC afuera, usa cc. Podés exportar CC=gcc o CC=clang
CC ?= cc

CSTD := c99
CPPFLAGS ?= -Iinclude                # rutas a headers
CFLAGS   ?= -std=$(CSTD) -Wall -Wextra -Wpedantic -O2
LDFLAGS  ?=
LDLIBS   ?=                          # ej: -lm

# ==== Fuentes / Objetos / Deps ====
SRC  := $(wildcard src/*.c)
OBJ  := $(SRC:src/%.c=build/%.o)
DEP  := $(OBJ:.o=.d)

# ==== Targets por defecto ====
.PHONY: all clean run debug info test

all: $(PROJECT)

# Carpeta build (se crea una vez)
build:
	@mkdir -p build

# Link final
$(PROJECT): build $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJ) $(LDLIBS)

# Regla de compilación con deps automáticas (-MMD -MP)
build/%.o: src/%.c | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

# Incluir archivos .d generados (no falla si no existen todavía)
-include $(DEP)

# === Utilitarios ===
run: $(PROJECT)
	./$(PROJECT)

debug: CFLAGS += -g -O0
debug: clean $(PROJECT)

info:
	@echo "CC       = $(CC)"
	@echo "CSTD     = $(CSTD)"
	@echo "CPPFLAGS = $(CPPFLAGS)"
	@echo "CFLAGS   = $(CFLAGS)"
	@echo "LDFLAGS  = $(LDFLAGS)"
	@echo "LDLIBS   = $(LDLIBS)"
	@echo "SRC      = $(SRC)"
	@echo "OBJ      = $(OBJ)"

test: $(PROJECT)
	@echo ">> Acá corré tus tests (tests/*)."

clean:
	rm -rf build $(PROJECT)
