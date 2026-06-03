#include "../include/riven.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    Token current;
    Token previous;
    bool had_error;
    bool panic_mode;
} Parser;

static Parser parser;

/**
 * Error reporting system.
 */
static void error_at(Token* token, const char* message) {
    if (parser.panic_mode) return;
    parser.panic_mode = true;
    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        /* Error details already in token */
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.had_error = true;
}

/**
 * Moves to the next token in the stream.
 */
static void advance() {
    parser.previous = parser.current;

    for (;;) {
        parser.current = lexer_scan_token();
        if (parser.current.type != TOKEN_ERROR) break;

        error_at(&parser.current, parser.current.start);
    }
}

/**
 * Consumes the expected token or reports an error.
 */
static void consume(TokenType type, const char* message) {
    if (parser.current.type == type) {
        advance();
        return;
    }

    error_at(&parser.current, message);
}

static bool check(TokenType type) {
    return parser.current.type == type;
}

static bool match(TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
}

static char* copy_string(const char* start, int length) {
    char* str = (char*)malloc(length + 1);
    memcpy(str, start, length);
    str[length] = '\0';
    return str;
}

/* --- Forward Declarations for Recursive Descent --- */
static ASTNode* expression();
static ASTNode* statement();
static ASTNode* declaration();

/**
 * Parses primary expressions: Literals, IDs, Grouping, and Conversions.
 */
static ASTNode* primary() {
    if (match(TOKEN_CORRECT)) {
        ASTNode* node = ast_create_node(NODE_LITERAL, parser.previous.line);
        node->data.literal.type = TOKEN_CORRECT;
        node->data.literal.as.b_val = true;
        return node;
    }
    if (match(TOKEN_INCORRECT)) {
        ASTNode* node = ast_create_node(NODE_LITERAL, parser.previous.line);
        node->data.literal.type = TOKEN_INCORRECT;
        node->data.literal.as.b_val = false;
        return node;
    }
    if (match(TOKEN_EMP)) {
        ASTNode* node = ast_create_node(NODE_LITERAL, parser.previous.line);
        node->data.literal.type = TOKEN_EMP;
        return node;
    }
    if (match(TOKEN_NUMBER)) {
        ASTNode* node = ast_create_node(NODE_LITERAL, parser.previous.line);
        char* temp = copy_string(parser.previous.start, parser.previous.length);
        if (strchr(temp, '.')) {
            node->data.literal.type = TOKEN_DNUM;
            node->data.literal.as.d_val = strtod(temp, NULL);
        } else {
            node->data.literal.type = TOKEN_INT;
            node->data.literal.as.i_val = strtol(temp, NULL, 10);
        }
        free(temp);
        return node;
    }
    if (match(TOKEN_STRING)) {
        ASTNode* node = ast_create_node(NODE_LITERAL, parser.previous.line);
        node->data.literal.type = TOKEN_STRING;
        node->data.literal.as.s_val = copy_string(parser.previous.start + 1, parser.previous.length - 2);
        return node;
    }
    if (match(TOKEN_IDENTIFIER)) {
        ASTNode* node = ast_create_node(NODE_IDENTIFIER, parser.previous.line);
        node->data.identifier = copy_string(parser.previous.start, parser.previous.length);
        return node;
    }

    /* Type Conversions: int(x), txt(x), dnum(x) */
    if (match(TOKEN_INT) || match(TOKEN_TXT) || match(TOKEN_DNUM)) {
        TokenType op = parser.previous.type;
        consume(TOKEN_LPAREN, "Expect '(' after type conversion keyword.");
        ASTNode* arg = expression();
        consume(TOKEN_RPAREN, "Expect ')' after expression.");
        ASTNode* node = ast_create_node(NODE_UNARY, parser.previous.line);
        node->data.unary.op = op;
        node->data.unary.operand = arg;
        return node;
    }

    if (match(TOKEN_LPAREN)) {
        ASTNode* node = expression();
        consume(TOKEN_RPAREN, "Expect ')' after expression.");
        return node;
    }

    error_at(&parser.current, "Expect expression.");
    return NULL;
}

