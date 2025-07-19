// Test harness for real commands
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// External command declarations
extern int cmd_ml_real(int argc, char** argv);
extern int cmd_benchmark_real(int argc, char** argv);

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <ml|benchmark> [args...]\n", argv[0]);
        return 1;
    }
    
    if (strcmp(argv[1], "ml") == 0) {
        // Shift arguments for ML command
        return cmd_ml_real(argc - 1, argv + 1);
    } else if (strcmp(argv[1], "benchmark") == 0) {
        // Shift arguments for benchmark command  
        return cmd_benchmark_real(argc - 1, argv + 1);
    } else {
        printf("Unknown command: %s\n", argv[1]);
        return 1;
    }
}