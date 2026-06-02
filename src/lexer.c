#include "../include/riven.h"
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

typedef struct {
    const char* start;
    const char* current;
    int line;
} Scanner;

static Scanner scanner;

void lexer_init(const char* source) {
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

static bool is_at_end() {
    return *scanner.current == '\0';
}

static char advance() {
    scanner.current++;
    return scanner.current[-1];
}

static char peek() {
    return *scanner.current;
}

static char peek_next() {
    if (is_at_end()) return '\0';
    return scanner.current[1];
}

static bool match(char expected) {
    if (is_at_end()) return false;
    if (*scanner.current != expected) return false;
    scanner.current++;
    return true;
}

static Token make_token(TokenType type) {
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;
    return token;
}

static Token error_token(const char* message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = scanner.line;
    return token;
}

static void skip_whitespace() {
    for (;;) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                scanner.line++;
                advance();
                break;
            case '~':
                if (peek_next() == '~') {
                    /* Single-line comment: ~~ */
                    while (peek() != '\n' && !is_at_end()) advance();
                } else {
                    return;
                }
                break;
            case '<':
                if (peek_next() == '<') {
                    /* Multi-line comment: << >> */
                    advance(); advance(); /* skip << */
                    while (!(peek() == '>' && peek_next() == '>') && !is_at_end()) {
                        if (peek() == '\n') scanner.line++;
                        advance();
                    }
                    if (!is_at_end()) {
                        advance(); advance(); /* skip >> */
                    }
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

static TokenType check_keyword(int start, int length, const char* rest, TokenType type) {
    if (scanner.current - scanner.start == start + length &&
        memcmp(scanner.start + start, rest, length) == 0) {
        return type;
    }
    return TOKEN_IDENTIFIER;
}

static TokenType identifier_type() {
    switch (scanner.start[0]) {
        case 'a':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'l': return check_keyword(2, 3, "tif", TOKEN_ALTIF);
                    case 'n': return check_keyword(2, 1, "d", TOKEN_AND);
                    case 't': return check_keyword(2, 4, "tack", TOKEN_ATTACK);
                }
            }
            break;
        case 'b':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'i': return check_keyword(2, 2, "nd", TOKEN_BIND);
                    case 'o': return check_keyword(2, 2, "ot", TOKEN_BOOT);
                }
            }
            break;
        case 'c':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'o':
                        if (scanner.current - scanner.start > 2) {
                            switch (scanner.start[2]) {
                                case 'l': return check_keyword(3, 1, "l", TOKEN_COLL);
                                case 'n': return check_keyword(3, 7, "sistof", TOKEN_CONSISTOF);
                                case 'r': return check_keyword(3, 4, "rect", TOKEN_CORRECT);
                            }
                        }
                        break;
                    case 'r': return check_keyword(2, 3, "aft", TOKEN_CRAFT);
                }
            }
            break;
        case 'd':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'n': return check_keyword(2, 2, "um", TOKEN_DNUM);
                    case 'r': return check_keyword(2, 2, "op", TOKEN_DROP);
                    case 'u': return check_keyword(2, 4, "ring", TOKEN_DURING);
                }
            }
            break;
        case 'e':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'l': return check_keyword(2, 2, "se", TOKEN_ELSE);
                    case 'm': return check_keyword(2, 1, "p", TOKEN_EMP);
                }
            }
            break;
        case 'f':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'e': return check_keyword(2, 3, "tch", TOKEN_FETCH);
                    case 'i': return check_keyword(2, 2, "rm", TOKEN_FIRM);
                    case 'l': return check_keyword(2, 2, "ow", TOKEN_FLOW);
                    case 'r': return check_keyword(2, 3, "ame", TOKEN_FRAME);
                }
            }
            break;
        case 'g': return check_keyword(1, 3, "rab", TOKEN_GRAB);
        case 'h': return check_keyword(1, 5, "idden", TOKEN_HIDDEN);
        case 'i':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'f': return check_keyword(2, 0, "", TOKEN_IF);
                    case 'm': return check_keyword(2, 5, "print", TOKEN_IMPRINT);
                    case 'n':
                        if (scanner.current - scanner.start > 2) {
                            switch (scanner.start[2]) {
                                case 'c': return check_keyword(3, 6, "orrect", TOKEN_INCORRECT);
                                case 't': return check_keyword(3, 0, "", TOKEN_INT);
                            }
                        }
                        break;
                }
            }
            break;
        case 'n': return check_keyword(1, 2, "ot", TOKEN_NOT);
        case 'o': return check_keyword(1, 3, "pen", TOKEN_OPEN);
        case 'p': return check_keyword(1, 2, "tr", TOKEN_PTR);
        case 'r':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'a': return check_keyword(2, 1, "w", TOKEN_RAW);
                    case 'e':
                        if (scanner.current - scanner.start > 2) {
                            switch (scanner.start[2]) {
                                case 'c': return check_keyword(3, 0, "", TOKEN_REC);
                                case 'f': return check_keyword(3, 0, "", TOKEN_REF);
                                case 's': return check_keyword(3, 1, "c", TOKEN_RESC);
                                case 't': return check_keyword(3, 4, "urns", TOKEN_RETURNS);
                            }
                        }
                        break;
                    case 'i':
                        if (scanner.current - scanner.start > 2) {
                            switch (scanner.start[2]) {
                                case 's': return check_keyword(3, 1, "e", TOKEN_RISE);
                                case 'v': return check_keyword(3, 2, "en", TOKEN_RIVEN);
                            }
                        }
                        break;
                }
            }
            break;
        case 's':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'p':
                        if (scanner.current - scanner.start > 2) {
                            switch (scanner.start[2]) {
                                case 'a':
                                    if (scanner.current - scanner.start > 3 && scanner.start[3] == 'r')
                                        return check_keyword(4, 1, "k", TOKEN_SPARK);
                                    if (scanner.current - scanner.start > 3 && scanner.start[3] == 'w')
                                        return check_keyword(4, 1, "n", TOKEN_SPAWN);
                                    break;
                            }
                        }
                        break;
                    case 'y': return check_keyword(2, 2, "nc", TOKEN_SYNC);
                }
            }
            break;
        case 't': return check_keyword(1, 2, "xt", TOKEN_TXT);
    }
    return TOKEN_IDENTIFIER;
}