static ASTNode* call() {
    ASTNode* expr = primary();

    for (;;) {
        if (match(TOKEN_LPAREN)) {
            ASTNode* node = ast_create_node(NODE_FUNC_CALL, parser.previous.line);
            /* In codegen we treat this as a function call wrapper */
            node->data.call_expr = expr;
            /* Placeholder for argument parsing if needed in Phase 2 */
            consume(TOKEN_RPAREN, "Expect ')' after arguments.");
            expr = node;
        } else if (match(TOKEN_DOT)) {
            ASTNode* node = ast_create_node(NODE_MEMBER_ACCESS, parser.previous.line);
            node->data.binary.left = expr;
            consume(TOKEN_IDENTIFIER, "Expect member name after '.'.");
            ASTNode* right = ast_create_node(NODE_IDENTIFIER, parser.previous.line);
            right->data.identifier = copy_string(parser.previous.start, parser.previous.length);
            node->data.binary.right = right;
            expr = node;
        } else {
            break;
        }
    }
    return expr;
}

static ASTNode* unary() {
    if (match(TOKEN_NOT) || match(TOKEN_BANG) || match(TOKEN_MINUS)) {
        TokenType op = parser.previous.type;
        ASTNode* node = ast_create_node(NODE_UNARY, parser.previous.line);
        node->data.unary.op = op;
        node->data.unary.operand = unary();
        return node;
    }
    return call();
}

static ASTNode* multiplication() {
    ASTNode* expr = unary();
    while (match(TOKEN_STAR) || match(TOKEN_SLASH)) {
        TokenType op = parser.previous.type;
        ASTNode* right = unary();
        ASTNode* node = ast_create_node(NODE_BINARY, parser.previous.line);
        node->data.binary.left = expr;
        node->data.binary.op = op;
        node->data.binary.right = right;
        expr = node;
    }
    return expr;
}

static ASTNode* addition() {
    ASTNode* expr = multiplication();
    while (match(TOKEN_PLUS) || match(TOKEN_MINUS)) {
        TokenType op = parser.previous.type;
        ASTNode* right = multiplication();
        ASTNode* node = ast_create_node(NODE_BINARY, parser.previous.line);
        node->data.binary.left = expr;
        node->data.binary.op = op;
        node->data.binary.right = right;
        expr = node;
    }
    return expr;
}

static ASTNode* comparison() {
    ASTNode* expr = addition();
    while (match(TOKEN_GREATER) || match(TOKEN_GREATER_EQUALS) || 
           match(TOKEN_LESS) || match(TOKEN_LESS_EQUALS)) {
        TokenType op = parser.previous.type;
        ASTNode* right = addition();
        ASTNode* node = ast_create_node(NODE_BINARY, parser.previous.line);
        node->data.binary.left = expr;
        node->data.binary.op = op;
        node->data.binary.right = right;
        expr = node;
    }
    return expr;
}

static ASTNode* equality() {
    ASTNode* expr = comparison();
    while (match(TOKEN_EQUALS_EQUALS) || match(TOKEN_BANG_EQUALS)) {
        TokenType op = parser.previous.type;
        ASTNode* right = comparison();
        ASTNode* node = ast_create_node(NODE_BINARY, parser.previous.line);
        node->data.binary.left = expr;
        node->data.binary.op = op;
        node->data.binary.right = right;
        expr = node;
    }
    return expr;
}

static ASTNode* expression() {
    if (match(TOKEN_SPAWN)) {
        ASTNode* node = ast_create_node(NODE_SPAWN_EXPR, parser.previous.line);
        consume(TOKEN_IDENTIFIER, "Expect frame name after spawn.");
        node->data.spawn.frame_name = copy_string(parser.previous.start, parser.previous.length);
        consume(TOKEN_LPAREN, "Expect '(' for spawn arguments.");
        consume(TOKEN_RPAREN, "Expect ')' after spawn arguments.");
        return node;
    }
    
    ASTNode* expr = equality();

    if (match(TOKEN_EQUALS)) {
        Token equals = parser.previous;
        ASTNode* value = expression();
        if (expr->type == NODE_IDENTIFIER || expr->type == NODE_MEMBER_ACCESS) {
            ASTNode* node = ast_create_node(NODE_ASSIGN, equals.line);
            node->data.binary.left = expr;
            node->data.binary.right = value;
            return node;
        }
        error_at(&equals, "Invalid assignment target.");
    }
    return expr;
}

