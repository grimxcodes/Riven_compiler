#include "../include/riven.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FILE* out;
static int indent_level = 0;

/* Helper to handle C code indentation */
static void emit_indent() {
    for (int i = 0; i < indent_level; i++) {
        fprintf(out, "    ");
    }
}

/* Forward declarations for recursive generation */
static void gen_expression(ASTNode* node);
static void gen_node(ASTNode* node);

/**
 * Maps Riven binary operators to C runtime calls or native operators.
 * Uses runtime wrappers to ensure Riven's dynamic-but-safe behavior.
 */
static void gen_binary(ASTNode* node) {
    fprintf(out, "rv_op_binary(");
    gen_expression(node->data.binary.left);
    fprintf(out, ", ");
    
    switch (node->data.binary.op) {
        case TOKEN_PLUS:           fprintf(out, "OP_ADD"); break;
        case TOKEN_MINUS:          fprintf(out, "OP_SUB"); break;
        case TOKEN_STAR:           fprintf(out, "OP_MUL"); break;
        case TOKEN_SLASH:          fprintf(out, "OP_DIV"); break;
        case TOKEN_EQUALS_EQUALS:  fprintf(out, "OP_EQ");  break;
        case TOKEN_BANG_EQUALS:    fprintf(out, "OP_NEQ"); break;
        case TOKEN_GREATER:        fprintf(out, "OP_GT");  break;
        case TOKEN_LESS:           fprintf(out, "OP_LT");  break;
        case TOKEN_AND:
        case TOKEN_AMP_AMP:        fprintf(out, "OP_AND"); break;
        case TOKEN_OR:
        case TOKEN_PIPE_PIPE:      fprintf(out, "OP_OR");  break;
        default:                   fprintf(out, "OP_UNKNOWN"); break;
    }
    
    fprintf(out, ", ");
    gen_expression(node->data.binary.right);
    fprintf(out, ")");
}

/**
 * Generates code for Riven expressions.
 * Implements ARC by wrapping literals in runtime constructors.
 */
static void gen_expression(ASTNode* node) {
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
            else fprintf(out, "rv_emp()");
            break;
        }

        case NODE_IDENTIFIER:
            /* Access variable through the runtime symbol table */
            fprintf(out, "rv_get_var(env, \"%s\")", node->data.identifier);
            break;

        case NODE_BINARY:
            gen_binary(node);
            break;

        case NODE_UNARY:
            fprintf(out, "rv_op_unary(");
            if (node->data.unary.op == TOKEN_MINUS) fprintf(out, "OP_NEG");
            else fprintf(out, "OP_NOT");
            fprintf(out, ", ");
            gen_expression(node->data.unary.operand);
            fprintf(out, ")");
            break;

        case NODE_SPAWN_EXPR:
            fprintf(out, "rv_spawn(\"%s\")", node->data.spawn.frame_name);
            break;

        default:
            fprintf(out, "rv_emp()");
            break;
    }
}

/**
 * Generates code for a single Riven statement or declaration.
 */
static void gen_node(ASTNode* node) {
    if (node == NULL) return;

    switch (node->type) {
        case NODE_CORE_BLOCK:
            fprintf(out, "\n/* --- Riven Core Entry --- */\n");
            fprintf(out, "int main(int argc, char** argv) {\n");
            indent_level++;
            emit_indent();
            fprintf(out, "rv_init_runtime();\n");
            emit_indent();
            fprintf(out, "RvEnv* env = rv_env_create(NULL);\n");

            for (int i = 0; i < node->data.program.count; i++) {
                gen_node(node->data.program.nodes[i]);
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
            gen_expression(node->data.unary.operand);
            fprintf(out, ");\n");
            break;

        case NODE_VAR_DECL:
        case NODE_CONST_DECL:
            emit_indent();
            fprintf(out, "rv_define_var(env, \"%s\", ", node->data.declaration.name);
            gen_expression(node->data.declaration.value);
            fprintf(out, ", %s);\n", node->data.declaration.is_firm ? "true" : "false");
            break;

        case NODE_ASSIGN:
            emit_indent();
            fprintf(out, "rv_set_var(env, \"%s\", ", node->data.binary.left->data.identifier);
            gen_expression(node->data.binary.right);
            fprintf(out, ");\n");
            break;

        case NODE_IF_STMT:
            emit_indent();
            fprintf(out, "if (rv_is_truthy(");
            gen_expression(node->data.conditional.condition);
            fprintf(out, ")) {\n");
            indent_level++;
            
            /* Recurse into block */
            if (node->data.conditional.then_branch->type == NODE_PROGRAM) {
                for (int i = 0; i < node->data.conditional.then_branch->data.program.count; i++) {
                    gen_node(node->data.conditional.then_branch->data.program.nodes[i]);
                }
            }
            
            indent_level--;
            emit_indent();
            fprintf(out, "}");
            
            if (node->data.conditional.else_branch) {
                fprintf(out, " else {\n");
                indent_level++;
                if (node->data.conditional.else_branch->type == NODE_PROGRAM) {
                    for (int i = 0; i < node->data.conditional.else_branch->data.program.count; i++) {
                        gen_node(node->data.conditional.else_branch->data.program.nodes[i]);
                    }
                }
                indent_level--;
                emit_indent();
                fprintf(out, "}\n");
            } else {
                fprintf(out, "\n");
            }
            break;

        case NODE_FLOW_LOOP:
            emit_indent();
            fprintf(out, "for (long i = 0, n = rv_to_int(");
            gen_expression(node->data.loop.count_expr);
            fprintf(out, "); i < n; i++) {\n");
            indent_level++;
            if (node->data.loop.body->type == NODE_PROGRAM) {
                for (int i = 0; i < node->data.loop.body->data.program.count; i++) {
                    gen_node(node->data.loop.body->data.program.nodes[i]);
                }
            }
            indent_level--;
            emit_indent();
            fprintf(out, "}\n");
            break;

        case NODE_RETURN_STMT:
            emit_indent();
            fprintf(out, "return ");
            gen_expression(node->data.unary.operand);
            fprintf(out, ";\n");
            break;

        default:
            break;
    }
}

/**
 * Main entry point for code generation.
 * Transpiles the AST into a temporary C file and triggers the native build.
 */
void codegen_generate(ASTNode* root, const char* output_path) {
    out = fopen("riven_native.c", "w");
    if (out == NULL) {
        fprintf(stderr, "Compiler Error: Could not create intermediate C file.\n");
        exit(1);
    }

    /* 1. Emit C Headers */
    fprintf(out, "/* Generated by Riven Native Compiler v1.0 */\n");
    fprintf(out, "#include <stdio.h>\n");
    fprintf(out, "#include <stdbool.h>\n");
    fprintf(out, "#include \"riven_runtime.h\"\n\n");

    /* 2. Traverse AST and Emit Code */
    if (root->type == NODE_PROGRAM) {
        for (int i = 0; i < root->data.program.count; i++) {
            gen_node(root->data.program.nodes[i]);
        }
    }

    fclose(out);
    printf("Native transpilation complete: riven_native.c generated.\n");
}
