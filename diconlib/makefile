CC = gcc
CFLAGS = -Wall -g -fPIC \
	-I./include -I../common/include


OBJ_DIR = obj
LIB_DIR = lib


# SRC

DICONLIB_SRC = $(wildcard src/*.c)
COMMON_SRC = $(wildcard ../common/src/*.c)

DICONLIB_OBJ = $(patsubst src/%.c, $(OBJ_DIR)/%.o, $(DICONLIB_SRC))
COMMON_OBJ = $(patsubst ../common/src/%.c, $(OBJ_DIR)/%.o, $(COMMON_SRC))



OBJ = $(DICONLIB_OBJ) $(COMMON_OBJ)


LIB_STATIC = $(LIB_DIR)/libdiconlib.a
LIB_SHARED = $(LIB_DIR)/libdiconlib.so


# BUILD OBJECTS

$(OBJ_DIR)/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: ../common/src/%.c
	$(CC) $(CFLAGS) -c $< -o $@


# OPTIONS

all: static shared

static: $(OBJ)
	ar rcs $(LIB_STATIC) $(OBJ)
	ranlib $(LIB_STATIC)

shared: $(OBJ)
	$(CC) $(CFLAGS) -shared -o $(LIB_SHARED) $(OBJ)

clean:
	rm -f ./lib/*
	rm -f ./obj/*