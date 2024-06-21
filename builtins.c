#include <stdio.h>
#include <string.h>
#include "builtins.h"

unsigned hash(const char* str) {
    unsigned h = 0;
    while (*str) h = 31 * h + (unsigned char)(*str++);
    return h;
}

void builtin_register(BuiltinTable* table, const char* name, BuiltinFunc handler) {
    unsigned idx = hash(name) % MAX_BUILTINS;
    for (int i = 0; i < MAX_BUILTINS; ++i) {
        unsigned probe = (idx + i) % MAX_BUILTINS;
        if (!table->occupied[probe]) {
            table->table[probe].name = name;
            table->table[probe].handler = handler;
            table->occupied[probe] = true;
            return;
        }
    }
    fprintf(stderr, "Builtin table full\n");
}

BuiltinCommand* builtin_lookup(BuiltinTable* table, const char* name) {
    unsigned idx = hash(name) % MAX_BUILTINS;
    for (int i = 0; i < MAX_BUILTINS; ++i) {
        unsigned probe = (idx + i) % MAX_BUILTINS;
        if (!table->occupied[probe]) return NULL;
        if (strcmp(table->table[probe].name, name) == 0) {
            return &table->table[probe];
        }
    }
    return NULL;
}
