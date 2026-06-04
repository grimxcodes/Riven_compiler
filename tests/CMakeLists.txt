add_executable(test_lexer
    test_lexer.cpp
)
target_link_libraries(test_lexer PRIVATE riven_lexer)

add_test(NAME LexerTests COMMAND test_lexer)
