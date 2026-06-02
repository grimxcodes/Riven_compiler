#ifndef RIVEN_H
#define RIVEN_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>

/* --- Lexical Analysis --- */

typedef enum {
    /* Metadata */
    TOKEN_ERROR,
    TOKEN_EOF,

    /* Literals & IDs */
    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_NUMBER,

    /* Entry & Structure */
    TOKEN_RIVEN,
    TOKEN_CORE,
    TOKEN_CRAFT,
    TOKEN_RETURNS,
    TOKEN_FIRM,
    TOKEN_CONSISTOF,

    /* Types */
    TOKEN_INT,
    TOKEN_DNUM,
    TOKEN_TXT,
    TOKEN_CORRECT,
    TOKEN_INCORRECT,
    TOKEN_COLL,
    TOKEN_REC,
    TOKEN_EMP,

    /* Control Flow */
    TOKEN_IF,
    TOKEN_ALTIF,
    TOKEN_ELSE,
    TOKEN_FLOW,
    TOKEN_DURING,

    /* Logic & Symbols */
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT,
    TOKEN_AMP_AMP,    /* && */
    TOKEN_PIPE_PIPE,  /* || */
    TOKEN_BANG,       /* ! */

    /* OOP System */
    TOKEN_FRAME,
    TOKEN_BOOT,
    TOKEN_SPAWN,
    TOKEN_OPEN,
    TOKEN_HIDDEN,

    /* Async & Background */
    TOKEN_SPARK,
    TOKEN_SYNC,

    /* Error Handling */
    TOKEN_RESC,
    TOKEN_ATTACK,

    /* System & Low-Level */
    TOKEN_RAW,
    TOKEN_BIND,
    TOKEN_REF,
    TOKEN_PTR,

    /* Built-ins */
    TOKEN_IMPRINT,    /* Replaced stamp */
    TOKEN_GRAB,
    TOKEN_FETCH,

    /* Operators */
    TOKEN_EQUALS,         /* = */
    TOKEN_EQUALS_EQUALS,  /* == */
    TOKEN_BANG_EQUALS,    /* != */
    TOKEN_PLUS,           /* + */
    TOKEN_MINUS,          /* - */
    TOKEN_STAR,           /* * */
    TOKEN_SLASH,          /* / */
    TOKEN_GREATER,        /* > */
    TOKEN_LESS,           /* < */
    TOKEN_PLUS_GT,        /* +> */
    TOKEN_MINUS_LT,       /* -< */
    TOKEN_RISE,
    TOKEN_DROP,

    /* Punctuators */
    TOKEN_LPAREN,   /* ( */
    TOKEN_RPAREN,   /* ) */
    TOKEN_LBRACE,   /* { */
    TOKEN_RBRACE,   /* } */
    TOKEN_LBRACKET, /* [ */
    TOKEN_RBRACKET, /* ] */
    TOKEN_COMMA,    /* , */
    TOKEN_DOT,      /* . */
    TOKEN_SEMICOLON /* ; */
} TokenType;

typedef struct {
    TokenType type;
    const char* start;
    int length;
    int line;
} Token;

/* --- Abstract Syntax Tree (AST) --- */

typedef enum {
    NODE_PROGRAM,
    NODE_CORE_BLOCK,
    NODE_VAR_DECL,
    NODE_CONST_DECL,
    NODE_ASSIGN,
    NODE_BINARY,
    NODE_UNARY,
    NODE_LITERAL,
    NODE_IDENTIFIER,
    NODE_FUNC_DECL,
    NODE_FUNC_CALL,
    NODE_IF_STMT,
    NODE_FLOW_LOOP,
    NODE_DURING_LOOP,
    NODE_FRAME_DECL,
    NODE_SPAWN_EXPR,
    NODE_MEMBER_ACCESS,
    NODE_IMPRINT_STMT,
    NODE_ASYNC_SPARK,
    NODE_SYNC_STMT,
    NODE_RAW_BLOCK,
    NODE_RETURN_STMT,
    NODE_IMPORT,
    NODE_ERROR_RESC
} ASTNodeType;

/* Value Container for Parser/AST (Not for Runtime) */
typedef struct {
    TokenType type;
    union {
        long i_val;
        double d_val;
        const char* s_val;
        bool b_val;
    } as;
} ASTValue;

typedef struct ASTNode {
    ASTNodeType type;
    int line;
    union {
        /* Program Structure */
        struct {
            struct ASTNode** nodes;
            int count;
        } program;

        /* Variable/Const Logic */
        struct {
            char* name;
            struct ASTNode* value;
            bool is_firm;
        } declaration;

        /* Expressions */
        struct {
            struct ASTNode* left;
            TokenType op;
            struct ASTNode* right;
        } binary;

        struct {
            TokenType op;
            struct ASTNode* operand;
        } unary;

        /* Control Flow */
        struct {
            struct ASTNode* condition;
            struct ASTNode* then_branch;
            struct ASTNode* else_branch;
        } conditional;

        struct {
            struct ASTNode* count_expr;
            struct ASTNode* condition;
            struct ASTNode* body;
        } loop;

        /* OOP */
        struct {
            char* name;
            struct ASTNode** members;
            int member_count;
        } frame;

        struct {
            char* frame_name;
            struct ASTNode** args;
            int arg_count;
        } spawn;

        /* Built-ins & Literals */
        ASTValue literal;
        char* identifier;
        struct ASTNode* call_expr;
    } data;
} ASTNode;

/* --- Native Compiler Toolchain API --- */

/* Lexer (lexer.c) */
void lexer_init(const char* source);
Token lexer_scan_token(void);

/* Parser (parser.c) */
ASTNode* parser_parse(void);

/* AST Builder (ast.c) */
ASTNode* ast_create_node(ASTNodeType type, int line);
void ast_free(ASTNode* node);

/* Semantic Analysis / Scoping (environment.c) */
typedef struct Symbol {
    char* name;
    bool is_firm;
    bool is_public;
} Symbol;

typedef struct Environment {
    Symbol* symbols;
    int count;
    int capacity;
    struct Environment* parent;
} Environment;

Environment* env_create(Environment* parent);
void env_add_symbol(Environment* env, const char* name, bool is_firm, bool is_public);
bool env_exists(Environment* env, const char* name);

/* Native Code Generator (codegen.c) */
void codegen_generate(ASTNode* root, const char* output_path);

/* CLI Driver (main.c) */
typedef struct {
    const char* input_file;
    const char* output_file;
    bool is_forge;
} CompilerConfig;

#endif /* RIVEN_H */
