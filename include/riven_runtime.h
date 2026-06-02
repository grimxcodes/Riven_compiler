#ifndef RIVEN_RUNTIME_H
#define RIVEN_RUNTIME_H

#include <stdbool.h>

/* --- Runtime Types --- */

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

/**
 * The core value structure used at runtime.
 * Implements a tagged union with metadata for ARC and immutability.
 */
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

/* --- Memory Management (ARC) API --- */

void rv_retain(RvValue v);
void rv_release(RvValue v);

/* --- Environment & Scoping --- */

struct RvEnv;
typedef struct RvEnv RvEnv;

RvEnv* rv_env_create(RvEnv* parent);
void rv_env_free(RvEnv* env);
void rv_define_var(RvEnv* env, const char* name, RvValue val, bool is_firm);
void rv_set_var(RvEnv* env, const char* name, RvValue val);
RvValue rv_get_var(RvEnv* env, const char* name);

/* --- Value Constructors --- */

RvValue rv_int(long val);
RvValue rv_dnum(double val);
RvValue rv_bool(bool val);
RvValue rv_txt(const char* val);
RvValue rv_emp(void);

/* --- Built-in Functions --- */

void rv_imprint(RvValue v);
long rv_to_int(RvValue v);
const char* rv_to_txt(RvValue v);

/* --- Operators --- */

typedef enum { 
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, 
    OP_EQ, OP_NEQ, OP_GT, OP_LT, 
    OP_AND, OP_OR, OP_NEG, OP_NOT 
} RvOp;

RvValue rv_op_binary(RvValue left, RvOp op, RvValue right);
RvValue rv_op_unary(RvOp op, RvValue operand);
bool rv_is_truthy(RvValue v);

/* --- Lifecycle --- */

void rv_init_runtime(void);
void rv_shutdown_runtime(void);

#endif /* RIVEN_RUNTIME_H */
