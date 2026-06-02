#include "../include/riven.h"
#include <stdlib.h>
#include <string.h>

/**
 * Creates a new AST node of the specified type.
 * Initializes the node memory to zero to ensure all union members are null.
 */
ASTNode* ast_create_node(ASTNodeType type, int line) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (node == NULL) {
        fprintf(stderr, "Fatal Error: Out of memory while allocating AST node at line %d.\n", line);
        exit(1);
    }
    
    node->type = type;
    node->line = line;
    
    /* Zero out the union data to prevent garbage pointers */
    memset(&node->data, 0, sizeof(node->data));
    
    return node;
}

/**
 * Recursively frees an AST node and all its children.
 * Handles different node structures based on the NodeType.
 */
void ast_free(ASTNode* node) {
    if (node == NULL) return;

    switch (node->type) {
        case NODE_PROGRAM:
        case NODE_CORE_BLOCK:
            for (int i = 0; i < node->data.program.count; i++) {
                ast_free(node->data.program.nodes[i]);
            }
            free(node->data.program.nodes);
            break;

        case NODE_VAR_DECL:
        case NODE_CONST_DECL:
            free(node->data.declaration.name);
            ast_free(node->data.declaration.value);
            break;

        case NODE_ASSIGN:
        case NODE_BINARY:
            ast_free(node->data.binary.left);
            ast_free(node->data.binary.right);
            break;

        case NODE_UNARY:
        case NODE_IMPRINT_STMT:
        case NODE_RETURN_STMT:
            ast_free(node->data.unary.operand);
            break;

        case NODE_IF_STMT:
            ast_free(node->data.conditional.condition);
            ast_free(node->data.conditional.then_branch);
            ast_free(node->data.conditional.else_branch);
            break;

        case NODE_FLOW_LOOP:
        case NODE_DURING_LOOP:
            ast_free(node->data.loop.count_expr);
            ast_free(node->data.loop.condition);
            ast_free(node->data.loop.body);
            break;

        case NODE_FRAME_DECL:
            free(node->data.frame.name);
            for (int i = 0; i < node->data.frame.member_count; i++) {
                ast_free(node->data.frame.members[i]);
            }
            free(node->data.frame.members);
            break;

        case NODE_SPAWN_EXPR:
            free(node->data.spawn.frame_name);
            for (int i = 0; i < node->data.spawn.arg_count; i++) {
                ast_free(node->data.spawn.args[i]);
            }
            free(node->data.spawn.args);
            break;

        case NODE_IDENTIFIER:
            free(node->data.identifier);
            break;

        case NODE_LITERAL:
            if (node->data.literal.type == TOKEN_STRING) {
                /* Cast away const to free string literal allocated by parser */
                free((void*)node->data.literal.as.s_val);
            }
            break;

        case NODE_FUNC_CALL:
        case NODE_ASYNC_SPARK:
        case NODE_MEMBER_ACCESS:
            /* These nodes utilize the call_expr or binary/unary fields 
               depending on specific parser implementation. */
            ast_free(node->data.call_expr);
            break;

        case NODE_RAW_BLOCK:
        case NODE_SYNC_STMT:
        case NODE_IMPORT:
        case NODE_ERROR_RESC:
            /* Basic structures with no unique dynamic children yet */
            break;

        default:
            break;
    }

    free(node);
}
