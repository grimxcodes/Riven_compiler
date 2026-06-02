#include "../include/riven.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * Creates a new environment scope.
 * If parent is NULL, this becomes the global scope.
 */
Environment* env_create(Environment* parent) {
    Environment* env = (Environment*)malloc(sizeof(Environment));
    if (env == NULL) {
        fprintf(stderr, "Fatal Error: Out of memory while creating semantic environment.\n");
        exit(1);
    }

    env->capacity = 16;
    env->count = 0;
    env->symbols = (Symbol*)malloc(sizeof(Symbol) * env->capacity);
    if (env->symbols == NULL) {
        fprintf(stderr, "Fatal Error: Out of memory while allocating symbol table.\n");
        exit(1);
    }

    env->parent = parent;
    return env;
}

/**
 * Frees the environment and all symbol metadata within it.
 * Note: Does not free parent environments.
 */
void env_free(Environment* env) {
    if (env == NULL) return;

    for (int i = 0; i < env->count; i++) {
        free(env->symbols[i].name);
    }
    free(env->symbols);
    free(env);
}

/**
 * Adds a symbol to the current scope.
 * If the symbol already exists in this specific scope, it updates the metadata.
 */
void env_add_symbol(Environment* env, const char* name, bool is_firm, bool is_public) {
    /* Check if already exists in this specific scope */
    for (int i = 0; i < env->count; i++) {
        if (strcmp(env->symbols[i].name, name) == 0) {
            env->symbols[i].is_firm = is_firm;
            env->symbols[i].is_public = is_public;
            return;
        }
    }

    /* Resize if capacity reached */
    if (env->count >= env->capacity) {
        env->capacity *= 2;
        env->symbols = (Symbol*)realloc(env->symbols, sizeof(Symbol) * env->capacity);
        if (env->symbols == NULL) {
            fprintf(stderr, "Fatal Error: Out of memory during symbol table expansion.\n");
            exit(1);
        }
    }

    /* Add new symbol */
    env->symbols[env->count].name = strdup(name);
    env->symbols[env->count].is_firm = is_firm;
    env->symbols[env->count].is_public = is_public;
    env->count++;
}

/**
 * Recursively checks if a symbol exists in the current or any parent scope.
 */
bool env_exists(Environment* env, const char* name) {
    if (env == NULL) return false;

    for (int i = 0; i < env->count; i++) {
        if (strcmp(env->symbols[i].name, name) == 0) {
            return true;
        }
    }

    return env_exists(env->parent, name);
}

/**
 * Finds and returns the metadata for a symbol.
 * Returns NULL if the symbol is not found in any accessible scope.
 */
Symbol* env_find_symbol(Environment* env, const char* name) {
    if (env == NULL) return NULL;

    for (int i = 0; i < env->count; i++) {
        if (strcmp(env->symbols[i].name, name) == 0) {
            return &env->symbols[i];
        }
    }

    return env_find_symbol(env->parent, name);
}

/**
 * Validates if an assignment is legal.
 * Checks for variable existence and 'firm' status.
 */
bool env_validate_assignment(Environment* env, const char* name) {
    Symbol* sym = env_find_symbol(env, name);
    if (sym == NULL) {
        fprintf(stderr, "Semantic Error: Attempting to assign to undefined variable '%s'.\n", name);
        return false;
    }

    if (sym->is_firm) {
        fprintf(stderr, "Semantic Error: Cannot modify firm constant '%s'.\n", name);
        return false;
    }

    return true;
}
