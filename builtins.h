#ifndef BUILTINS_H
#define BUILTINS_H

#include <stdbool.h>

#define MAX_BUILTINS 32

typedef void (*BuiltinFunc)(char* input);

typedef struct {
    const char* name;
    BuiltinFunc handler;
} BuiltinCommand;

typedef struct {
    BuiltinCommand table[MAX_BUILTINS];
    bool occupied[MAX_BUILTINS];
} BuiltinTable;

unsigned hash(const char* str);
void builtin_register(BuiltinTable* table, const char* name, BuiltinFunc handler);
BuiltinCommand* builtin_lookup(BuiltinTable* table, const char* name);

#endif
