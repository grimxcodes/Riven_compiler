#include "lexer.h"
#include <cctype>
#include <stdexcept>
#include <sstream>

// ─────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────
Lexer::Lexer(std::string source, std::string filename, DiagCallback cb)
    : src_(std::move(source)), file_(std::move(filename)), diagCb_(std::move(cb)) {}

// ─────────────────────────────────────────────
//  Public: tokenize
// ─────────────────────────────────────────────
std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    tokens.reserve(256);

    while (true) {
        skipWhitespace();
        if (atEnd()) {
            markStart();
            tokens.push_back(makeToken(TokenKind::EOF_TOKEN, ""));
            break;
        }

        markStart();
        char c = peek();

        // ── Single-line comment ~~
        if (c == '~' && peek(1) == '~') {
            skipLineComment();
            continue;
        }
        // ── Block comment << … >>
        if (c == '<' && peek(1) == '<') {
            skipBlockComment();
            continue;
        }
        // ── String literals
        if (c == '"' || c == '\'') {
            tokens.push_back(scanString(c));
            continue;
        }
        // ── Numbers
        if (std::isdigit(static_cast<unsigned char>(c))) {
            tokens.push_back(scanNumber());
            continue;
        }
        // ── Identifiers / keywords
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            tokens.push_back(scanIdentifierOrKeyword());
            continue;
        }
        // ── Operators / delimiters
        tokens.push_back(scanOperator());
    }
    return tokens;
}

bool Lexer::hasErrors() const {
    for (auto& d : diags_)
        if (d.isFatal) return true;
    return !diags_.empty();
}

// ─────────────────────────────────────────────
//  Character helpers
// ─────────────────────────────────────────────
bool Lexer::atEnd() const noexcept {
    return pos_ >= src_.size();
}

char Lexer::peek(std::size_t offset) const noexcept {
    std::size_t idx = pos_ + offset;
    return (idx < src_.size()) ? src_[idx] : '\0';
}

char Lexer::advance() {
    char c = src_[pos_++];
    if (c == '\n') { ++line_; col_ = 1; }
    else            { ++col_; }
    return c;
}

bool Lexer::match(char expected) {
    if (atEnd() || src_[pos_] != expected) return false;
    advance();
    return true;
}

void Lexer::skipWhitespace() {
    while (!atEnd()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
            advance();
        else
            break;
    }
}

// ─────────────────────────────────────────────
//  Comments
// ─────────────────────────────────────────────
void Lexer::skipLineComment() {
    // consume ~~
    advance(); advance();
    while (!atEnd() && peek() != '\n')
        advance();
}

void Lexer::skipBlockComment() {
    // consume <<
    advance(); advance();
    while (!atEnd()) {
        if (peek() == '>' && peek(1) == '>') {
            advance(); advance(); // consume >>
            return;
        }
        advance();
    }
    emitError("Unterminated block comment (missing >>)", true);
}

// ─────────────────────────────────────────────
//  Token builders
// ─────────────────────────────────────────────
void Lexer::markStart() {
    startLoc_ = currentLoc();
}

SourceLocation Lexer::currentLoc() const {
    return {line_, col_, file_};
}

Token Lexer::makeToken(TokenKind k, std::string lexeme, std::string value) {
    if (value.empty()) value = lexeme;
    return Token(k, std::move(lexeme), std::move(value), startLoc_);
}

Token Lexer::errorToken(const std::string& msg) {
    emitError(msg, false);
    return makeToken(TokenKind::ERROR, std::string(1, peek()), msg);
}

void Lexer::emitError(const std::string& msg, bool fatal) {
    LexerDiagnostic d;
    d.loc     = startLoc_;
    d.message = msg;
    d.isFatal = fatal;
    diags_.push_back(d);
    if (diagCb_) diagCb_(d);
}

