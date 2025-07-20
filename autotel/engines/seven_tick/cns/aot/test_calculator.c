#include <stdio.h>
#include "aot_calculator.h"

int main(void) {
    printf("AOT Calculator Test\n");
    printf("Small graph: %zu bytes\n", aot_calculate_for_small_graph());
    printf("Medium graph: %zu bytes\n", aot_calculate_for_medium_graph());
    printf("Large graph: %zu bytes\n", aot_calculate_for_large_graph());
    return 0;
}
