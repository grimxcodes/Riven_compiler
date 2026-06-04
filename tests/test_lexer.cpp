// Minimal self-contained test runner (no external framework needed)
#include "../include/lexer.h"
#include <cassert>
#include <iostream>
#include <string>

inline std::ostream& operator<<(std::ostream& os, TokenKind kind) {
    return os << static_cast<int>(kind);
}


// ─────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────
static int  g_passed = 0;
static int  g_failed = 0;

#define CHECK_EQ(a,b) do {                                          \
    if ((a) != (b)) {                                               \
        std::cerr << "  FAIL  " << __FILE__ << ":" << __LINE__     \
                  << "  expected=" << (b) << "  got=" << (a) << "\n"; \
        ++g_failed;                                                 \
    } else { ++g_passed; }                                          \
} while(0)

#define CHECK(expr) do {                                            \
    if (!(expr)) {                                                  \
        std::cerr << "  FAIL  " << __FILE__ << ":" << __LINE__     \
                  << "  assertion failed: " #expr "\n";            \
        ++g_failed;                                                 \
    } else { ++g_passed; }                                          \
} while(0)

// Tokenize and return kind list (excluding EOF)
static std::vector<TokenKind> kinds(const std::string& src) {
    Lexer lex(src, "<test>");
    auto toks = lex.tokenize();
    std::vector<TokenKind> ks;
    for (auto& t : toks)
        if (t.kind != TokenKind::EOF_TOKEN) ks.push_back(t.kind);
    return ks;
}

// ─────────────────────────────────────────────
//  Tests
// ─────────────────────────────────────────────

void test_entry_point() {
    std::cout << "[test_entry_point]\n";
    auto k = kinds("riven core { }");
    CHECK_EQ(k.size(), 4u);
    CHECK_EQ(k[0], TokenKind::RIVEN);
    CHECK_EQ(k[1], TokenKind::CORE);
    CHECK_EQ(k[2], TokenKind::LBRACE);
    CHECK_EQ(k[3], TokenKind::RBRACE);
}

void test_variables() {
    std::cout << "[test_variables]\n";
    auto k = kinds(R"(name = "Ansh"  age = 18  price = 10.5)");
    CHECK_EQ(k[0], TokenKind::IDENTIFIER);
    CHECK_EQ(k[1], TokenKind::ASSIGN);
    CHECK_EQ(k[2], TokenKind::STRING);
    CHECK_EQ(k[3], TokenKind::IDENTIFIER);
    CHECK_EQ(k[4], TokenKind::ASSIGN);
    CHECK_EQ(k[5], TokenKind::NUMBER);
    CHECK_EQ(k[6], TokenKind::IDENTIFIER);
    CHECK_EQ(k[7], TokenKind::ASSIGN);
    CHECK_EQ(k[8], TokenKind::NUMBER);
}

void test_constants() {
    std::cout << "[test_constants]\n";
    auto k = kinds("firm version = 1.0");
    CHECK_EQ(k[0], TokenKind::FIRM);
    CHECK_EQ(k[1], TokenKind::IDENTIFIER);
    CHECK_EQ(k[2], TokenKind::ASSIGN);
    CHECK_EQ(k[3], TokenKind::NUMBER);
}

void test_imprint() {
    std::cout << "[test_imprint]\n";
    auto k = kinds(R"(imprint("Hello World"))");
    CHECK_EQ(k[0], TokenKind::IMPRINT);
    CHECK_EQ(k[1], TokenKind::LPAREN);
    CHECK_EQ(k[2], TokenKind::STRING);
    CHECK_EQ(k[3], TokenKind::RPAREN);
}

void test_inp() {
    std::cout << "[test_inp]\n";
    auto k = kinds(R"(name = inp("Enter Name"))");
    CHECK_EQ(k[0], TokenKind::IDENTIFIER);
    CHECK_EQ(k[1], TokenKind::ASSIGN);
    CHECK_EQ(k[2], TokenKind::INP);
    CHECK_EQ(k[3], TokenKind::LPAREN);
    CHECK_EQ(k[4], TokenKind::STRING);
    CHECK_EQ(k[5], TokenKind::RPAREN);
}