// ─────────────────────────────────────────────
//  String literals
//  Supports:  "hello"  'hello'
//  Escape sequences: \n \t \r \\ \" \'
// ─────────────────────────────────────────────
Token Lexer::scanString(char quote) {
    advance(); // consume opening quote
    std::string raw;
    raw += quote;
    std::string val;
    bool escaped = false;

    while (!atEnd()) {
        char c = peek();
        if (escaped) {
            raw += c; advance();
            switch (c) {
                case 'n':  val += '\n'; break;
                case 't':  val += '\t'; break;
                case 'r':  val += '\r'; break;
                case '\\': val += '\\'; break;
                case '"':  val += '"';  break;
                case '\'': val += '\''; break;
                case '0':  val += '\0'; break;
                default:
                    emitError(std::string("Unknown escape sequence: \\") + c);
                    val += c;
            }
            escaped = false;
        } else if (c == '\\') {
            raw += c; advance(); escaped = true;
        } else if (c == quote) {
            raw += c; advance();
            raw += quote; // include closing quote in lexeme
            return makeToken(TokenKind::STRING, raw, val);
        } else if (c == '\n') {
            emitError("Unterminated string literal (newline in string)");
            return makeToken(TokenKind::ERROR, raw, "unterminated string");
        } else {
            raw += c; val += c; advance();
        }
    }
    emitError("Unterminated string literal (reached EOF)", true);
    return makeToken(TokenKind::ERROR, raw, "unterminated string");
}

// ─────────────────────────────────────────────
//  Numbers
//  integer: 42  0xFF  0b1010  0o17
//  float:   3.14  1.0e10  1.5e-3
// ─────────────────────────────────────────────
Token Lexer::scanNumber() {
    std::string raw;
    bool isFloat = false;

    // Hex / Binary / Octal prefix
    if (peek() == '0') {
        char next = peek(1);
        if (next == 'x' || next == 'X') {
            raw += advance(); raw += advance(); // 0x
            while (!atEnd() && std::isxdigit(static_cast<unsigned char>(peek())))
                raw += advance();
            return makeToken(TokenKind::NUMBER, raw, raw);
        }
        if (next == 'b' || next == 'B') {
            raw += advance(); raw += advance(); // 0b
            while (!atEnd() && (peek() == '0' || peek() == '1'))
                raw += advance();
            return makeToken(TokenKind::NUMBER, raw, raw);
        }
        if (next == 'o' || next == 'O') {
            raw += advance(); raw += advance(); // 0o
            while (!atEnd() && peek() >= '0' && peek() <= '7')
                raw += advance();
            return makeToken(TokenKind::NUMBER, raw, raw);
        }
    }

    // Decimal integer part
    while (!atEnd() && std::isdigit(static_cast<unsigned char>(peek())))
        raw += advance();

    // Fractional part
    if (!atEnd() && peek() == '.' &&
        std::isdigit(static_cast<unsigned char>(peek(1)))) {
        isFloat = true;
        raw += advance(); // consume '.'
        while (!atEnd() && std::isdigit(static_cast<unsigned char>(peek())))
            raw += advance();
    }

    // Exponent part
    if (!atEnd() && (peek() == 'e' || peek() == 'E')) {
        isFloat = true;
        raw += advance();
        if (!atEnd() && (peek() == '+' || peek() == '-'))
            raw += advance();
        while (!atEnd() && std::isdigit(static_cast<unsigned char>(peek())))
            raw += advance();
    }

    // Optional type suffix: u8 u16 u32 u64 i8 i16 i32 i64 f32 f64
    if (!atEnd() && std::isalpha(static_cast<unsigned char>(peek()))) {
        std::string suffix;
        while (!atEnd() && std::isalnum(static_cast<unsigned char>(peek())))
            suffix += advance();
        raw += suffix;
    }

    (void)isFloat; // used for future semantic tagging
    return makeToken(TokenKind::NUMBER, raw, raw);
}

// ─────────────────────────────────────────────
//  Identifiers & keywords
// ─────────────────────────────────────────────
Token Lexer::scanIdentifierOrKeyword() {
    std::string raw;
    while (!atEnd()) {
        char c = peek();
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_')
            raw += advance();
        else
            break;
    }

    auto& kw = getKeywordMap();
    auto  it = kw.find(raw);
    if (it != kw.end())
        return makeToken(it->second, raw, raw);

    return makeToken(TokenKind::IDENTIFIER, raw, raw);
}

