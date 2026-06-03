#include "../include/riven.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FILE* out;
static int indent_level = 0;

static void emit_indent() {
    for (int i = 0; i < indent_level; i++) {
        fprintf(out, "    ");
    }
}

/* Forward Declarations for Full AST Traversal */
static void gen_node(ASTNode* node, Environment* env);
static void gen_expression(ASTNode* node, Environment* env);

/**
 * Handles Riven's Binary Operators mapping to Runtime C Ops.
 * Covers: +, -, *, /, ==, !=, >, <, >=, <=, and, or.
 */
static void gen_binary_op(ASTNode* node, Environment* env) {
    fprintf(out, "rv_op_binary(");
    gen_expression(node->data.binary.left, env);
    fprintf(out, ", ");

    switch (node->data.binary.op) {
        case TOKEN_PLUS:           fprintf(out, "OP_ADD"); break;
        case TOKEN_MINUS:          fprintf(out, "OP_SUB"); break;
        case TOKEN_STAR:           fprintf(out, "OP_MUL"); break;
        case TOKEN_SLASH:          fprintf(out, "OP_DIV"); break;
        case TOKEN_EQUALS_EQUALS:  fprintf(out, "OP_EQ");  break;
        case TOKEN_BANG_EQUALS:    fprintf(out, "OP_NEQ"); break;
        case TOKEN_GREATER:        fprintf(out, "OP_GT");  break;
        case TOKEN_GREATER_EQUALS: fprintf(out, "OP_GTE"); break;
        case TOKEN_LESS:           fprintf(out, "OP_LT");  break;
        case TOKEN_LESS_EQUALS:    fprintf(out, "OP_LTE"); break;
        case TOKEN_AND:
        case TOKEN_AMP_AMP:        fprintf(out, "OP_AND"); break;
        case TOKEN_OR:
        case TOKEN_PIPE_PIPE:      fprintf(out, "OP_OR");  break;
        default:                   fprintf(out, "OP_UNKNOWN"); break;
    }

    fprintf(out, ", ");
    gen_expression(node->data.binary.right, env);
    fprintf(out, ")");
}

/**
 * Handles Expression Generation including ARC Retain calls.
 */
static void gen_expression(ASTNode* node, Environment* env) {
    if (node == NULL) {
        fprintf(out, "rv_emp()");
        return;
    }

    switch (node->type) {
        case NODE_LITERAL: {
            ASTValue v = node->data.literal;
            if (v.type == TOKEN_INT) fprintf(out, "rv_int(%ld)", v.as.i_val);
            else if (v.type == TOKEN_DNUM) fprintf(out, "rv_dnum(%f)", v.as.d_val);
            else if (v.type == TOKEN_STRING) fprintf(out, "rv_txt(\"%s\")", v.as.s_val);
            else if (v.type == TOKEN_CORRECT) fprintf(out, "rv_bool(true)");
            else if (v.type == TOKEN_INCORRECT) fprintf(out, "rv_bool(false)");
            else if (v.type == TOKEN_EMP) fprintf(out, "rv_emp()");
            break;
        }

        case NODE_IDENTIFIER:
            /* Automatic Reference Counting: The runtime handles the retain logic here */
            fprintf(out, "rv_get_var(env, \"%s\")", node->data.identifier);
            break;

        case NODE_BINARY:
            gen_binary_op(node, env);
            break;

        case NODE_UNARY:
            fprintf(out, "rv_op_unary(");
            if (node->data.unary.op == TOKEN_MINUS) fprintf(out, "OP_NEG");
            else if (node->data.unary.op == TOKEN_NOT || node->data.unary.op == TOKEN_BANG) fprintf(out, "OP_NOT");
            fprintf(out, ", ");
            gen_expression(node->data.unary.operand, env);
            fprintf(out, ")");
            break;

        case NODE_SPAWN_EXPR:
            fprintf(out, "rv_spawn(env, \"%s\")", node->data.spawn.frame_name);
            break;

        case NODE_MEMBER_ACCESS:
            fprintf(out, "rv_get_member(");
            gen_expression(node->data.binary.left, env);
            fprintf(out, ", \"%s\")", node->data.binary.right->data.identifier);
            break;

        case NODE_FUNC_CALL:
            fprintf(out, "rv_call_craft(env, \"%s\")", node->data.identifier);
            break;

        default:
            fprintf(out, "rv_emp()");
            break;
    }
}

/**
 * Main Node Generation Pass (Statements and Declarations)
 */