void test_craft_function() {
    std::cout << "[test_craft_function]\n";
    auto k = kinds("craft add(a,b){ returns a+b }");
    CHECK_EQ(k[0], TokenKind::CRAFT);
    CHECK_EQ(k[1], TokenKind::IDENTIFIER);  // add
    CHECK_EQ(k[2], TokenKind::LPAREN);
    CHECK_EQ(k[3], TokenKind::IDENTIFIER);  // a
    CHECK_EQ(k[4], TokenKind::COMMA);
    CHECK_EQ(k[5], TokenKind::IDENTIFIER);  // b
    CHECK_EQ(k[6], TokenKind::RPAREN);
    CHECK_EQ(k[7], TokenKind::LBRACE);
    CHECK_EQ(k[8], TokenKind::RETURNS);
    CHECK_EQ(k[9], TokenKind::IDENTIFIER);  // a
    CHECK_EQ(k[10], TokenKind::PLUS);
    CHECK_EQ(k[11], TokenKind::IDENTIFIER); // b
    CHECK_EQ(k[12], TokenKind::RBRACE);
}

void test_conditions() {
    std::cout << "[test_conditions]\n";
    auto k = kinds("if age > 18 { } altif age == 18 { } else { }");
    CHECK_EQ(k[0], TokenKind::IF);
    CHECK_EQ(k[3], TokenKind::GREATER);
    CHECK_EQ(k[6], TokenKind::ALTIF);
    CHECK_EQ(k[9], TokenKind::EQUAL_EQUAL);
    CHECK_EQ(k[12], TokenKind::ELSE);
}

void test_bool_operators() {
    std::cout << "[test_bool_operators]\n";
    auto k = kinds("and or not && || !");
    CHECK_EQ(k[0], TokenKind::AND_KW);
    CHECK_EQ(k[1], TokenKind::OR_KW);
    CHECK_EQ(k[2], TokenKind::NOT_KW);
    CHECK_EQ(k[3], TokenKind::AND_AND);
    CHECK_EQ(k[4], TokenKind::OR_OR);
    CHECK_EQ(k[5], TokenKind::BANG);
}

void test_loops() {
    std::cout << "[test_loops]\n";
    auto k = kinds("flow 10 { } during score < 10 { }");
    CHECK_EQ(k[0], TokenKind::FLOW);
    CHECK_EQ(k[1], TokenKind::NUMBER);
    CHECK_EQ(k[4], TokenKind::DURING);
    CHECK_EQ(k[5], TokenKind::IDENTIFIER);
    CHECK_EQ(k[6], TokenKind::LESS);
    CHECK_EQ(k[7], TokenKind::NUMBER);
}

void test_increment_decrement() {
    std::cout << "[test_increment_decrement]\n";
    // Symbol forms
    auto k = kinds("score+> score-<");
    CHECK_EQ(k[0], TokenKind::IDENTIFIER);
    CHECK_EQ(k[1], TokenKind::INCREMENT);
    CHECK_EQ(k[2], TokenKind::IDENTIFIER);
    CHECK_EQ(k[3], TokenKind::DECREMENT);
    // Readable forms
    auto k2 = kinds("rise score drop score");
    CHECK_EQ(k2[0], TokenKind::RISE);
    CHECK_EQ(k2[2], TokenKind::DROP);
}

void test_collection() {
    std::cout << "[test_collection]\n";
    auto k = kinds("coll nums = [1,2,3]");
    CHECK_EQ(k[0], TokenKind::COLL);
    CHECK_EQ(k[1], TokenKind::IDENTIFIER);
    CHECK_EQ(k[2], TokenKind::ASSIGN);
    CHECK_EQ(k[3], TokenKind::LBRACKET);
    CHECK_EQ(k[7], TokenKind::RBRACKET);
}

void test_record() {
    std::cout << "[test_record]\n";
    auto k = kinds(R"(rec user = { name = "Ansh" })");
    CHECK_EQ(k[0], TokenKind::REC);
    CHECK_EQ(k[1], TokenKind::IDENTIFIER);
    CHECK_EQ(k[2], TokenKind::ASSIGN);
    CHECK_EQ(k[3], TokenKind::LBRACE);
    CHECK_EQ(k[4], TokenKind::IDENTIFIER); // name
    CHECK_EQ(k[5], TokenKind::ASSIGN);
    CHECK_EQ(k[6], TokenKind::STRING);
    CHECK_EQ(k[7], TokenKind::RBRACE);
}

