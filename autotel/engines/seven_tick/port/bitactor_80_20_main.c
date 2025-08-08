#include "cns/bitactor_80_20.h"
#include <stdio.h>

int main(int argc, char* argv[]) {
    printf("🌌 BitActor 80/20 System - Sub-100ns Semantic Computing\n");
    printf("======================================================\n\n");
    
    // Run the benchmark
    benchmark_bitactor_80_20();
    
    // Run L6-L7 permutation benchmark
    benchmark_l6_l7_permutations();
    
    printf("\n🎯 BitActor 80/20 Demo Complete\n");
    return 0;
}