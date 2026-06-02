#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* --- Runtime Type Definitions --- */

typedef enum {
    RV_INT,
    RV_DNUM,
    RV_TXT,
    RV_BOOL,
    RV_EMP
} RvType;

typedef struct {
    int ref_count;
    char* data;
} RvString;

typedef struct {
    RvType type;
    bool is_firm;
    union {
        long i_val;
        double d_val;
        bool b_val;
        RvString* s_ptr;
    } as;
} RvValue;

/* --- Memory Management (ARC) --- */

static void rv_retain(RvValue v) {
    if (v.type == RV_TXT && v.as.s_ptr != NULL) {
        v.as.s_ptr->ref_count++;
    }
}

static void rv_release(RvValue v) {
    if (v.type == RV_TXT && v.as.s_ptr != NULL) {
        v.as.s_ptr->ref_count--;
        if (v.as.s_ptr->ref_count <= 0) {
            free(v.as.s_ptr->data);
            free(v.as.s_ptr);
        }
    }
}

/* --- Value Constructors --- */

RvValue rv_int(long val) {
    return (RvValue){.type = RV_INT, .as.i_val = val, .is_firm = false};
}

RvValue rv_dnum(double val) {
    return (RvValue){.type = RV_DNUM, .as.d_val = val, .is_firm = false};
}

RvValue rv_bool(bool val) {
    return (RvValue){.type = RV_BOOL, .as.b_val = val, .is_firm = false};
}

RvValue rv_txt(const char* val) {
    RvString* rs = malloc(sizeof(RvString));
    rs->ref_count = 1;
    rs->data = strdup(val);
    return (RvValue){.type = RV_TXT, .as.s_ptr = rs, .is_firm = false};
}

RvValue rv_emp() {
    return (RvValue){.type = RV_EMP, .is_firm = false};
}

/* --- Environment & Scoping --- */

typedef struct RvSymbol {
    char* name;
    RvValue value;
    struct RvSymbol* next;
} RvSymbol;

typedef struct RvEnv {
    RvSymbol* symbols;
    struct RvEnv* parent;
} RvEnv;

RvEnv* rv_env_create(RvEnv* parent) {
    RvEnv* env = malloc(sizeof(RvEnv));
    env->symbols = NULL;
    env->parent = parent;
    return env;
}

void rv_define_var(RvEnv* env, const char* name, RvValue val, bool is_firm) {
    RvSymbol* sym = malloc(sizeof(RvSymbol));
    sym->name = strdup(name);
    rv_retain(val);
    sym->value = val;
    sym->value.is_firm = is_firm;
    sym->next = env->symbols;
    env->symbols = sym;
}

void rv_set_var(RvEnv* env, const char* name, RvValue val) {
    RvSymbol* curr = env->symbols;
    while (curr) {
        if (strcmp(curr->name, name) == 0) {
            if (curr->value.is_firm) {
                fprintf(stderr, "Runtime Error: Cannot modify firm constant '%s'\n", name);
                exit(1);
            }
            rv_retain(val);
            rv_release(curr->value);
            curr->value = val;
            return;
        }
        curr = curr->next;
    }
    if (env->parent) rv_set_var(env->parent, name, val);
}

RvValue rv_get_var(RvEnv* env, const char* name) {
    RvSymbol* curr = env->symbols;
    while (curr) {
        if (strcmp(curr->name, name) == 0) {
            rv_retain(curr->value);
            return curr->value;
        }
        curr = curr->next;
    }
    if (env->parent) return rv_get_var(env->parent, name);
    fprintf(stderr, "Runtime Error: Undefined variable '%s'\n", name);
    exit(1);
}

void rv_env_free(RvEnv* env) {
    RvSymbol* curr = env->symbols;
    while (curr) {
        RvSymbol* next = curr->next;
        rv_release(curr->value);
        free(curr->name);
        free(curr);
        curr = next;
    }
    free(env);
}

/* --- Built-ins --- */

void rv_imprint(RvValue v) {
    switch (v.type) {
        case RV_INT:  printf("%ld\n", v.as.i_val); break;
        case RV_DNUM: printf("%g\n", v.as.d_val); break;
        case RV_BOOL: printf("%s\n", v.as.b_val ? "correct" : "incorrect"); break;
        case RV_TXT:  printf("%s\n", v.as.s_ptr->data); break;
        case RV_EMP:  printf("emp\n"); break;
    }
    rv_release(v);
}

/* --- Operator Logic --- */

typedef enum { OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_EQ, OP_NEQ, OP_GT, OP_LT, OP_AND, OP_OR } RvOp;

RvValue rv_op_binary(RvValue left, RvOp op, RvValue right) {
    RvValue res = rv_emp();
    if (left.type == RV_INT && right.type == RV_INT) {
        switch (op) {
            case OP_ADD: res = rv_int(left.as.i_val + right.as.i_val); break;
            case OP_SUB: res = rv_int(left.as.i_val - right.as.i_val); break;
            case OP_MUL: res = rv_int(left.as.i_val * right.as.i_val); break;
            case OP_DIV: res = rv_int(left.as.i_val / right.as.i_val); break;
            case OP_EQ:  res = rv_bool(left.as.i_val == right.as.i_val); break;
            case OP_GT:  res = rv_bool(left.as.i_val > right.as.i_val); break;
            case OP_LT:  res = rv_bool(left.as.i_val < right.as.i_val); break;
            default: break;
        }
    } else if (left.type == RV_TXT && right.type == RV_TXT && op == OP_ADD) {
        char* buf = malloc(strlen(left.as.s_ptr->data) + strlen(right.as.s_ptr->data) + 1);
        strcpy(buf, left.as.s_ptr->data);
        strcat(buf, right.as.s_ptr->data);
        res = rv_txt(buf);
        free(buf);
    }
    /* Dynamic memory release of operands after operation */
    rv_release(left);
    rv_release(right);
    return res;
}

bool rv_is_truthy(RvValue v) {
    bool res = false;
    if (v.type == RV_BOOL) res = v.as.b_val;
    else if (v.type == RV_INT) res = v.as.i_val != 0;
    else if (v.type == RV_EMP) res = false;
    else res = true;
    rv_release(v);
    return res;
}

/* --- Initialization --- */

void rv_init_runtime() {
    /* Potential initialization for sockets/threads */
}

void rv_shutdown_runtime() {
    /* Cleanup logic */
}
