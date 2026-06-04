#!/usr/bin/env bash
# Creates riven-compiler.zip ready for GitHub upload

set -e

ROOT="riven-compiler"

# Create directory structure
mkdir -p "$ROOT/include"
mkdir -p "$ROOT/src"
mkdir -p "$ROOT/tests"
mkdir -p "$ROOT/examples"

# ── Copy files (assumes you saved them with the names below) ──
cp include/token.h      "$ROOT/include/token.h"
cp include/lexer.h      "$ROOT/include/lexer.h"
cp src/token.cpp        "$ROOT/src/token.cpp"
cp src/lexer.cpp        "$ROOT/src/lexer.cpp"
cp src/main.cpp         "$ROOT/src/main.cpp"
cp tests/test_lexer.cpp "$ROOT/tests/test_lexer.cpp"
cp tests/CMakeLists.txt "$ROOT/tests/CMakeLists.txt"
cp CMakeLists.txt       "$ROOT/CMakeLists.txt"
cp README.md            "$ROOT/README.md"

# ── Write a sample Riven source file ──────────
cat > "$ROOT/examples/hello.riven" << 'EOF'
~~ Hello World in Riven
riven core {
    imprint("Hello, World!")
    name = inp("Enter your name: ")
    imprint(name)
    firm VERSION = 1.0
    coll nums = [1, 2, 3]
    flow 3 {
        imprint("loop")
    }
}
EOF

# ── Create .gitignore ─────────────────────────
cat > "$ROOT/.gitignore" << 'EOF'
build/
*.o
*.a
*.exe
.DS_Store
EOF

# ── Zip it ────────────────────────────────────
zip -r riven-compiler.zip "$ROOT"
rm -rf "$ROOT"

echo "✅  riven-compiler.zip created successfully!"
echo "    Upload it to GitHub or: unzip riven-compiler.zip && cd riven-compiler"
