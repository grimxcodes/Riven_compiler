#include "token.h"
#include <sstream>

// ──────────────────────────────────────────────
//  Keyword map (must match TokenKind exactly)
// ──────────────────────────────────────────────
const std::unordered_map<std::string, TokenKind>& getKeywordMap() {
    static const std::unordered_map<std::string, TokenKind> kw = {
        // Core structure
        {"riven",      TokenKind::RIVEN},
        {"core",       TokenKind::CORE},
        {"firm",       TokenKind::FIRM},
        // Functions
        {"craft",      TokenKind::CRAFT},
        {"returns",    TokenKind::RETURNS},
        // Control flow
        {"if",         TokenKind::IF},
        {"altif",      TokenKind::ALTIF},
        {"else",       TokenKind::ELSE},
        {"flow",       TokenKind::FLOW},
        {"during",     TokenKind::DURING},
        {"break",      TokenKind::BREAK},
        {"continue",   TokenKind::CONTINUE},
        {"return",     TokenKind::RETURN},
        // Inc/Dec readable
        {"rise",       TokenKind::RISE},
        {"drop",       TokenKind::DROP},
        // Collections / OOP
        {"coll",       TokenKind::COLL},
        {"rec",        TokenKind::REC},
        {"frame",      TokenKind::FRAME},
        {"boot",       TokenKind::BOOT},
        {"spawn",      TokenKind::SPAWN},
        // Access control
        {"hidden",     TokenKind::HIDDEN},
        {"open",       TokenKind::OPEN},
        // Imports
        {"consistof",  TokenKind::CONSISTOF},
        {"import",     TokenKind::IMPORT},
        {"from",       TokenKind::FROM},
        {"as",         TokenKind::AS},
        {"in",         TokenKind::IN},
        {"of",         TokenKind::OF},
        // Error handling
        {"resc",       TokenKind::RESC},
        {"attack",     TokenKind::ATTACK},
        // Async
        {"spark",      TokenKind::SPARK},
        {"sync",       TokenKind::SYNC},
        // Memory
        {"ref",        TokenKind::REF},
        {"ptr",        TokenKind::PTR},
        {"raw",        TokenKind::RAW},
        // Boolean word operators
        {"and",        TokenKind::AND_KW},
        {"or",         TokenKind::OR_KW},
        {"not",        TokenKind::NOT_KW},
        // Literals
        {"true",       TokenKind::TRUE_KW},
        {"false",      TokenKind::FALSE_KW},
        {"null",       TokenKind::NULL_KW},
        // Built-ins
        {"imprint",    TokenKind::IMPRINT},
        {"inp",        TokenKind::INP},
        // OOP extras
        {"self",       TokenKind::SELF},
        {"extends",    TokenKind::EXTENDS},
        {"implements", TokenKind::IMPLEMENTS},
        {"interface",  TokenKind::INTERFACE},
        {"enum",       TokenKind::ENUM_KW},
        // Pattern matching
        {"match",      TokenKind::MATCH},
        {"case",       TokenKind::CASE},
        {"default",    TokenKind::DEFAULT},
    };
    return kw;
}

// ──────────────────────────────────────────────
//  TokenKind → name string
// ──────────────────────────────────────────────
std::string Token::kindName() const {
    switch (kind) {
#define K(x) case TokenKind::x: return #x
        K(NUMBER); K(STRING); K(IDENTIFIER);
        K(RIVEN); K(CORE); K(FIRM); K(CRAFT); K(RETURNS);
        K(IF); K(ALTIF); K(ELSE); K(FLOW); K(DURING);
        K(RISE); K(DROP); K(COLL); K(REC); K(FRAME);
        K(BOOT); K(SPAWN); K(HIDDEN); K(OPEN);
        K(CONSISTOF); K(IMPORT); K(FROM); K(AS); K(IN); K(OF);
        K(RESC); K(ATTACK); K(SPARK); K(SYNC);
        K(REF); K(PTR); K(RAW);
        K(AND_KW); K(OR_KW); K(NOT_KW);
        K(TRUE_KW); K(FALSE_KW); K(NULL_KW);
        K(IMPRINT); K(INP);
        K(BREAK); K(CONTINUE); K(RETURN); K(SELF);
        K(EXTENDS); K(IMPLEMENTS); K(INTERFACE); K(ENUM_KW);
        K(MATCH); K(CASE); K(DEFAULT);
        K(ASSIGN); K(PLUS); K(MINUS); K(STAR); K(SLASH);
        K(PERCENT); K(CARET); K(AMP); K(PIPE); K(TILDE);
        K(PLUS_ASSIGN); K(MINUS_ASSIGN); K(STAR_ASSIGN);
        K(SLASH_ASSIGN); K(PERCENT_ASSIGN);
        K(EQUAL_EQUAL); K(BANG_EQUAL);
        K(LESS); K(GREATER); K(LESS_EQUAL); K(GREATER_EQUAL);
        K(AND_AND); K(OR_OR); K(BANG);
        K(ARROW); K(FAT_ARROW); K(DOUBLE_COLON);
        K(INCREMENT); K(DECREMENT);
        K(DOT); K(COMMA); K(COLON); K(SEMICOLON); K(QUESTION);
        K(LPAREN); K(RPAREN); K(LBRACE); K(RBRACE);
        K(LBRACKET); K(RBRACKET);
        K(EOF_TOKEN); K(ERROR);
#undef K
        default: return "UNKNOWN";
    }
}

std::string Token::toString() const {
    std::ostringstream os;
    os << "[" << kindName();
    if (!value.empty() && value != lexeme)
        os << " lexeme='" << lexeme << "' value='" << value << "'";
    else if (!lexeme.empty())
        os << " '" << lexeme << "'";
    os << " @" << loc.line << ":" << loc.column << "]";
    return os.str();
}