static ASTNode* block() {
    ASTNode* node = ast_create_node(NODE_PROGRAM, parser.previous.line);
    node->data.program.nodes = NULL;
    node->data.program.count = 0;

    consume(TOKEN_LBRACE, "Expect '{' to start block.");
    while (!check(TOKEN_RBRACE) && !check(TOKEN_EOF)) {
        node->data.program.count++;
        node->data.program.nodes = realloc(node->data.program.nodes, sizeof(ASTNode*) * node->data.program.count);
        node->data.program.nodes[node->data.program.count - 1] = declaration();
    }
    consume(TOKEN_RBRACE, "Expect '}' to end block.");
    return node;
}

static ASTNode* statement() {
    if (match(TOKEN_IMPRINT)) {
        consume(TOKEN_LPAREN, "Expect '(' after imprint.");
        ASTNode* expr = expression();
        consume(TOKEN_RPAREN, "Expect ')' after expression.");
        ASTNode* node = ast_create_node(NODE_IMPRINT_STMT, parser.previous.line);
        node->data.unary.operand = expr;
        return node;
    }

    if (match(TOKEN_IF)) {
        ASTNode* node = ast_create_node(NODE_IF_STMT, parser.previous.line);
        node->data.conditional.condition = expression();
        node->data.conditional.then_branch = block();
        if (match(TOKEN_ELSE)) {
            node->data.conditional.else_branch = block();
        }
        return node;
    }

    if (match(TOKEN_FLOW)) {
        ASTNode* node = ast_create_node(NODE_FLOW_LOOP, parser.previous.line);
        node->data.loop.count_expr = expression();
        node->data.loop.body = block();
        return node;
    }

    if (match(TOKEN_DURING)) {
        ASTNode* node = ast_create_node(NODE_DURING_LOOP, parser.previous.line);
        node->data.loop.condition = expression();
        node->data.loop.body = block();
        return node;
    }

    /* Postfix Increments / Decrements */
    ASTNode* expr = expression();
    if (match(TOKEN_PLUS_GT) || match(TOKEN_MINUS_LT)) {
        Token op = parser.previous;
        ASTNode* node = ast_create_node(NODE_ASSIGN, op.line);
        /* In a full compiler, we would generate x = x + 1 here. 
           For Phase 2 transpilation, we map the node for codegen. */
        node->data.binary.left = expr;
        return node;
    }

    return expr;
}

static ASTNode* declaration() {
    if (match(TOKEN_FIRM)) {
        consume(TOKEN_IDENTIFIER, "Expect name for firm constant.");
        char* name = copy_string(parser.previous.start, parser.previous.length);
        consume(TOKEN_EQUALS, "Expect '=' after constant name.");
        ASTNode* node = ast_create_node(NODE_CONST_DECL, parser.previous.line);
        node->data.declaration.name = name;
        node->data.declaration.value = expression();
        node->data.declaration.is_firm = true;
        return node;
    }
    return statement();
}

/**
 * Entry point for parsing a Riven source.
 */
ASTNode* parser_parse() {
    advance();
    ASTNode* root = ast_create_node(NODE_PROGRAM, 0);
    root->data.program.nodes = NULL;
    root->data.program.count = 0;

    while (!check(TOKEN_EOF)) {
        if (match(TOKEN_RIVEN)) {
            consume(TOKEN_CORE, "Expect 'core' after 'riven'.");
            ASTNode* core = ast_create_node(NODE_CORE_BLOCK, parser.previous.line);
            ASTNode* b = block();
            core->data.program.nodes = b->data.program.nodes;
            core->data.program.count = b->data.program.count;
            /* Note: b's memory is freed by ast_free, but we transfer child ownership */
            root->data.program.count++;
            root->data.program.nodes = realloc(root->data.program.nodes, sizeof(ASTNode*) * root->data.program.count);
            root->data.program.nodes[root->data.program.count - 1] = core;
        } else if (match(TOKEN_CONSISTOF)) {
            consume(TOKEN_STRING, "Expect library path after consistof.");
            /* Import logic handled by Toolchain */
            continue;
        } else {
            root->data.program.count++;
            root->data.program.nodes = realloc(root->data.program.nodes, sizeof(ASTNode*) * root->data.program.count);
            root->data.program.nodes[root->data.program.count - 1] = declaration();
        }
    }
    return parser.had_error ? NULL : root;
}
