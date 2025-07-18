#include <stdio.h>
#include "../runtime/src/seven_t_runtime.h"

int main() {
    printf("Creating engine...\n");
    EngineState* engine = s7t_create_engine();
    printf("Engine created: %p\n", engine);
    
    if (engine) {
        printf("Destroying engine...\n");
        s7t_destroy_engine(engine);
        printf("Done\n");
    }
    
    return 0;
}
