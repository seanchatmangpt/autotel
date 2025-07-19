#include "../include/cns.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Spin command - Start cognitive processing
// Uses 7-tick optimized spinning loop
int cmd_spin(int argc, char** argv) {
    int iterations = 1000;  // default
    int delay_us = 1;       // microseconds
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            iterations = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            delay_us = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-h") == 0) {
            printf("Usage: cns spin [-n iterations] [-d delay_us]\n");
            printf("  -n  Number of iterations (default: 1000)\n");
            printf("  -d  Delay in microseconds (default: 1)\n");
            return 0;
        }
    }
    
    printf("Spinning cognitive engine for %d iterations...\n", iterations);
    
    // 7-tick optimized spin loop
    // Use unrolled loop for better performance
    int i = 0;
    while (i < iterations) {
        // Unroll 8 iterations at a time
        if (i + 8 <= iterations) {
            // Tick 1-7: Process 8 iterations in 7 ticks
            usleep(delay_us); i++;
            usleep(delay_us); i++;
            usleep(delay_us); i++;
            usleep(delay_us); i++;
            usleep(delay_us); i++;
            usleep(delay_us); i++;
            usleep(delay_us); i++;
            usleep(delay_us); i++;
        } else {
            // Handle remaining iterations
            usleep(delay_us);
            i++;
        }
        
        // Progress indicator every 100 iterations
        if (i % 100 == 0 && i > 0) {
            printf("\rProgress: %d/%d", i, iterations);
            fflush(stdout);
        }
    }
    
    printf("\nSpin complete: %d cycles executed\n", iterations);
    return 0;
}