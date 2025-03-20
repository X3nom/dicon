CC = gcc
CFLAGS = -Wall \
	-I./include -I../common/include

EXEC_NAME = main

OBJ_DIR = obj
BIN_DIR = bin


# SRC

NODE_SRC = $(wildcard src/*.c)
COMMON_SRC = $(wildcard ../common/src/*.c)

NODE_OBJ = $(patsubst src/%.c, $(OBJ_DIR)/%.o, $(NODE_SRC))
COMMON_OBJ = $(patsubst ../common/src/%.c, $(OBJ_DIR)/%.o, $(COMMON_SRC))



OBJ = $(NODE_OBJ) $(COMMON_OBJ)


# BUILD OBJECTS

$(OBJ_DIR)/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: ../common/src/%.c
	$(CC) $(CFLAGS) -c $< -o $@


# OPTIONS

all: normal

normal: $(OBJ)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$(EXEC_NAME) $(OBJ)

clean:
	rm -f ./lib/*
	rm -f ./obj/*