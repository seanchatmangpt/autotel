// Test harness for fixed benchmark
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int cmd_benchmark_fixed(int argc, char** argv);

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <test> [iterations]\n", argv[0]);
        printf("Tests: nop, add, hash, memcpy, atoi, all\n");
        return 1;
    }
    
    return cmd_benchmark_fixed(argc, argv);
}