void test_frame_oop() {
    std::cout << "[test_frame_oop]\n";
    auto k = kinds("frame User { } boot(){ } user = spawn User()");
    CHECK_EQ(k[0], TokenKind::FRAME);
    CHECK_EQ(k[1], TokenKind::IDENTIFIER);
    CHECK_EQ(k[4], TokenKind::BOOT);
    CHECK_EQ(k[9], TokenKind::SPAWN);
    CHECK_EQ(k[10], TokenKind::IDENTIFIER);
}

void test_access_control() {
    std::cout << "[test_access_control]\n";
    auto k = kinds("hidden pin open login(){}");
    CHECK_EQ(k[0], TokenKind::HIDDEN);
    CHECK_EQ(k[1], TokenKind::IDENTIFIER);
    CHECK_EQ(k[2], TokenKind::OPEN);
    CHECK_EQ(k[3], TokenKind::IDENTIFIER);
}

void test_import() {
    std::cout << "[test_import]\n";
    auto k = kinds(R"(consistof "system.rvh")");
    CHECK_EQ(k[0], TokenKind::CONSISTOF);
    CHECK_EQ(k[1], TokenKind::STRING);
}

void test_error_handling_kw() {
    std::cout << "[test_error_handling_kw]\n";
    auto k = kinds(R"(resc { } attack("Missing File"))");
    CHECK_EQ(k[0], TokenKind::RESC);
    CHECK_EQ(k[3], TokenKind::ATTACK);
    CHECK_EQ(k[5], TokenKind::STRING);
}

void test_async() {
    std::cout << "[test_async]\n";
    auto k = kinds("spark craft music(){ } sync;");
    CHECK_EQ(k[0], TokenKind::SPARK);
    CHECK_EQ(k[1], TokenKind::CRAFT);
    CHECK_EQ(k[2], TokenKind::IDENTIFIER);
    auto k2 = kinds("sync;");
    CHECK_EQ(k2[0], TokenKind::SYNC);
    CHECK_EQ(k2[1], TokenKind::SEMICOLON);
}

void test_ref_ptr() {
    std::cout << "[test_ref_ptr]\n";
    auto k = kinds("ref data = user ptr memory = kernel");
    CHECK_EQ(k[0], TokenKind::REF);
    CHECK_EQ(k[4], TokenKind::PTR);
}

void test_raw_unsafe() {
    std::cout << "[test_raw_unsafe]\n";
    auto k = kinds("raw { }");
    CHECK_EQ(k[0], TokenKind::RAW);
    CHECK_EQ(k[1], TokenKind::LBRACE);
    CHECK_EQ(k[2], TokenKind::RBRACE);
}

void test_line_comment() {
    std::cout << "[test_line_comment]\n";
    auto k = kinds("age = 18 ~~ this is a comment\n name = \"Ansh\"");
    // comment tokens must be ignored
    CHECK_EQ(k[0], TokenKind::IDENTIFIER);
    CHECK_EQ(k[3], TokenKind::IDENTIFIER);
    CHECK_EQ(k.size(), 6u);
}

void test_block_comment() {
    std::cout << "[test_block_comment]\n";
    auto k = kinds("age = 18 << this is\na block comment >> name = 20");
    CHECK_EQ(k[0], TokenKind::IDENTIFIER);
    CHECK_EQ(k[1], TokenKind::ASSIGN);
    CHECK_EQ(k[2], TokenKind::NUMBER);
    CHECK_EQ(k[3], TokenKind::IDENTIFIER); // name
    CHECK_EQ(k.size(), 6u);
}

