# Riven Compiler

> **Phase 1 — Lexer** (complete)  
> A fully from-scratch compiler for the **Riven** system-level programming language, written in modern C++20.

---

## Project Roadmap

| Phase | Component                        | Status        |
|-------|----------------------------------|---------------|
| 1     | Lexer / Tokenizer                | ✅ Complete   |
| 2     | Parser + AST                     | 🔜 Next       |
| 3     | Semantic Analyzer + Type System  | 🔜 Planned    |
| 4     | Interpreter Runtime              | 🔜 Planned    |
| 5     | Riven IR (custom)                | 🔜 Planned    |
| 6     | Optimizer                        | 🔜 Planned    |
| 7     | x86-64 Backend (from scratch)    | 🔜 Planned    |
| 8     | ELF Executable Generator (Linux) | 🔜 Planned    |
| 9     | PE Executable Generator (Windows)| 🔜 Planned    |
| 10    | Linker + Package System          | 🔜 Planned    |
| 11    | Self-hosting Roadmap             | 🔜 Planned    |

---

## Building

### Requirements

- CMake ≥ 3.20
- C++20-capable compiler (GCC 11+, Clang 13+, MSVC 2022+)

### Linux / macOS

```bash
git clone https://github.com/your-username/riven-compiler
cd riven-compiler
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Windows (MSVC)

```powershell
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
```

---

## Running the Compiler

```bash
# Lex a source file and print token stream
./rivenc examples/hello.riven --tokens

# Just check for lex errors
./rivenc examples/hello.riven
```

---

## Running Tests

```bash
cd build
ctest --output-on-failure
# or run directly:
./tests/test_lexer
```

Expected output:
```
=== Riven Lexer Unit Tests ===
[test_entry_point]
[test_variables]
...
=== Results ===
  Passed: 100+
  Failed: 0
```

---

## Language Quick Reference (Phase 1 — Tokens)

```riven
~~ Single-line comment
<< Multi-line
   comment >>

riven core {
    firm PI = 3.14
    name = inp("Enter name: ")
    imprint("Hello!")

    coll nums = [1, 2, 3]

    flow 5 { imprint("loop") }
    during score < 10 { score+> }

    craft add(a, b) { returns a + b }

    frame Dog {
        boot() { }
        open bark() { imprint("Woof") }
    }
    d = spawn Dog()

    ref r = name
    ptr p = name
    raw { }

    spark craft fetch() { }
    sync;

    consistof "net.rvh"
    resc { attack("oops") }
}
```

---

## File Structure

```
riven-compiler/
├── CMakeLists.txt
├── README.md
├── include/
│   ├── token.h        ← Token kinds, Token struct, keyword map
│   └── lexer.h        ← Lexer class declaration
├── src/
│   ├── token.cpp      ← kindName(), toString(), keyword table
│   ├── lexer.cpp      ← Full lexer implementation
│   └── main.cpp       ← Compiler driver (rivenc CLI)
└── tests/
    ├── CMakeLists.txt
    └── test_lexer.cpp ← 25+ unit tests, zero dependencies
```

---

## Keywords Supported (50+)

`riven` `core` `firm` `craft` `returns` `if` `altif` `else`  
`flow` `during` `break` `continue` `return` `rise` `drop`  
`coll` `rec` `frame` `boot` `spawn` `hidden` `open`  
`consistof` `import` `from` `as` `in` `of`  
`resc` `attack` `spark` `sync` `ref` `ptr` `raw`  
`and` `or` `not` `true` `false` `null`  
`imprint` `inp` `self` `extends` `implements` `interface`  
`enum` `match` `case` `default`

---

## Operators Supported

| Symbol | Token          |
|--------|----------------|
| `+>`   | INCREMENT      |
| `-<`   | DECREMENT      |
| `==`   | EQUAL_EQUAL    |
| `!=`   | BANG_EQUAL     |
| `<=`   | LESS_EQUAL     |
| `>=`   | GREATER_EQUAL  |
| `&&`   | AND_AND        |
| `\|\|` | OR_OR          |
| `->`   | ARROW          |
| `=>`   | FAT_ARROW      |
| `::`   | DOUBLE_COLON   |
| `+=` `-=` `*=` `/=` `%=` | compound assigns |

---

*Built from scratch. No LLVM. No GCC. No third-party codegen.*
