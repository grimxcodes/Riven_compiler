#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include "../include/riven_runtime.h"

typedef struct RvSymbol {
    char* name;
    RvValue value;
    bool is_firm;
    struct RvSymbol* next;
} RvSymbol;

struct RvEnv {
    RvSymbol* symbols;
    struct RvEnv* parent;
};

void rv_retain(RvValue v) {
    if (v.type == RV_TXT && v.as.s_ptr != NULL) v.as.s_ptr->ref_count++;
}

void rv_release(RvValue v) {
    if (v.type == RV_TXT && v.as.s_ptr != NULL) {
        v.as.s_ptr->ref_count--;
        if (v.as.s_ptr->ref_count <= 0) {
            free(v.as.s_ptr->data);
            free(v.as.s_ptr);
        }
    }
}

RvValue rv_int(long val) { return (RvValue){.type = RV_INT, .as.i_val = val}; }
RvValue rv_dnum(double val) { return (RvValue){.type = RV_DNUM, .as.d_val = val}; }
RvValue rv_bool(bool val) { return (RvValue){.type = RV_BOOL, .as.b_val = val}; }
RvValue rv_txt(const char* val) {
    RvString* rs = malloc(sizeof(RvString));
    rs->ref_count = 1;
    rs->data = strdup(val);
    return (RvValue){.type = RV_TXT, .as.s_ptr = rs};
}
RvValue rv_emp() { return (RvValue){.type = RV_EMP}; }

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
    sym->is_firm = is_firm;
    sym->next = env->symbols;
    env->symbols = sym;
}

void rv_set_var(RvEnv* env, const char* name, RvValue val) {
    RvSymbol* curr = env->symbols;
    while (curr) {
        if (strcmp(curr->name, name) == 0) {
            if (curr->is_firm) { fprintf(stderr, "Error: Firm constant '%s' modified.\n", name); exit(1); }
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
        if (strcmp(curr->name, name) == 0) { rv_retain(curr->value); return curr->value; }
        curr = curr->next;
    }
    if (env->parent) return rv_get_var(env->parent, name);
    fprintf(stderr, "Error: Undefined variable '%s'.\n", name); exit(1);
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

RvValue rv_op_unary(RvOp op, RvValue operand) {
    RvValue res = rv_emp();
    if (op == OP_TO_TXT) {
        char buf[128];
        if (operand.type == RV_INT) sprintf(buf, "%ld", operand.as.i_val);
        else if (operand.type == RV_DNUM) sprintf(buf, "%g", operand.as.d_val);
        else if (operand.type == RV_BOOL) sprintf(buf, "%s", operand.as.b_val ? "correct" : "incorrect");
        else strcpy(buf, "emp");
        res = rv_txt(buf);
    } else if (op == OP_NEG && operand.type == RV_INT) {
        res = rv_int(-operand.as.i_val);
    } else if (op == OP_NOT) {
        res = rv_bool(!rv_is_truthy(operand));
    }
    rv_release(operand);
    return res;
}

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
            case OP_GTE: res = rv_bool(left.as.i_val >= right.as.i_val); break;
            case OP_LT:  res = rv_bool(left.as.i_val < right.as.i_val); break;
            case OP_LTE: res = rv_bool(left.as.i_val <= right.as.i_val); break;
            default: break;
        }
    } else if (left.type == RV_TXT && right.type == RV_TXT && op == OP_ADD) {
        char* b = malloc(strlen(left.as.s_ptr->data) + strlen(right.as.s_ptr->data) + 1);
        strcpy(b, left.as.s_ptr->data); strcat(b, right.as.s_ptr->data);
        res = rv_txt(b); free(b);
    }
    rv_release(left); rv_release(right);
    return res;
}

bool rv_is_truthy(RvValue v) {
    bool t = (v.type == RV_BOOL) ? v.as.b_val : (v.type != RV_EMP);
    rv_release(v); return t;
}

void rv_imprint(RvValue v) {
    if (v.type == RV_INT) printf("%ld\n", v.as.i_val);
    else if (v.type == RV_TXT) printf("%s\n", v.as.s_ptr->data);
    else if (v.type == RV_BOOL) printf("%s\n", v.as.b_val ? "correct" : "incorrect");
    rv_release(v);
}

long rv_to_int(RvValue v) {
    long r = (v.type == RV_INT) ? v.as.i_val : 0;
    rv_release(v); return r;
}

void rv_init_runtime() {}
void rv_shutdown_runtime() {}