void test_operators_full() {
    std::cout << "[test_operators_full]\n";
    auto k = kinds("+= -= *= /= %= == != <= >= -> => ::");
    CHECK_EQ(k[0], TokenKind::PLUS_ASSIGN);
    CHECK_EQ(k[1], TokenKind::MINUS_ASSIGN);
    CHECK_EQ(k[2], TokenKind::STAR_ASSIGN);
    CHECK_EQ(k[3], TokenKind::SLASH_ASSIGN);
    CHECK_EQ(k[4], TokenKind::PERCENT_ASSIGN);
    CHECK_EQ(k[5], TokenKind::EQUAL_EQUAL);
    CHECK_EQ(k[6], TokenKind::BANG_EQUAL);
    CHECK_EQ(k[7], TokenKind::LESS_EQUAL);
    CHECK_EQ(k[8], TokenKind::GREATER_EQUAL);
    CHECK_EQ(k[9], TokenKind::ARROW);
    CHECK_EQ(k[10], TokenKind::FAT_ARROW);
    CHECK_EQ(k[11], TokenKind::DOUBLE_COLON);
}

void test_string_escape() {
    std::cout << "[test_string_escape]\n";
    Lexer lex(R"("hello\nworld")", "<test>");
    auto toks = lex.tokenize();
    CHECK_EQ(toks[0].kind, TokenKind::STRING);
    CHECK_EQ(toks[0].value, "hello\nworld");
    CHECK(lex.diagnostics().empty());
}

void test_location_tracking() {
    std::cout << "[test_location_tracking]\n";
    Lexer lex("riven\ncore", "<test>");
    auto toks = lex.tokenize();
    CHECK_EQ(toks[0].loc.line, 1u);
    CHECK_EQ(toks[0].loc.column, 1u);
    CHECK_EQ(toks[1].loc.line, 2u);
    CHECK_EQ(toks[1].loc.column, 1u);
}

void test_hex_binary_octal() {
    std::cout << "[test_hex_binary_octal]\n";
    auto k = kinds("0xFF 0b1010 0o17");
    CHECK_EQ(k[0], TokenKind::NUMBER);
    CHECK_EQ(k[1], TokenKind::NUMBER);
    CHECK_EQ(k[2], TokenKind::NUMBER);

    Lexer lex("0xFF 0b1010 0o17", "<test>");
    auto toks = lex.tokenize();
    CHECK_EQ(toks[0].lexeme, "0xFF");
    CHECK_EQ(toks[1].lexeme, "0b1010");
    CHECK_EQ(toks[2].lexeme, "0o17");
}

void test_full_program() {
    std::cout << "[test_full_program]\n";
    const char* prog = R"(
~~ Riven sample program
riven core {
    firm PI = 3.14
    name = inp("Enter name: ")
    imprint("Hello, ")
    coll nums = [1, 2, 3]
    flow 3 {
        imprint("loop")
    }
    craft add(a, b) {
        returns a + b
    }
    frame Dog {
        boot() { }
        open bark() { imprint("Woof") }
    }
    d = spawn Dog()
    if PI > 3.0 { imprint("yes") }
    altif PI == 3.0 { } else { }
    score+>
    drop score
    ref r = name
    ptr p = name
    raw { }
    spark craft fetch() { }
    sync;
    consistof "net.rvh"
    resc { attack("err") }
}
)";
    Lexer lex(prog, "sample.riven");
    auto toks = lex.tokenize();
    CHECK(toks.size() > 50);
    CHECK(toks.back().kind == TokenKind::EOF_TOKEN);
    CHECK(lex.diagnostics().empty());
    std::cout << "  Total tokens: " << toks.size() << "\n";
}

// ─────────────────────────────────────────────
//  main
// ─────────────────────────────────────────────
int main() {
    std::cout << "=== Riven Lexer Unit Tests ===\n\n";

    test_entry_point();
    test_variables();
    test_constants();
    test_imprint();
    test_inp();
    test_craft_function();
    test_conditions();
    test_bool_operators();
    test_loops();
    test_increment_decrement();
    test_collection();
    test_record();
    test_frame_oop();
    test_access_control();
    test_import();
    test_error_handling_kw();
    test_async();
    test_ref_ptr();
    test_raw_unsafe();
    test_line_comment();
    test_block_comment();
    test_operators_full();
    test_string_escape();
    test_location_tracking();
    test_hex_binary_octal();
    test_full_program();

    std::cout << "\n=== Results ===\n";
    std::cout << "  Passed: " << g_passed << "\n";
    std::cout << "  Failed: " << g_failed << "\n";
    return (g_failed == 0) ? 0 : 1;
}
