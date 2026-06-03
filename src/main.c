#include "../include/riven.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Reads the Riven source file into a heap-allocated string buffer.
 */
static char* read_file(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Fatal Error: Could not open source file \"%s\".\n", path);
        exit(74);
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(fileSize + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Fatal Error: Not enough memory to read \"%s\".\n", path);
        fclose(file);
        exit(74);
    }

    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
        fprintf(stderr, "Fatal Error: Could not read file \"%s\".\n", path);
        free(buffer);
        fclose(file);
        exit(74);
    }

    buffer[bytesRead] = '\0';
    fclose(file);
    return buffer;
}

/**
 * CLI Driver for Riven Native Compiler Toolchain.
 */
int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Riven Native Compiler (rvn) v1.0\n");
        printf("Usage:\n");
        printf("  rvn run <file.rv>     - Compiles and executes immediately\n");
        printf("  rvn forge <file.rv>   - Builds a permanent native executable\n");
        return 64;
    }

    const char* command = argv[1];
    const char* filename = argv[2];

    /* 1. Read Source */
    char* source = read_file(filename);

    /* 2. Lexical Analysis Initialization */
    lexer_init(source);

    /* 3. Syntax Analysis (Parsing) into AST */
    ASTNode* root = parser_parse();
    if (root == NULL) {
        /* Error messages handled by parser */
        free(source);
        return 65; 
    }

    /* 4. Native Code Generation (Transpilation to C11) */
    /* Generates 'riven_native.c' in the current working directory */
    codegen_generate(root, "riven_native.c");

    /* 5. Toolchain Invocation (System C Compiler) */
    char compile_cmd[2048];
    const char* output_name = "riven_output";
    char* final_binary_name = NULL;

    if (strcmp(command, "forge") == 0) {
        /* Strip .rv and use it as binary name */
        char* out_name = strdup(filename);
        char* dot = strrchr(out_name, '.');
        if (dot) *dot = '\0';
        final_binary_name = out_name;
    } else {
        final_binary_name = strdup(output_name);
    }

    /* FIX: Added -Iinclude flag to allow the backend compiler to find Riven runtime headers.
       We link with src/runtime.c directly during the forge process. */
#ifdef _WIN32
    snprintf(compile_cmd, sizeof(compile_cmd), 
             "gcc -O3 riven_native.c src/runtime.c -o %s.exe -Iinclude -lws2_32 -lpthread", final_binary_name);
#else
    /* On Linux/Termux, use lpthread and lm for math/async support */
    snprintf(compile_cmd, sizeof(compile_cmd), 
             "gcc -O3 riven_native.c src/runtime.c -o %s -Iinclude -lpthread -lm", final_binary_name);
#endif

    printf("Riven: Building native binary [%s]...\n", final_binary_name);
    int compile_result = system(compile_cmd);

    if (compile_result != 0) {
        fprintf(stderr, "Toolchain Error: Native compilation failed (Exit Code: %d).\n", compile_result);
        free(source);
        free(final_binary_name);
        ast_free(root);
        return 1;
    }

    /* 6. Execution (for 'run' command) */
    if (strcmp(command, "run") == 0) {
        char run_cmd[512];
#ifdef _WIN32
        snprintf(run_cmd, sizeof(run_cmd), "%s.exe", final_binary_name);
#else
        snprintf(run_cmd, sizeof(run_cmd), "./%s", final_binary_name);
#endif
        printf("Riven: Executing...\n\n");
        system(run_cmd);

        /* Cleanup temporary artifacts */
        remove("riven_native.c");
#ifdef _WIN32
        char cleanup_bin[512];
        snprintf(cleanup_bin, sizeof(cleanup_bin), "del %s.exe", final_binary_name);
        system(cleanup_bin);
#else
        remove(final_binary_name);
#endif
    } else {
        /* Success message for 'forge' command */
        printf("Riven: Native binary forged successfully -> ./%s\n", final_binary_name);
        remove("riven_native.c");
    }

    /* Final Memory Cleanup of the compiler process */
    ast_free(root);
    free(source);
    free(final_binary_name);

    return 0;
}
