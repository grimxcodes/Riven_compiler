#pragma once
#include "token.h"
#include <string>
#include <vector>
#include <functional>

// ──────────────────────────────────────────────
//  Diagnostic (non-fatal lexer error)
// ──────────────────────────────────────────────
struct LexerDiagnostic {
    SourceLocation loc;
    std::string    message;
    bool           isFatal = false;
};

// ──────────────────────────────────────────────
//  Lexer
// ──────────────────────────────────────────────
class Lexer {
public:
    // Callback type for diagnostics (optional)
    using DiagCallback = std::function<void(const LexerDiagnostic&)>;

    explicit Lexer(std::string source,
                   std::string filename = "<source>",
                   DiagCallback cb = nullptr);

    // Tokenise entire source; returns token list ending with EOF_TOKEN
    std::vector<Token> tokenize();

    // Access diagnostics after tokenize()
    [[nodiscard]] const std::vector<LexerDiagnostic>& diagnostics() const {
        return diags_;
    }
    [[nodiscard]] bool hasErrors() const;

private:
    // ── source state ─────────────────────────
    std::string src_;
    std::string file_;
    std::size_t pos_    = 0;   // current index
    std::size_t line_   = 1;
    std::size_t col_    = 1;
    std::size_t tokLine_= 1;   // start of current token
    std::size_t tokCol_ = 1;

    std::vector<LexerDiagnostic> diags_;
    DiagCallback diagCb_;

    // ── character helpers ────────────────────
    [[nodiscard]] bool   atEnd()  const noexcept;
    [[nodiscard]] char   peek(std::size_t offset = 0) const noexcept;
    char advance();
    bool match(char expected);
    void skipWhitespace();

    // ── comment helpers ──────────────────────
    void skipLineComment();    // ~~ …
    void skipBlockComment();   // << … >>

    // ── token builders ───────────────────────
    Token makeToken(TokenKind k, std::string lexeme, std::string value = "");
    Token errorToken(const std::string& msg);
    void  markStart();                   // record token start pos
    SourceLocation currentLoc() const;
    SourceLocation startLoc_  {};

    // ── scan helpers ─────────────────────────
    Token scanString(char quote);
    Token scanNumber();
    Token scanIdentifierOrKeyword();
    Token scanOperator();

    // ── error reporting ──────────────────────
    void emitError(const std::string& msg, bool fatal = false);
};
