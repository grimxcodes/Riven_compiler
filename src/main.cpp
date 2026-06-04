// ─────────────────────────────────────────────
//  Riven Compiler — Driver  (Phase 1: Lexer)
//  Usage:  rivenc <source.riven> [--tokens] [--dump]
// ─────────────────────────────────────────────
#include "lexer.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>

static void printUsage(const char* argv0) {
    std::cerr
        << "Riven Compiler v0.1.0  (Phase 1 — Lexer)\n"
        << "Usage: " << argv0 << " <file.riven> [options]\n"
        << "Options:\n"
        << "  --tokens    Print token stream to stdout\n"
        << "  --dump      Same as --tokens (alias)\n"
        << "  --help      Show this message\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) { printUsage(argv[0]); return 1; }

    std::string  filepath;
    bool         printTokens = false;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--help") == 0) {
            printUsage(argv[0]); return 0;
        } else if (std::strcmp(argv[i], "--tokens") == 0 ||
                   std::strcmp(argv[i], "--dump")   == 0) {
            printTokens = true;
        } else {
            filepath = argv[i];
        }
    }

    if (filepath.empty()) {
        std::cerr << "Error: no input file specified.\n";
        return 1;
    }

    // ── Read source file ──────────────────────
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Error: cannot open '" << filepath << "'\n";
        return 1;
    }
    std::ostringstream buf;
    buf << file.rdbuf();
    std::string source = buf.str();

    // ── Diagnostic callback (errors → stderr) ─
    auto diagCb = [](const LexerDiagnostic& d) {
        std::cerr
            << (d.isFatal ? "fatal" : "error")
            << ": " << d.loc.file
            << ":" << d.loc.line
            << ":" << d.loc.column
            << ": " << d.message << "\n";
    };

    // ── Run lexer ─────────────────────────────
    Lexer lexer(source, filepath, diagCb);
    auto  tokens = lexer.tokenize();

    // ── Print diagnostics summary ─────────────
    const auto& diags = lexer.diagnostics();
    if (!diags.empty()) {
        std::cerr << diags.size() << " diagnostic(s) emitted.\n";
    }

    // ── Optionally dump token stream ──────────
    if (printTokens) {
        std::cout << "─── Token Stream (" << tokens.size() << " tokens) ───\n";
        for (auto& tok : tokens) {
            std::cout << tok.toString() << "\n";
        }
    }

    // ── Exit code ─────────────────────────────
    if (lexer.hasErrors()) {
        std::cerr << "Compilation failed at Phase 1 (Lexer).\n";
        return 1;
    }

    if (!printTokens) {
        std::cout << "Lexing succeeded: "
                  << tokens.size() << " tokens  ("
                  << filepath << ")\n";
        std::cout << "Phase 2 (Parser) not yet implemented.\n";
    }
    return 0;
}