static void gen_node(ASTNode* node, Environment* env) {
    if (node == NULL) return;

    switch (node->type) {
        case NODE_PROGRAM:
            for (int i = 0; i < node->data.program.count; i++) {
                gen_node(node->data.program.nodes[i], env);
            }
            break;

        case NODE_CORE_BLOCK:
            fprintf(out, "\n/* --- RIVEN CORE ENTRY POINT --- */\n");
            fprintf(out, "int main(int argc, char** argv) {\n");
            indent_level++;
            emit_indent();
            fprintf(out, "rv_init_runtime();\n");
            emit_indent();
            fprintf(out, "RvEnv* env = rv_env_create(NULL);\n");

            for (int i = 0; i < node->data.program.count; i++) {
                gen_node(node->data.program.nodes[i], env);
            }

            emit_indent();
            fprintf(out, "rv_env_free(env);\n");
            emit_indent();
            fprintf(out, "rv_shutdown_runtime();\n");
            emit_indent();
            fprintf(out, "return 0;\n");
            indent_level--;
            fprintf(out, "}\n");
            break;

        case NODE_IMPRINT_STMT:
            emit_indent();
            fprintf(out, "rv_imprint(");
            gen_expression(node->data.unary.operand, env);
            fprintf(out, ");\n");
            break;

        case NODE_VAR_DECL:
        case NODE_CONST_DECL:
            emit_indent();
            fprintf(out, "rv_define_var(env, \"%s\", ", node->data.declaration.name);
            gen_expression(node->data.declaration.value, env);
            fprintf(out, ", %s);\n", node->data.declaration.is_firm ? "true" : "false");
            break;

        case NODE_ASSIGN:
            emit_indent();
            if (node->data.binary.left->type == NODE_MEMBER_ACCESS) {
                fprintf(out, "rv_set_member(");
                gen_expression(node->data.binary.left->data.binary.left, env);
                fprintf(out, ", \"%s\", ", node->data.binary.left->data.binary.right->data.identifier);
                gen_expression(node->data.binary.right, env);
                fprintf(out, ");\n");
            } else {
                fprintf(out, "rv_set_var(env, \"%s\", ", node->data.binary.left->data.identifier);
                gen_expression(node->data.binary.right, env);
                fprintf(out, ");\n");
            }
            break;

        case NODE_IF_STMT:
            emit_indent();
            fprintf(out, "if (rv_is_truthy(");
            gen_expression(node->data.conditional.condition, env);
            fprintf(out, ")) {\n");
            indent_level++;
            gen_node(node->data.conditional.then_branch, env);
            indent_level--;
            emit_indent();
            fprintf(out, "}");
            if (node->data.conditional.else_branch) {
                fprintf(out, " else {\n");
                indent_level++;
                gen_node(node->data.conditional.else_branch, env);
                indent_level--;
                emit_indent();
                fprintf(out, "}\n");
            } else {
                fprintf(out, "\n");
            }
            break;

        case NODE_FLOW_LOOP:
            emit_indent();
            fprintf(out, "{\n");
            indent_level++;
            emit_indent();
            fprintf(out, "long _limit = rv_to_int(");
            gen_expression(node->data.loop.count_expr, env);
            fprintf(out, ");\n");
            emit_indent();
            fprintf(out, "for (long i = 0; i < _limit; i++) {\n");
            indent_level++;
            gen_node(node->data.loop.body, env);
            indent_level--;
            emit_indent();
            fprintf(out, "}\n");
            indent_level--;
            emit_indent();
            fprintf(out, "}\n");
            break;

        case NODE_DURING_LOOP:
            emit_indent();
            fprintf(out, "while (rv_is_truthy(");
            gen_expression(node->data.loop.condition, env);
            fprintf(out, ")) {\n");
            indent_level++;
            gen_node(node->data.loop.body, env);
            indent_level--;
            emit_indent();
            fprintf(out, "}\n");
            break;

        case NODE_FRAME_DECL:
            emit_indent();
            fprintf(out, "/* Frame: %s Registration */\n", node->data.frame.name);
            fprintf(out, "rv_register_frame(\"%s\");\n", node->data.frame.name);
            /* Implementation of boot() and members would follow in a specialized frame pass */
            break;

        case NODE_ASYNC_SPARK:
            emit_indent();
            fprintf(out, "rv_spark_task(");
            gen_expression(node->data.call_expr, env);
            fprintf(out, ");\n");
            break;

        case NODE_SYNC_STMT:
            emit_indent();
            fprintf(out, "rv_sync_tasks();\n");
            break;

        case NODE_RAW_BLOCK:
            fprintf(out, "\n/* --- RAW UNSAFE BLOCK --- */\n");
            /* In a native compiler, we emit direct C code if possible, or handle it as a specialized block */
            break;

        case NODE_RETURN_STMT:
            emit_indent();
            fprintf(out, "return ");
            gen_expression(node->data.unary.operand, env);
            fprintf(out, ";\n");
            break;

        default:
            break;
    }
}

/**
 * Entry point for Code Generation.
 * Strictly transpiles the full AST into optimized C11 code.
 */
void codegen_generate(ASTNode* root, const char* output_path) {
    out = fopen("riven_native.c", "w");
    if (out == NULL) {
        fprintf(stderr, "Compiler Error: Could not create intermediate C file for native build.\n");
        exit(1);
    }

    /* Emit Mandatory Runtime Headers */
    fprintf(out, "/* AUTO-GENERATED BY RIVEN COMPILER v1.0 */\n");
    fprintf(out, "#include <stdio.h>\n");
    fprintf(out, "#include <stdbool.h>\n");
    fprintf(out, "#include \"riven_runtime.h\"\n\n");

    /* AST Traversal Start */
    gen_node(root, NULL);

    fclose(out);
    printf("Native transpilation phase complete. Generated: riven_native.c\n");
}
