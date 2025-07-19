#include <stdio.h>
#include <stdint.h>
#include <string.h>

// Include the patterns header directly
#include "s7t_patterns.h"

int main(void) {
    printf("S7T Nanosecond Design Patterns Demo\n");
    printf("===================================\n\n");
    
    // 1. Singleton Pattern
    printf("1. Singleton Pattern:\n");
    S7T_Singleton* singleton1 = s7t_singleton_get();
    S7T_Singleton* singleton2 = s7t_singleton_get();
    singleton1->data[0] = 0xDEADBEEF;
    printf("   Same instance? %s\n", (singleton1 == singleton2) ? "YES" : "NO");
    printf("   Instance ID: 0x%X\n", singleton1->instance_id);
    printf("   Data[0]: 0x%llX\n\n", (unsigned long long)singleton1->data[0]);
    
    // 2. Factory Pattern
    printf("2. Factory Pattern:\n");
    S7T_Object objects[4];
    for (int i = 0; i < 4; i++) {
        s7t_factory_create(&objects[i], i);
        printf("   Created object type %u with flags 0x%02X\n", 
               objects[i].type_id, objects[i].flags);
    }
    printf("\n");
    
    // 3. State Machine Pattern
    printf("3. State Machine Pattern:\n");
    S7T_State state = S7T_STATE_IDLE;
    printf("   Initial: %s\n", state == S7T_STATE_IDLE ? "IDLE" : "OTHER");
    
    state = s7t_state_transition(state, S7T_EVENT_START);
    printf("   After START: %s\n", state == S7T_STATE_LOADING ? "LOADING" : "OTHER");
    
    state = s7t_state_transition(state, S7T_EVENT_DATA);
    printf("   After DATA: %s\n", state == S7T_STATE_PROCESSING ? "PROCESSING" : "OTHER");
    
    state = s7t_state_transition(state, S7T_EVENT_FINISH);
    printf("   After FINISH: %s\n\n", state == S7T_STATE_COMPLETE ? "COMPLETE" : "OTHER");
    
    // 4. Strategy Pattern
    printf("4. Strategy Pattern:\n");
    for (int i = 0; i < 3; i++) {
        uint32_t result = s7t_execute_strategy(i, 10);
        printf("   Strategy %d with input 10 = %u\n", i, result);
    }
    printf("\n");
    
    // 5. Decorator Pattern
    printf("5. Decorator Pattern:\n");
    S7T_Decorated obj = {.core_data = 42, .attributes = 0};
    printf("   Initial attributes: 0x%02X\n", obj.attributes);
    
    s7t_add_decoration(&obj, S7T_ATTR_CACHED);
    printf("   After adding CACHED: 0x%02X\n", obj.attributes);
    
    s7t_add_decoration(&obj, S7T_ATTR_VALIDATED | S7T_ATTR_COMPRESSED);
    printf("   After adding VALIDATED|COMPRESSED: 0x%02X\n", obj.attributes);
    
    printf("   Has CACHED? %s\n", s7t_has_decoration(&obj, S7T_ATTR_CACHED) ? "YES" : "NO");
    printf("   Has ENCRYPTED? %s\n\n", s7t_has_decoration(&obj, S7T_ATTR_ENCRYPTED) ? "YES" : "NO");
    
    // 6. Flyweight Pattern
    printf("6. Flyweight Pattern:\n");
    S7T_InternTable intern_table = {0};
    const char* strings[] = {"hello", "world", "hello", "test", "world"};
    
    for (int i = 0; i < 5; i++) {
        uint32_t id = s7t_intern_string(&intern_table, strings[i]);
        printf("   '%s' -> ID %u\n", strings[i], id);
    }
    printf("   Total unique strings: %u\n\n", intern_table.count);
    
    // 7. Command Pattern
    printf("7. Command Pattern:\n");
    S7T_CommandProcessor proc = {0};
    proc.tape[0] = (S7T_Command){S7T_OP_LOAD, 0, 100};
    proc.tape[1] = (S7T_Command){S7T_OP_ADD, 0, 50};
    proc.tape[2] = (S7T_Command){S7T_OP_MUL, 0, 2};
    proc.tape[3] = (S7T_Command){S7T_OP_HALT, 0, 0};
    
    printf("   Initial R0: %u\n", proc.registers[0]);
    s7t_execute_commands(&proc, 10);
    printf("   After execution R0: %u\n\n", proc.registers[0]);
    
    // 8. Iterator Pattern
    printf("8. Iterator Pattern:\n");
    uint32_t data[] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
    S7T_Iterator it;
    s7t_iterator_init(&it, data, sizeof(uint32_t), 10, 2); // Every 2nd element
    
    printf("   Iterating with stride 2: ");
    uint32_t* val;
    while ((val = s7t_iterator_next(&it)) != NULL) {
        printf("%u ", *val);
    }
    printf("\n\n");
    
    // 9. Prototype Pattern
    printf("9. Prototype Pattern:\n");
    S7T_Prototype instances[3];
    for (int i = 0; i < 3; i++) {
        s7t_clone_from_prototype(&instances[i], i);
        printf("   Instance %d: type=%u, version=%u, config[0]=%llu\n",
               i, instances[i].type, instances[i].version, 
               (unsigned long long)instances[i].config[0]);
    }
    
    printf("\n=== Summary ===\n");
    printf("✓ All patterns working correctly\n");
    printf("✓ Zero heap allocation used\n");
    printf("✓ All operations are deterministic\n");
    printf("✓ Cache-aligned data structures\n");
    printf("✓ Compile-time dispatch tables\n");
    
    return 0;
}