// ─────────────────────────────────────────────
//  Operators & delimiters
//  Riven-specific:  +>  (INCREMENT)   -<  (DECREMENT)
// ─────────────────────────────────────────────
Token Lexer::scanOperator() {
    char c = advance();
    std::string raw(1, c);

    switch (c) {
        // ── Parentheses / brackets ──────────
        case '(': return makeToken(TokenKind::LPAREN,   raw);
        case ')': return makeToken(TokenKind::RPAREN,   raw);
        case '{': return makeToken(TokenKind::LBRACE,   raw);
        case '}': return makeToken(TokenKind::RBRACE,   raw);
        case '[': return makeToken(TokenKind::LBRACKET, raw);
        case ']': return makeToken(TokenKind::RBRACKET, raw);
        // ── Punctuation ─────────────────────
        case ',': return makeToken(TokenKind::COMMA,     raw);
        case ';': return makeToken(TokenKind::SEMICOLON, raw);
        case '.': return makeToken(TokenKind::DOT,       raw);
        case '?': return makeToken(TokenKind::QUESTION,  raw);
        case '~': return makeToken(TokenKind::TILDE,     raw);

        // ── Colon / double colon ─────────────
        case ':':
            if (match(':')) return makeToken(TokenKind::DOUBLE_COLON, "::");
            return makeToken(TokenKind::COLON, raw);

        // ── Plus: +=  +>  + ─────────────────
        case '+':
            if (match('=')) return makeToken(TokenKind::PLUS_ASSIGN,  "+=");
            if (match('>')) return makeToken(TokenKind::INCREMENT,     "+>");
            return makeToken(TokenKind::PLUS, raw);

        // ── Minus: -=  -<  ->  - ────────────
        case '-':
            if (match('=')) return makeToken(TokenKind::MINUS_ASSIGN, "-=");
            if (match('<')) return makeToken(TokenKind::DECREMENT,     "-<");
            if (match('>')) return makeToken(TokenKind::ARROW,         "->");
            return makeToken(TokenKind::MINUS, raw);

        // ── Star: *=  * ─────────────────────
        case '*':
            if (match('=')) return makeToken(TokenKind::STAR_ASSIGN,  "*=");
            return makeToken(TokenKind::STAR, raw);

        // ── Slash: /=  / ────────────────────
        case '/':
            if (match('=')) return makeToken(TokenKind::SLASH_ASSIGN, "/=");
            return makeToken(TokenKind::SLASH, raw);

        // ── Percent: %=  % ──────────────────
        case '%':
            if (match('=')) return makeToken(TokenKind::PERCENT_ASSIGN, "%=");
            return makeToken(TokenKind::PERCENT, raw);

        // ── Caret ────────────────────────────
        case '^': return makeToken(TokenKind::CARET, raw);

        // ── Amp: &&  & ───────────────────────
        case '&':
            if (match('&')) return makeToken(TokenKind::AND_AND, "&&");
            return makeToken(TokenKind::AMP, raw);

        // ── Pipe: ||  | ──────────────────────
        case '|':
            if (match('|')) return makeToken(TokenKind::OR_OR, "||");
            return makeToken(TokenKind::PIPE, raw);

        // ── Bang: !=  ! ──────────────────────
        case '!':
            if (match('=')) return makeToken(TokenKind::BANG_EQUAL, "!=");
            return makeToken(TokenKind::BANG, raw);

        // ── Equal: ==  =>  = ─────────────────
        case '=':
            if (match('=')) return makeToken(TokenKind::EQUAL_EQUAL, "==");
            if (match('>')) return makeToken(TokenKind::FAT_ARROW,   "=>");
            return makeToken(TokenKind::ASSIGN, raw);

        // ── Less: <=  << (comment, handled above)  < ────
        case '<':
            if (match('=')) return makeToken(TokenKind::LESS_EQUAL, "<=");
            return makeToken(TokenKind::LESS, raw);

        // ── Greater: >=  > ───────────────────
        case '>':
            if (match('=')) return makeToken(TokenKind::GREATER_EQUAL, ">=");
            return makeToken(TokenKind::GREATER, raw);

        default: {
            std::ostringstream msg;
            msg << "Unexpected character '" << c
                << "' (0x" << std::hex << static_cast<int>(c) << ")";
            advance(); // skip bad char
            return errorToken(msg.str());
        }
    }
}