static Token identifier() {
    while (isalnum(peek()) || peek() == '_') advance();
    return make_token(identifier_type());
}

static Token number() {
    while (isdigit(peek())) advance();
    if (peek() == '.' && isdigit(peek_next())) {
        advance(); /* consume . */
        while (isdigit(peek())) advance();
    }
    return make_token(TOKEN_NUMBER);
}

static Token string() {
    while (peek() != '"' && !is_at_end()) {
        if (peek() == '\n') scanner.line++;
        advance();
    }
    if (is_at_end()) return error_token("Unterminated string.");
    advance(); /* consume closing " */
    return make_token(TOKEN_STRING);
}

Token lexer_scan_token() {
    skip_whitespace();
    scanner.start = scanner.current;

    if (is_at_end()) return make_token(TOKEN_EOF);

    char c = advance();
    if (isalpha(c) || c == '_') return identifier();
    if (isdigit(c)) return number();

    switch (c) {
        case '(': return make_token(TOKEN_LPAREN);
        case ')': return make_token(TOKEN_RPAREN);
        case '{': return make_token(TOKEN_LBRACE);
        case '}': return make_token(TOKEN_RBRACE);
        case '[': return make_token(TOKEN_LBRACKET);
        case ']': return make_token(TOKEN_RBRACKET);
        case ';': return make_token(TOKEN_SEMICOLON);
        case ',': return make_token(TOKEN_COMMA);
        case '.': return make_token(TOKEN_DOT);
        case '-':
            if (match('<')) return make_token(TOKEN_MINUS_LT);
            return make_token(TOKEN_MINUS);
        case '+':
            if (match('>')) return make_token(TOKEN_PLUS_GT);
            return make_token(TOKEN_PLUS);
        case '*': return make_token(TOKEN_STAR);
        case '/': return make_token(TOKEN_SLASH);
        case '!':
            return make_token(match('=') ? TOKEN_BANG_EQUALS : TOKEN_BANG);
        case '=':
            return make_token(match('=') ? TOKEN_EQUALS_EQUALS : TOKEN_EQUALS);
        case '<':
            return make_token(match('=') ? TOKEN_LESS_EQUALS : TOKEN_LESS);
        case '>':
            return make_token(match('=') ? TOKEN_GREATER_EQUALS : TOKEN_GREATER);
        case '&':
            if (match('&')) return make_token(TOKEN_AMP_AMP);
            break;
        case '|':
            if (match('|')) return make_token(TOKEN_PIPE_PIPE);
            break;
        case '"': return string();
    }

    return error_token("Unexpected character.");
}
