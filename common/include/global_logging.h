#pragma once
#include <stdio.h>

extern int GLOBAL_LOG_LEVEL;

#define LOG(_level, ...) if(GLOBAL_LOG_LEVEL >= _level) fprintf(stderr, __VA_ARGS__)