#pragma once
#include <string>
#include <unordered_map>

// ──────────────────────────────────────────────
//  All token kinds in Riven
// ──────────────────────────────────────────────
enum class TokenKind {
    // ── Literals ──────────────────────────────
    NUMBER,        // 42  3.14
    STRING,        // "hello"
    IDENTIFIER,    // foo  bar

    // ── Keywords ──────────────────────────────
    RIVEN,         // riven
    CORE,          // core
    FIRM,          // firm        (constant)
    CRAFT,         // craft       (function def)
    RETURNS,       // returns
    IF,            // if
    ALTIF,         // altif       (else if)
    ELSE,          // else
    FLOW,          // flow        (fixed loop)
    DURING,        // during      (while loop)
    RISE,          // rise        (readable inc)
    DROP,          // drop        (readable dec)
    COLL,          // coll        (collection/array)
    REC,           // rec         (record/struct)
    FRAME,         // frame       (class/OOP)
    BOOT,          // boot        (constructor)
    SPAWN,         // spawn       (object creation)
    HIDDEN,        // hidden      (private)
    OPEN,          // open        (public)
    CONSISTOF,     // consistof   (import)
    RESC,          // resc        (try/rescue block)
    ATTACK,        // attack      (throw error)
    SPARK,         // spark       (async)
    SYNC,          // sync        (await)
    REF,           // ref         (reference)
    PTR,           // ptr         (pointer)
    RAW,           // raw         (unsafe block)
    AND_KW,        // and         (word form)
    OR_KW,         // or          (word form)
    NOT_KW,        // not         (word form)
    TRUE_KW,       // true
    FALSE_KW,      // false
    NULL_KW,       // null
    IMPRINT,       // imprint     (print)
    INP,           // inp         (input)
    BREAK,         // break
    CONTINUE,      // continue
    RETURN,        // return
    SELF,          // self        (this)
    EXTENDS,       // extends     (inheritance)
    IMPLEMENTS,    // implements
    INTERFACE,     // interface
    ENUM_KW,       // enum
    MATCH,         // match       (switch/pattern)
    CASE,          // case
    DEFAULT,       // default
    IMPORT,        // import      (alias)
    FROM,          // from
    AS,            // as
    IN,            // in          (for-in)
    OF,            // of

    // ── Operators ─────────────────────────────
    ASSIGN,        // =
    PLUS,          // +
    MINUS,         // -
    STAR,          // *
    SLASH,         // /
    PERCENT,       // %
    CARET,         // ^
    AMP,           // &
    PIPE,          // |
    TILDE,         // ~

    PLUS_ASSIGN,   // +=
    MINUS_ASSIGN,  // -=
    STAR_ASSIGN,   // *=
    SLASH_ASSIGN,  // /=
    PERCENT_ASSIGN,// %=

    EQUAL_EQUAL,   // ==
    BANG_EQUAL,    // !=
    LESS,          // <
    GREATER,       // >
    LESS_EQUAL,    // <=
    GREATER_EQUAL, // >=

    AND_AND,       // &&
    OR_OR,         // ||
    BANG,          // !

    ARROW,         // ->
    FAT_ARROW,     // =>
    DOUBLE_COLON,  // ::

    INCREMENT,     // +>
    DECREMENT,     // -<

    DOT,           // .
    COMMA,         // ,
    COLON,         // :
    SEMICOLON,     // ;
    QUESTION,      // ?

    // ── Delimiters ────────────────────────────
    LPAREN,        // (
    RPAREN,        // )
    LBRACE,        // {
    RBRACE,        // }
    LBRACKET,      // [
    RBRACKET,      // ]

    // ── Special ───────────────────────────────
    EOF_TOKEN,
    ERROR          // lexer error
};

// ──────────────────────────────────────────────
//  Source location
// ──────────────────────────────────────────────
struct SourceLocation {
    std::size_t line   = 1;
    std::size_t column = 1;
    std::string file;
};

// ──────────────────────────────────────────────
//  Token
// ──────────────────────────────────────────────
struct Token {
    TokenKind      kind;
    std::string    lexeme;   // raw source text
    std::string    value;    // processed value (strings unquoted, etc.)
    SourceLocation loc;

    Token(TokenKind k,
          std::string lex,
          std::string val,
          SourceLocation l)
        : kind(k), lexeme(std::move(lex)),
          value(std::move(val)), loc(std::move(l)) {}

    // Human-readable representation
    [[nodiscard]] std::string toString() const;
    [[nodiscard]] std::string kindName() const;
};

// ──────────────────────────────────────────────
//  Keyword table  (keyword string → TokenKind)
// ──────────────────────────────────────────────
const std::unordered_map<std::string, TokenKind>& getKeywordMap();
