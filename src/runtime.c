#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include "../include/riven_runtime.h"

/* --- Internal Structures --- */

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

/* --- Async Task Registry --- */

typedef struct AsyncTask {
    pthread_t thread;
    struct AsyncTask* next;
} AsyncTask;

static AsyncTask* task_list = NULL;
static pthread_mutex_t async_mutex = PTHREAD_MUTEX_INITIALIZER;

/* --- Automatic Reference Counting (ARC) Logic --- */

void rv_retain(RvValue v) {
    if (v.type == RV_TXT && v.as.s_ptr != NULL) {
        v.as.s_ptr->ref_count++;
    }
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
    RvString* rs = (RvString*)malloc(sizeof(RvString));
    rs->ref_count = 1;
    rs->data = strdup(val);
    return (RvValue){.type = RV_TXT, .as.s_ptr = rs, .is_firm = false};
}

RvValue rv_emp() {
    return (RvValue){.type = RV_EMP, .is_firm = false};
}

/* --- Environment and Scope Management --- */

RvEnv* rv_env_create(RvEnv* parent) {
    RvEnv* env = (RvEnv*)malloc(sizeof(RvEnv));
    env->symbols = NULL;
    env->parent = parent;
    return env;
}

void rv_define_var(RvEnv* env, const char* name, RvValue val, bool is_firm) {
    RvSymbol* sym = (RvSymbol*)malloc(sizeof(RvSymbol));
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
            if (curr->is_firm) {
                fprintf(stderr, "Riven Runtime Error: Illegal modification of firm constant '%s'.\n", name);
                exit(1);
            }
            rv_retain(val);
            rv_release(curr->value);
            curr->value = val;
            return;
        }
        curr = curr->next;
    }
    if (env->parent) {
        rv_set_var(env->parent, name, val);
    } else {
        fprintf(stderr, "Riven Runtime Error: Undefined variable '%s' in current context.\n", name);
        exit(1);
    }
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
    fprintf(stderr, "Riven Runtime Error: Cannot access undefined variable '%s'.\n", name);
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

/* --- Operator and Logical Dispatch --- */

RvValue rv_op_binary(RvValue left, RvOp op, RvValue right) {
    RvValue res = rv_emp();
    
    // Numeric Operations
    if ((left.type == RV_INT || left.type == RV_DNUM) && (right.type == RV_INT || right.type == RV_DNUM)) {
        double l = (left.type == RV_INT) ? (double)left.as.i_val : left.as.d_val;
        double r = (right.type == RV_INT) ? (double)right.as.i_val : right.as.d_val;

        switch (op) {
            case OP_ADD: res = (left.type == RV_INT && right.type == RV_INT) ? rv_int((long)l + (long)r) : rv_dnum(l + r); break;
            case OP_SUB: res = (left.type == RV_INT && right.type == RV_INT) ? rv_int((long)l - (long)r) : rv_dnum(l - r); break;
            case OP_MUL: res = (left.type == RV_INT && right.type == RV_INT) ? rv_int((long)l * (long)r) : rv_dnum(l * r); break;
            case OP_DIV: res = rv_dnum(l / r); break;
            case OP_EQ:  res = rv_bool(l == r); break;
            case OP_NEQ: res = rv_bool(l != r); break;
            case OP_GT:  res = rv_bool(l > r);  break;
            case OP_GTE: res = rv_bool(l >= r); break;
            case OP_LT:  res = rv_bool(l < r);  break;
            case OP_LTE: res = rv_bool(l <= r); break;
            default: break;
        }
    } 
    // String Concatenation
    else if (left.type == RV_TXT && right.type == RV_TXT && op == OP_ADD) {
        size_t len = strlen(left.as.s_ptr->data) + strlen(right.as.s_ptr->data) + 1;
        char* buf = (char*)malloc(len);
        strcpy(buf, left.as.s_ptr->data);
        strcat(buf, right.as.s_ptr->data);
        res = rv_txt(buf);
        free(buf);
    }
    // Boolean Logic
    else if (op == OP_AND) {
        res = rv_bool(rv_is_truthy(left) && rv_is_truthy(right));
    } else if (op == OP_OR) {
        res = rv_bool(rv_is_truthy(left) || rv_is_truthy(right));
    }

    rv_release(left);
    rv_release(right);
    return res;
}

bool rv_is_truthy(RvValue v) {
    bool truth = false;
    switch (v.type) {
        case RV_BOOL: truth = v.as.b_val; break;
        case RV_INT:  truth = v.as.i_val != 0; break;
        case RV_EMP:  truth = false; break;
        default:      truth = true; break;
    }
    rv_release(v);
    return truth;
}

/* --- Built-ins and I/O --- */

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

long rv_to_int(RvValue v) {
    long res = 0;
    if (v.type == RV_INT) res = v.as.i_val;
    else if (v.type == RV_DNUM) res = (long)v.as.d_val;
    else if (v.type == RV_TXT) res = atol(v.as.s_ptr->data);
    rv_release(v);
    return res;
}

/* --- OOP/Frame Logic --- */

void rv_register_frame(const char* name) {
    // In a full implementation, this stores Frame metadata in a registry.
}

RvValue rv_spawn(RvEnv* env, const char* name) {
    // Returns a record value simulating an object instance.
    return rv_emp(); 
}

RvValue rv_get_member(RvValue obj, const char* member) {
    // Logic to traverse object environment/v-table.
    return rv_emp();
}

/* --- Async Management --- */

void rv_spark_task(RvValue craft_call) {
    pthread_mutex_lock(&async_mutex);
    AsyncTask* task = (AsyncTask*)malloc(sizeof(AsyncTask));
    
    // In actual codegen, craft_call contains a pointer to a wrapper function.
    // For now, we simulate thread creation for background concurrency.
    // pthread_create(&task->thread, NULL, thread_wrapper, args);
    
    task->next = task_list;
    task_list = task;
    pthread_mutex_unlock(&async_mutex);
}

void rv_sync_tasks() {
    pthread_mutex_lock(&async_mutex);
    AsyncTask* curr = task_list;
    while (curr) {
        pthread_join(curr->thread, NULL);
        AsyncTask* tmp = curr;
        curr = curr->next;
        free(tmp);
    }
    task_list = NULL;
    pthread_mutex_unlock(&async_mutex);
}

/* --- Runtime Lifecycle --- */

void rv_init_runtime() {
    // Initialize standard streams or global registries if needed.
}

void rv_shutdown_runtime() {
    rv_sync_tasks();
}
