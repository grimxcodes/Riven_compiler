
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

static void error_at(Token* token, const char* message) {
    if (parser.panic_mode) return;
    parser.panic_mode = true;
    fprintf(stderr, "[line %d] Error", token->line);
    if (token->type == TOKEN_EOF) fprintf(stderr, " at end");
    else if (token->type == TOKEN_ERROR) {}
    else fprintf(stderr, " at '%.*s'", token->length, token->start);
    fprintf(stderr, ": %s\n", message);
    parser.had_error = true;
}

static void advance() {
    parser.previous = parser.current;
    for (;;) {
        parser.current = lexer_scan_token();
        if (parser.current.type != TOKEN_ERROR) break;
        error_at(&parser.current, parser.current.start);
    }
}

static void consume(TokenType type, const char* message) {
    if (parser.current.type == type) { advance(); return; }
    error_at(&parser.current, message);
}

static bool check(TokenType type) { return parser.current.type == type; }
static bool match(TokenType type) { if (!check(type)) return false; advance(); return true; }

static char* copy_string(const char* start, int length) {
    char* str = (char*)malloc(length + 1);
    memcpy(str, start, length);
    str[length] = '\0';
    return str;
}

/* --- Precedence Layers --- */
static ASTNode* expression();
static ASTNode* statement();
static ASTNode* declaration();
static ASTNode* or_expression();

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
    if (match(TOKEN_INT) || match(TOKEN_TXT) || match(TOKEN_DNUM)) {
        TokenType type = parser.previous.type;
        consume(TOKEN_LPAREN, "Expect '(' after conversion.");
        ASTNode* arg = expression();
        consume(TOKEN_RPAREN, "Expect ')'.");
        ASTNode* node = ast_create_node(NODE_UNARY, parser.previous.line);
        node->data.unary.op = type;
        node->data.unary.operand = arg;
        return node;
    }
    if (match(TOKEN_LPAREN)) {
        ASTNode* node = expression();
        consume(TOKEN_RPAREN, "Expect ')' after grouping.");
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
            node->data.call_expr = expr;
            consume(TOKEN_RPAREN, "Expect ')'.");
            expr = node;
        } else if (match(TOKEN_DOT)) {
            ASTNode* node = ast_create_node(NODE_MEMBER_ACCESS, parser.previous.line);
            node->data.binary.left = expr;
            consume(TOKEN_IDENTIFIER, "Expect member name.");
            ASTNode* right = ast_create_node(NODE_IDENTIFIER, parser.previous.line);
            right->data.identifier = copy_string(parser.previous.start, parser.previous.length);
            node->data.binary.right = right;
            expr = node;
        } else break;
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
    while (match(TOKEN_GREATER) || match(TOKEN_GREATER_EQUALS) || match(TOKEN_LESS) || match(TOKEN_LESS_EQUALS)) {
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

/* FIX: Logic AND Layer */
static ASTNode* and_expression() {
    ASTNode* expr = equality();
    while (match(TOKEN_AND) || match(TOKEN_AMP_AMP)) {
        TokenType op = parser.previous.type;
        ASTNode* right = equality();
        ASTNode* node = ast_create_node(NODE_BINARY, parser.previous.line);
        node->data.binary.left = expr;
        node->data.binary.op = op;
        node->data.binary.right = right;
        expr = node;
    }
    return expr;
}

/* FIX: Logic OR Layer */
static ASTNode* or_expression() {
    ASTNode* expr = and_expression();
    while (match(TOKEN_OR) || match(TOKEN_PIPE_PIPE)) {
        TokenType op = parser.previous.type;
        ASTNode* right = and_expression();
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
        consume(TOKEN_IDENTIFIER, "Expect frame name.");
        node->data.spawn.frame_name = copy_string(parser.previous.start, parser.previous.length);
        consume(TOKEN_LPAREN, "Expect '(' after spawn.");
        consume(TOKEN_RPAREN, "Expect ')' after spawn.");
        return node;
    }
    
    ASTNode* expr = or_expression();

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
    consume(TOKEN_LBRACE, "Expect '{'.");
    while (!check(TOKEN_RBRACE) && !check(TOKEN_EOF)) {
        node->data.program.count++;
        node->data.program.nodes = realloc(node->data.program.nodes, sizeof(ASTNode*) * node->data.program.count);
        node->data.program.nodes[node->data.program.count - 1] = declaration();
    }
    consume(TOKEN_RBRACE, "Expect '}'.");
    return node;
}

static ASTNode* statement() {
    if (match(TOKEN_IMPRINT)) {
        consume(TOKEN_LPAREN, "Expect '('.");
        ASTNode* expr = expression();
        consume(TOKEN_RPAREN, "Expect ')'.");
        ASTNode* node = ast_create_node(NODE_IMPRINT_STMT, parser.previous.line);
        node->data.unary.operand = expr;
        return node;
    }
    if (match(TOKEN_IF)) {
        ASTNode* node = ast_create_node(NODE_IF_STMT, parser.previous.line);
        node->data.conditional.condition = expression();
        node->data.conditional.then_branch = block();
        if (match(TOKEN_ELSE)) node->data.conditional.else_branch = block();
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
    
    ASTNode* expr = expression();
    if (match(TOKEN_PLUS_GT) || match(TOKEN_MINUS_LT)) {
        Token op = parser.previous;
        ASTNode* node = ast_create_node(NODE_ASSIGN, op.line);
        node->data.binary.left = expr;
        return node;
    }
    return expr;
}

static ASTNode* declaration() {
    if (match(TOKEN_FIRM)) {
        consume(TOKEN_IDENTIFIER, "Expect constant name.");
        char* name = copy_string(parser.previous.start, parser.previous.length);
        consume(TOKEN_EQUALS, "Expect '='.");
        ASTNode* node = ast_create_node(NODE_CONST_DECL, parser.previous.line);
        node->data.declaration.name = name;
        node->data.declaration.value = expression();
        node->data.declaration.is_firm = true;
        return node;
    }
    return statement();
}

ASTNode* parser_parse() {
    advance();
    ASTNode* root = ast_create_node(NODE_PROGRAM, 0);
    root->data.program.nodes = NULL;
    root->data.program.count = 0;
    while (!check(TOKEN_EOF)) {
        if (match(TOKEN_RIVEN)) {
            consume(TOKEN_CORE, "Expect 'core'.");
            ASTNode* core = ast_create_node(NODE_CORE_BLOCK, parser.previous.line);
            ASTNode* b = block();
            core->data.program.nodes = b->data.program.nodes;
            core->data.program.count = b->data.program.count;
            root->data.program.count++;
            root->data.program.nodes = realloc(root->data.program.nodes, sizeof(ASTNode*) * root->data.program.count);
            root->data.program.nodes[root->data.program.count - 1] = core;
        } else if (match(TOKEN_CONSISTOF)) {
            consume(TOKEN_STRING, "Expect library path.");
            continue;
        } else {
            root->data.program.count++;
            root->data.program.nodes = realloc(root->data.program.nodes, sizeof(ASTNode*) * root->data.program.count);
            root->data.program.nodes[root->data.program.count - 1] = declaration();
        }
    }
    return parser.had_error ? NULL : root;
}
